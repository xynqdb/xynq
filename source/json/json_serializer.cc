#include "json_serializer.h"

#include "base/stream.h"
#include "base/str_builder.h"
#include "base/system_allocator.h"

#include <stdint.h>

using namespace xynq;

JsonSerializer::JsonSerializer(StreamWriter &writer)
    : writer_(writer) {

}

SerializerResult JsonSerializer::Serialize(TypedValue value) {
    if (value.type->IsBasic()) {
        WriteBasicValue(value);
    } else {
        WriteObject(value.value.ptr, value.type);
    }

    return FinalizeWrite();
}

SerializerResult JsonSerializer::Serialize(Span<TypedValue> values) {
    auto it = values.begin();
    writer_.Write('[');
    while (it != values.end()) {
        if (it->type->IsBasic()) {
            WriteBasicValue(*it);
        } else {
            WriteObject(it->value.ptr, it->type);
        }

        ++it;
        if (it != values.end()) {
            writer_.Write(", ");
        }
    }
    writer_.Write(']');
    return FinalizeWrite();
}

SerializerResult JsonSerializer::Serialize(StrSpan str) {
    WriteEscapedString(str);
    return FinalizeWrite();
}

SerializerResult JsonSerializer::FinalizeWrite() {
    writer_.Write('\n');
    writer_.Flush();
    if (writer_.IsGood()) {
        return SerializerSuccess{};
    } else {
        return StrSpan{"Failed to serialize - I/O error"};
    }

}

void JsonSerializer::WriteObject(const void *object, TypeSchemaPtr schema) {
    XYAssert(schema->IsAligned(object));

    writer_.Write('{');
    {
        const FieldSchema *field = schema->fields;
        size_t num_fields = schema->field_count;
        while (num_fields--) {
            writer_.Write("\"");
            writer_.Write(field->name);
            writer_.Write("\":");

            object = TypeSchema::AlignPtr(object, field->schema->alignment);
            if (field->schema->IsBasic()) {
                XYAssert(field->schema->size <= sizeof(Value));
                WriteBasicValue(field->schema, object);
            } else {
                WriteObject(object, field->schema);
            }
            object = TypeSchema::OffsetPtr(object, field->schema->size);
            ++field;
            if (num_fields != 0) {
                writer_.Write(", ");
            }
        }
    }
    writer_.Write('}');
}

void JsonSerializer::WriteBasicValue(const TypeSchemaPtr type, const void *data) {
    XYAssert(type != nullptr);
    XYAssert(data != nullptr);

    if (type->IsFloatingPoint()) {
        double dbl;
        switch (type->size) {
            case 4:
                dbl = *static_cast<const float *>(data);
                break;
            case 8:
                dbl = *static_cast<const double *>(data);
                break;
            default:
                XYAssert(0);
        }
        WriteBasicValue(TypedValue{type, dbl});
    } else if (type->IsUnsignedInt()) {
        uint64_t val;
        switch (type->size) {
            case 1:
                val = *static_cast<const uint8_t *>(data);
                break;
            case 2:
                val = *static_cast<const uint16_t *>(data);
                break;
            case 4:
                val = *static_cast<const uint32_t *>(data);
                break;
            case 8:
                val = *static_cast<const uint64_t *>(data);
                break;
            default:
                XYAssert(0);
        }

        WriteBasicValue(TypedValue{type, val});
    } else {
        int64_t val;
        switch (type->size) {
            case 1:
                val = *static_cast<const int8_t *>(data);
                break;
            case 2:
                val = *static_cast<const int16_t *>(data);
                break;
            case 4:
                val = *static_cast<const int32_t *>(data);
                break;
            case 8:
                val = *static_cast<const int64_t *>(data);
                break;
            default:
                XYAssert(0);
        }

        WriteBasicValue(TypedValue{type, val});
    }
}

void JsonSerializer::WriteBasicValue(TypedValue value) {
    StrBuilder<128> str_builder;
    if (value.type->IsUnsignedInt()) {
        str_builder << value.value.u64;
    } else if (value.type->IsSignedInt()) {
        str_builder << value.value.i64;
    } else if (value.type->IsFloatingPoint()) {
        str_builder << StrHiPrecision(value.value.dbl); // todo: use high precsision

        // Failed to write, need to allocate memory dynamically,
        // should be extremely rare case if ever happen at all.
        if (str_builder.Buffer().IsEmpty()) {
            int sz = std::snprintf(0, 0, "%.24g", value.value.dbl);
            if (sz > 0) {
                char *buf = (char *)SystemAllocator::Shared().Alloc(sz + 1);
                sz = std::snprintf(buf, sz + 1, "%.24g", value.value.dbl);
                if (sz > 0) {
                    writer_.WriteData(DataSpan{buf, (size_t)sz});
                }
                SystemAllocator::Shared().Free(buf);
            }
        }
    } else if (value.type == XYBasicType(StrSpan)) {
        WriteEscapedString(value.value.str);
    } else {
        XYAssert(0);
    }
    writer_.Write(str_builder.Buffer());
}

void JsonSerializer::WriteEscapedString(StrSpan str) {
    const char *cur = str.begin();
    const char *prev = cur;
    const char *e = str.end();

    writer_.Write('"');

    while (cur != e) {
        switch (*cur) {
            case '\\':
            case '"':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\\"");
                prev = ++cur;
                break;
            case '\b':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\b");
                prev = ++cur;
                break;
            case '\t':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\t");
                prev = ++cur;
                break;
            case '\n':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\n");
                prev = ++cur;
                break;
            case '\f':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\f");
                prev = ++cur;
                break;
            case '\r':
                writer_.Write(StrSpan(prev, cur));
                writer_.Write("\\r");
                prev = ++cur;
                break;
            default:
                unsigned char c = *cur;
                if (c < 0x10) {
                    writer_.Write(StrSpan(prev, cur));
                    StrBuilder<64> esc;
                    esc << "\\u000" << StrHex{c, false};
                    writer_.Write(esc.Buffer());
                    prev = ++cur;
                } else if (c < 0x20) {
                    writer_.Write(StrSpan(prev, cur));
                    StrBuilder<64> esc;
                    esc << "\\u00" << StrHex{c, false};
                    writer_.Write(esc.Buffer());
                    prev = ++cur;
                } else {
                    writer_.Write(c);
                    prev = ++cur;
                }
        }
    }

    writer_.Write(StrSpan(prev, cur));
    writer_.Write('"');
}
