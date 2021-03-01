#pragma once

#include <string>
#include "base/allocator.h"
#include "base/scratch_allocator.h"
#include "base/span.h"
#include "base/system_allocator.h"

namespace xynq {


// Those are basically std string with xynq allocators.

using StrBaseType = std::basic_string<char, std::char_traits<char>, StdAlloc<char, BaseAllocator>>;
using ScratchStrBaseType = std::basic_string<char, std::char_traits<char>, StdAlloc<char, ScratchAllocator>>;


// Dynamically allocated string. Equivalent of std::string.
class Str final : public StrBaseType {
public:
    Str()
        : StrBaseType{&SystemAllocator::Shared()}
    {}

    explicit Str(BaseAllocator *allocator)
        : StrBaseType{allocator}
    {}

    Str(const Str &other)
        : StrBaseType(other, &SystemAllocator::Shared())
    {}

    Str(const char *str)
        : StrBaseType(str, &SystemAllocator::Shared())
    {}

    Str(CStrSpan str)
        : StrBaseType(str.Data(), str.Size(), &SystemAllocator::Shared())
    {}

    Str(StrSpan str)
        : StrBaseType(str.Data(), str.Size(), &SystemAllocator::Shared())
    {}

    using StrBaseType::operator=;
    using StrBaseType::operator+=;

    operator CStrSpan() const { return CStrSpan{c_str(), size()}; }
    operator StrSpan() const { return StrSpan{c_str(), size()}; }
};

inline static Str operator+(const Str &left, const Str &right) {
    Str result;
    result += left;
    result += right;
    return result;
}

class ScratchStr final : public ScratchStrBaseType {
public:
    ScratchStr()
        : ScratchStrBaseType{(ScratchAllocator*)nullptr}
    {}

    explicit ScratchStr(ScratchAllocator *allocator)
        : ScratchStrBaseType{allocator}
    {}

    ScratchStr(const ScratchStr &other)
        : ScratchStrBaseType(other)
    {}

    ScratchStr(ScratchStr &&other)
        : ScratchStrBaseType(std::forward<ScratchStr>(other))
    {}

    ScratchStr &operator=(const ScratchStr &other) {
        ScratchStrBaseType::assign(other);
        return *this;
    }

    ScratchStr &operator=(ScratchStr &&other) {
        (ScratchStrBaseType &)(*this) = std::move(other);
        return *this;
    }

    using ScratchStrBaseType::operator=;
    using ScratchStrBaseType::operator+=;
};

// Allocates a copy of the string str with scratch allocator.
StrSpan MakeScratchStrCopy(StrSpan str, ScratchAllocator &allocator);
CStrSpan MakeScratchCStrCopy(StrSpan str, ScratchAllocator &allocator);

} // xynq