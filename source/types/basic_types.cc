#include "schema.h"
#include "basic_types.h"

#define TYPE_VAR_NAME(a, b) a ## b

#define XYDefineBasicTypeSchemaNamed(t, name, extra_flags)  \
    TypeSchema TYPE_VAR_NAME(k_types_basic_, t) {       \
        name,                                           \
        alignof(t),                                     \
        sizeof(t),                                      \
        extra_flags | TypeSchemaFlags::kBasic,          \
        0, {}                                           \
    };                                                  \
    TypeSchemaPtr TYPE_VAR_NAME(k_types_basic_ptr_, t) = &k_types_basic_##t

#define XYDefineBasicTypeSchema(t) XYDefineBasicTypeSchemaNamed(t, #t, 0)

namespace xynq {

XYDefineBasicTypeSchemaNamed(int, "int", TypeSchemaFlags::kSignedInt);
XYDefineBasicTypeSchemaNamed(int8_t, "int8", TypeSchemaFlags::kSignedInt);
XYDefineBasicTypeSchemaNamed(int16_t, "int16", TypeSchemaFlags::kSignedInt);
XYDefineBasicTypeSchemaNamed(int32_t, "int32", TypeSchemaFlags::kSignedInt);
XYDefineBasicTypeSchemaNamed(int64_t, "int64", TypeSchemaFlags::kSignedInt);

XYDefineBasicTypeSchemaNamed(uint8_t, "uint8", TypeSchemaFlags::kUnsignedInt);
XYDefineBasicTypeSchemaNamed(uint16_t, "uint16", TypeSchemaFlags::kUnsignedInt);
XYDefineBasicTypeSchemaNamed(uint32_t, "uint32", TypeSchemaFlags::kUnsignedInt);
XYDefineBasicTypeSchemaNamed(uint64_t, "uint64", TypeSchemaFlags::kUnsignedInt);

XYDefineBasicTypeSchemaNamed(float, "float", TypeSchemaFlags::kFloatingPoint);
XYDefineBasicTypeSchemaNamed(double, "double", TypeSchemaFlags::kFloatingPoint);

XYDefineBasicTypeSchema(FrameBarrier);
XYDefineBasicTypeSchema(StrSpan);

} // xynq