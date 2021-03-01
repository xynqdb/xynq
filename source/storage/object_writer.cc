#include "object_writer.h"

using namespace xynq;

namespace {

double ValueToDouble(TypeSchemaPtr type, Value value) {
    if (type == XYBasicType(double)) {
        return value.dbl;
    } else if (type->IsSignedInt()) {
        return static_cast<double>(value.i64);
    } else {
        return static_cast<double>(value.u64);
    }
}

uint64_t ValueToUInt64(TypeSchemaPtr type, Value value) {
    if (type == XYBasicType(double)) {
        return static_cast<uint64_t>(value.dbl);
    } else {
        return value.u64;
    }
}

void WriteBasicValue(TypeSchemaPtr type, Value value, TypeSchemaPtr dst_type, void *dst_buf) {
    if (dst_type == XYBasicType(double)) {
        *(double *)dst_buf = ValueToDouble(type, value);
    } else if (dst_type == XYBasicType(float)) {
        *(float *)dst_buf = (float)ValueToDouble(type, value);
    } else { // Integral type.
        switch (dst_type->size) {
            case 1:
                *(uint8_t *)dst_buf = static_cast<uint8_t>(ValueToUInt64(type, value));
                break;
            case 2:
                *(uint16_t *)dst_buf = static_cast<uint16_t>(ValueToUInt64(type, value));
                break;
            case 4:
                *(uint32_t *)dst_buf = static_cast<uint32_t>(ValueToUInt64(type, value));
                break;
            case 8:
                *(uint64_t *)dst_buf = static_cast<uint64_t>(ValueToUInt64(type, value));
                break;
            default:
                XYAssert(0);
        }
    }
}

} // anon namespace


ObjectWriter::ObjectWriter(Object::Handle object, TypeSchemaPtr schema, Dep<Storage> storage)
    : storage_(storage)
    , schema_(schema) {
    object_ = storage_->LockObject(object);
}

ObjectWriter::~ObjectWriter() {
    storage_->UnlockObject(object_);
}

std::pair<void *, TypeSchemaPtr> ObjectWriter::FindDataStore(StrSpan field_name) {
    const FieldSchema *field = &schema_->fields[0];
    const FieldSchema *field_end = field + schema_->field_count;
    void *cur = object_->Data();

    while (field != field_end) {
        cur = TypeSchema::AlignPtr(cur, field->schema->alignment);
        if (field->name == field_name) {
            return {cur, field->schema};
        }
        cur = TypeSchema::OffsetPtr(cur, field->schema->size);
        ++field;
    }

    return {nullptr, k_types_invalid_schema};
}

ObjectWriterResult ObjectWriter::WriteTyped(StrSpan field_name, TypeSchemaPtr type, Value value) {
    auto [field_ptr, field_type] = FindDataStore(field_name);
    if (field_ptr == nullptr) {
        return StrSpan{"Field does not exist"};
    }

    if (field_type->IsBasic()) {
        WriteBasicValue(type, value, field_type, field_ptr);
    } else {
        return StrSpan{"Unsupported type"};
    }
    return ObjectWriteSuccess{};
}