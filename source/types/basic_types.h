#pragma once

#include "schema.h"

#define XYBasicType(t) k_types_basic_ptr_##t

#define XYTypesUseBasicType(t) \
    extern TypeSchemaPtr XYBasicType(t)

#define XYTypesCheckType(tp) \
    if constexpr (std::is_same_v<T, tp>) { \
        return XYBasicType(tp); \
    }

namespace xynq {

XYTypesUseBasicType(int);
XYTypesUseBasicType(int8_t);
XYTypesUseBasicType(int16_t);
XYTypesUseBasicType(int32_t);
XYTypesUseBasicType(int64_t);

XYTypesUseBasicType(unsigned);
XYTypesUseBasicType(uint8_t);
XYTypesUseBasicType(uint16_t);
XYTypesUseBasicType(uint32_t);
XYTypesUseBasicType(uint64_t);

XYTypesUseBasicType(float);
XYTypesUseBasicType(double);

XYTypesUseBasicType(StrSpan);

enum class FrameBarrier{};
XYTypesUseBasicType(FrameBarrier); // Auxilary type needed for slang.

template<class T>
constexpr TypeSchemaPtr GetBasicType() {
    XYTypesCheckType(int);
    XYTypesCheckType(int8_t);
    XYTypesCheckType(int16_t);
    XYTypesCheckType(int32_t);
    XYTypesCheckType(int64_t);
    XYTypesCheckType(unsigned);
    XYTypesCheckType(uint8_t);
    XYTypesCheckType(uint16_t);
    XYTypesCheckType(uint32_t);
    XYTypesCheckType(uint64_t);
    XYTypesCheckType(float);
    XYTypesCheckType(double);
    XYTypesCheckType(StrSpan);

    return k_types_invalid_schema;
}


} // xynq