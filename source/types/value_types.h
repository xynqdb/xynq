#pragma once

#include <stdint.h>
#include "base/span.h"

#include "basic_types.h"

namespace xynq {

// Value storage.
union Value {
    inline Value() {}

    inline Value(uint64_t u64_)
        : u64(u64_)
    {}

    inline Value(int64_t i64_)
        : i64(i64_)
    {}

    inline Value(int i)
        : i64(i)
    {}

    inline Value(double dbl_)
        : dbl(dbl_)
    {}

    inline Value(const void *ptr_)
        : ptr(ptr_)
    {}

    inline Value(StrSpan str_)
        : str(str_)
    {}

    uint64_t u64;
    int64_t i64;
    double dbl;
    const void *ptr;
    StrSpan str;
};

// Value storage and type pair.
struct TypedValue {
    inline TypedValue(TypeSchemaPtr t, Value v)
        : type(t)
        , value(v)
    {}
    TypedValue() = default;

    TypeSchemaPtr type = k_types_invalid_schema;
    Value value;
};


} // xynq