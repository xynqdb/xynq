#pragma once

#include "object.h"
#include "storage.h"

#include "base/either.h"
#include "types/value_types.h"
#include "types/basic_types.h"


namespace xynq {

struct ObjectWriteSuccess{};
using ObjectWriterResult = Either<StrSpan, ObjectWriteSuccess>;

class ObjectWriter {
public:
    ObjectWriter(Object::Handle object, TypeSchemaPtr schema, Dep<Storage> storage);
    ~ObjectWriter();

    ObjectWriterResult WriteTyped(StrSpan field_name, TypeSchemaPtr type, Value value);
private:
    Dep<Storage> storage_;
    Object *object_ = nullptr;
    TypeSchemaPtr schema_ = k_types_invalid_schema;
    std::pair<void *, TypeSchemaPtr> FindDataStore(StrSpan field_name);
};

} // xynq