#pragma once

#include "base/assert.h"
#include "base/dep.h"
#include "base/span.h"

#include <unistd.h>

namespace xynq {

struct TypeSchema;
using TypeSchemaPtr = TypeSchema *;

struct FieldSchema {
    StrSpan name;
    TypeSchemaPtr schema;
};

struct TypeSchemaFlags {
    enum : uint32_t {
        kBasic          = 1 << 0,
        kSignedInt      = 1 << 1,
        kUnsignedInt    = 1 << 2,
        kFloatingPoint  = 1 << 3,
    };
};


struct TypeSchema {
    StrSpan name;
    size_t alignment = 0;
    size_t size = 0;
    uint32_t flags = 0;

    size_t field_count = 0;
    FieldSchema fields[];

    inline bool IsBasic() const { return (flags & TypeSchemaFlags::kBasic) != 0; }
    inline bool IsSignedInt() const { return (flags & TypeSchemaFlags::kSignedInt) != 0; }
    inline bool IsUnsignedInt() const { return (flags & TypeSchemaFlags::kUnsignedInt) != 0; }
    inline bool IsFloatingPoint() const { return (flags & TypeSchemaFlags::kFloatingPoint) != 0; }
    inline bool IsNumeric() const { return IsIntegral() || IsFloatingPoint(); }
    inline bool IsIntegral() const { return IsSignedInt() || IsUnsignedInt(); }
    inline bool IsAligned(const void *object) const {
        XYAssert((alignment & (alignment - 1)) == 0); // accept pow2 only
        return ((reinterpret_cast<uintptr_t>(object)) & (alignment - 1)) == 0;
    }

    inline static const void *AlignPtr(const void *ptr, size_t alignment) {
        const uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<const void *>((ptr_val + alignment - 1) & ~(alignment - 1));
    }

    inline static void *AlignPtr(void *ptr, size_t alignment) {
        const uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<void *>((ptr_val + alignment - 1) & ~(alignment - 1));
    }

    inline static const void *OffsetPtr(const void *ptr, size_t offset_bytes) {
        const uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr) + offset_bytes;
        return reinterpret_cast<const void *>(ptr_val);
    }

    inline static void *OffsetPtr(void *ptr, size_t offset_bytes) {
        uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr) + offset_bytes;
        return reinterpret_cast<void *>(ptr_val);
    }
};

static TypeSchemaPtr k_types_invalid_schema{nullptr};

} // xynq