#pragma once

#include "types/serializer.h"
#include "types/schema.h"

namespace xynq {

class StreamWriter;

class JsonSerializer final : public Serializer {
public:
    JsonSerializer(StreamWriter &writer);

    // Serializer interface.
    SerializerResult Serialize(TypedValue value) final;
    SerializerResult Serialize(Span<TypedValue> values) final;
    SerializerResult Serialize(StrSpan str) final;

private:
    StreamWriter &writer_;

    void WriteObject(const void *object, TypeSchemaPtr schema);
    void WriteBasicValue(const TypeSchemaPtr type, const void *data);
    void WriteBasicValue(TypedValue value);
    void WriteEscapedString(StrSpan str);
    SerializerResult FinalizeWrite();
};

} // xynq