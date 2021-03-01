#include "slang_env.h"

#include "shared_deps.h"

#include "base/log.h"
#include "storage/object_writer.h"

#include "slang/math_funcs.h"

using namespace xynq;
using namespace xynq::slang;

DefineTaggedLog(Slang);

slang::Env xynq::CreateSlangEnv(Dep<JsonPayloadHandler> json_payload_handler) {
    FuncTable func_table;

    // Creates new object, initializes its fields and commits it into storage.
    // First argument is expected to be a type name for the object we are creating.
    // Then it expects pairs of <field name, field value>.
    func_table["create"] = [](slang::CallContext &call_context) -> bool {
        SharedDeps &deps = call_context.UserData<SharedDeps>();

        if (call_context.args->Begin().IsEnd()) {
            call_context.error_text.Append("Not enough arguments for a function. Expected (create type_name [fields]).");
            return false;
        }

        auto arg_it = call_context.args->Begin();
        auto type_name = arg_it.Get<StrSpan>();
        if (!type_name.HasValue()) {
            call_context.error_text.Append("Expected a type name.");
            return false;
        }

        ++arg_it;

        deps.storage->EnsureVaultWithType(deps.types, type_name.Value());
        auto result = deps.storage->CreateObject(type_name.Value());
        if (result.IsLeft()) {
            call_context.error_text << "Failed to create new object of type '" << type_name.Value() << "': " << result.Left();
            return false;
        }

        auto [new_object, new_object_schema] = result.Right();
        while (!arg_it.IsEnd()) {
            // Read by pairs of field name -> field value
            const auto &field_name = arg_it.Get<Field>();
            if (!field_name.HasValue()) {
                call_context.error_text << "Expected field name for type '" << type_name.Value() << "'";
                return false;
            }

            ++arg_it;
            if (arg_it.IsEnd()) {
                call_context.error_text << "Expected value for field '" << ((const StrSpan &)field_name.Value()) << "'";
                return false;
            }

            ObjectWriter writer(new_object, new_object_schema, deps.storage);
            auto written = writer.WriteTyped(field_name.Value(), arg_it.Type(), arg_it.Value());

            if (written.IsLeft()) {
                call_context.error_text << "Failed to write a field: '" << ((const StrSpan &)field_name.Value()) << "': " << written.Left();
                return false;
            }

            ++arg_it;
        }

        call_context.output->AddTyped(new_object_schema, new_object->Data());
        return true;
    };

    // Signature: (select type [filter] [modifier(s)])
    func_table["select"] = [](slang::CallContext &call_context) -> bool {
        if (call_context.args->Begin().IsEnd() || call_context.args->Begin().Type() != XYBasicType(StrSpan)) {
            call_context.error_text.Append("Expected type name.");
            return false;
        }

        auto arg_it = call_context.args->Begin();
        StrSpan type_name = arg_it.Get<StrSpan>().Value();

        SharedDeps &deps = call_context.UserData<SharedDeps>();
        deps.storage->Enumerate(type_name, [&](Object::Handle object, TypeSchemaPtr schema) {
            call_context.output->AddTyped(schema, object->Data());
        });

        return true;
    };

    func_table["list"] = [](slang::CallContext &call_context) -> bool {
        auto it = call_context.args->Begin();
        while (!it.IsEnd()) {
            call_context.output->AddTyped(it.Type(), it.Value());
            ++it;
        }
        return true;
    };

    func_table["defstruct"] = [](slang::CallContext &call_context) -> bool {
        if (call_context.args->Begin().IsEnd() || call_context.args->Begin().Type() != XYBasicType(StrSpan)) {
            call_context.error_text.Append("Expected type name.");
            return false;
        }

        SharedDeps &deps = call_context.UserData<SharedDeps>();
        auto arg_it = call_context.args->Begin();
        StrSpan type_name = arg_it.Get<StrSpan>().Value();

        if (deps.types->HasType(type_name)) {
            call_context.error_text << "Type '" << type_name << "' already exists.";
            return false;
        }
        ++arg_it;

        size_t num_fields = 0;
        size_t fields_size = 0;
        auto arg_tmp = arg_it;
        while (!arg_tmp.IsEnd()) {
            if (arg_tmp.Type() == k_slang_field_type_ptr) {
                ++num_fields;
                fields_size += arg_tmp.GetUnsafe<Field>().Size();
            }
            ++arg_tmp;
        }

        TypeSchemaPtr schema = deps.types->CreateSchema(type_name, num_fields, fields_size,
            [&](TypeSchema &schema, char *fields_buf) -> bool {
                size_t total_size = 0;
                size_t alignment = 0;

                FieldSchema *field_schema = &schema.fields[0];

                while (!arg_it.IsEnd()) {
                    if (arg_it.Type() != k_slang_field_type_ptr) {
                        call_context.error_text << "Expected field name but got '" << arg_it.Type()->name << "'";
                        return false;
                    }

                    // Store field name in the buffer.
                    Field field = arg_it.GetUnsafe<Field>();
                    memcpy(fields_buf, field.Data(), field.Size());
                    field_schema->name = StrSpan{fields_buf, field.Size()};
                    fields_buf += field.Size();

                    ++arg_it;

                    // Now looking for type.
                    if (arg_it.IsEnd() || arg_it.Type() != XYBasicType(StrSpan)) {
                        call_context.error_text << "Expected type name, but got '" << (arg_it.IsEnd() ? "none" : arg_it.Type()->name) << "'";
                        return false;
                    }

                    field_schema->schema = deps.types->FindSchema(arg_it.GetUnsafe<StrSpan>());
                    if (field_schema->schema == k_types_invalid_schema) {
                        call_context.error_text << "Unknown type name '" << arg_it.GetUnsafe<StrSpan>() << "'";
                        return false;
                    }
                    total_size = (uintptr_t)TypeSchema::AlignPtr(reinterpret_cast<void *>(total_size), field_schema->schema->alignment);
                    total_size += field_schema->schema->size;
                    alignment = std::max(alignment, field_schema->schema->alignment);

                    ++arg_it;
                    ++field_schema;
                }

                // add padding
                total_size = (uintptr_t)TypeSchema::AlignPtr(reinterpret_cast<void *>(total_size), alignment);

                schema.alignment = alignment;
                schema.size = total_size;
                return true;
            });

        return schema != k_types_invalid_schema;
    };

    RegisterMathFunctions(func_table);

    PayloadHandlerTable payload_handlers;
    payload_handlers.emplace(std::make_pair(0, json_payload_handler)); // Default.
    payload_handlers.emplace(std::make_pair(MakePayloadHandlerToken("json"), json_payload_handler)); // Json.

    return slang::Env{std::move(func_table), std::move(payload_handlers)};
}
