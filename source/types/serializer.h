#pragma once

#include "value_types.h"

#include "base/either.h"
#include "base/span.h"

namespace xynq {

struct SerializerSuccess{};
using SerializerResult = Either<StrSpan, SerializerSuccess>;

// Generic serializer.
class Serializer {
public:
    virtual ~Serializer() = default;

    // Serializes single value.
    virtual SerializerResult Serialize(TypedValue value) = 0;

    // Serializes list of values.
    virtual SerializerResult Serialize(Span<TypedValue> values) = 0;

    // Basic types
    virtual SerializerResult Serialize(StrSpan value) = 0;
};


// Serializer that is just bypassing all the data. Mostly for tests.
class DummySerializer : public Serializer {
public:
    SerializerResult Serialize(TypedValue) override { return SerializerSuccess{}; }
    SerializerResult Serialize(Span<TypedValue>) override { return SerializerSuccess{}; }
    SerializerResult Serialize(StrSpan) override { return SerializerSuccess{}; }
};

} // xynq