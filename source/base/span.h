#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "base/assert.h"

namespace xynq {

namespace detail {

template<class T>
struct SpanIndexed {
    using Ret = T&;
    using CRet = const T&;
    inline static Ret Get(T *ptr, size_t i) {
        return ptr[i];
    }

    inline static CRet CGet(const T *ptr, size_t i) {
        return ptr[i];
    }
};

template<>
struct SpanIndexed<void> {
    using Ret = uint8_t&;
    using CRet = const uint8_t&;
    inline static Ret Get(void *ptr, size_t i) {
        return ((uint8_t *)ptr)[i];
    }

   inline static CRet CGet(const void *ptr, size_t i) {
        return ((uint8_t *)ptr)[i];
    }
};

template<class T>
struct SpanPtrCast {
    using Type = T*;
    using CType = const T*;
};

template<>
struct SpanPtrCast<void> {
    using Type = uint8_t*;
    using CType = const uint8_t*;
};

} // detail

// Equivalent of C++20 std::span.
// Immutable. Only returning consts.
template<class Elem, class SizeType>
class BasicSpan {
public:
    using Iterator = const Elem*;

    inline BasicSpan() = default;
    inline BasicSpan(const Elem *begin, const Elem *end);
    inline BasicSpan(const Elem *begin, SizeType size);
    template<class StdContainer>
    explicit inline BasicSpan(const StdContainer &cont)
        : BasicSpan(cont.data(), cont.size())
    {}

    template<size_t Size, class T = Elem>
    inline BasicSpan(typename std::enable_if<!std::is_same<T, void>::value && !std::is_same<T, char>::value, const T>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size)
    {}

    // String literal.
    template<size_t Size, class T = Elem>
    inline BasicSpan(typename std::enable_if<std::is_same<T, char>::value, const T>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size - 1)
    {
        static_assert(Size > 0, "Invalid string literal");
    }

    template<size_t Size, class T = Elem>
    inline BasicSpan(typename std::enable_if<std::is_same<T, void>::value, const char>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size)
    {}

    inline BasicSpan(const BasicSpan &) = default;
    inline BasicSpan(BasicSpan &&) = default;
    inline BasicSpan &operator=(const BasicSpan &) = default;
    inline BasicSpan &operator=(BasicSpan &&) = default;

    inline const Elem *Data() const { return ptr_; }
    inline SizeType Size() const { return size_; }
    inline bool IsEmpty() const { return size_ == 0; }

    inline Iterator begin() const { return ptr_; }
    inline Iterator end() const { return (typename detail::SpanPtrCast<Elem>::CType)ptr_ + size_; }

    inline auto operator[](size_t index) const -> typename detail::SpanIndexed<Elem>::CRet {
        return detail::SpanIndexed<Elem>::CGet(ptr_, index);
    }
protected:
    const Elem *ptr_ = nullptr;
    SizeType size_ = 0;
};

// Mutable Span.
template<class Elem, class SizeType>
class MutBasicSpan {
public:
    using Iterator = Elem*;

    inline MutBasicSpan() = default;
    inline MutBasicSpan(Elem *begin, Elem *end);
    inline MutBasicSpan(Elem *begin, SizeType size);

    template<size_t Size, class T = Elem>
    inline MutBasicSpan(typename std::enable_if<!std::is_same<T, void>::value && !std::is_same<T, char>::value, T>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size)
    {}

    // String literal.
    template<size_t Size, class T = Elem>
    inline MutBasicSpan(typename std::enable_if<std::is_same<T, char>::value, T>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size - 1)
    {
        static_assert(Size > 0, "Invalid string buffer");
    }

    template<size_t Size, class T = Elem>
    inline MutBasicSpan(typename std::enable_if<std::is_same<T, void>::value, char>::type (&arr) [Size])
        : ptr_(&arr[0])
        , size_(Size)
    {}

    inline MutBasicSpan(const MutBasicSpan &rhs) = default;
    inline MutBasicSpan(MutBasicSpan &&rhs) = default;
    inline MutBasicSpan &operator=(MutBasicSpan &rhs) = default;
    inline MutBasicSpan &operator=(MutBasicSpan &&rhs) = default;

    inline Elem *Data() { return ptr_; }
    inline const Elem *Data() const { return ptr_; }
    inline SizeType Size() const { return size_; }
    inline bool IsEmpty() const { return size_ == 0; }
    inline operator BasicSpan<Elem, SizeType>() const { return BasicSpan{ptr_, size_}; }

    inline Iterator begin() const { return ptr_; }
    inline Iterator end() const { return (typename detail::SpanPtrCast<Elem>::Type)ptr_ + size_; }

    inline auto operator[](size_t index) -> typename detail::SpanIndexed<Elem>::Ret {
        return detail::SpanIndexed<Elem>::Get(ptr_, index);
    }
    inline auto operator[](size_t index) const -> typename detail::SpanIndexed<Elem>::CRet {
        return detail::SpanIndexed<Elem>::CGet(ptr_, index);
    }

protected:
    Elem *ptr_ = nullptr;
    SizeType size_ = 0;
};

using StrSpan = BasicSpan<char, size_t>;
using MutStrSpan = MutBasicSpan<char, size_t>;


// Equivalent of std::string_view but guarantees that
// underlying string is zero terminated C string.
// So it's convinient to use with plain C functions.
// Size returns length of the string without counting ending zero.
class CStrSpan : public StrSpan {
public:
    inline CStrSpan()
        : CStrSpan("", size_t(0))
    {}

    inline CStrSpan(const char *c_str)
        : CStrSpan(c_str, strlen(c_str))
    {}

    inline CStrSpan(const char *begin, const char *end)
        : BasicSpan(begin, end) {
        XYAssert(begin != nullptr);
        XYAssert(end != nullptr);
        XYAssert(*end == '\0'); // check for zero terminated str.
    }

    inline CStrSpan(const char *begin, size_t length)
        : BasicSpan(begin, length) {
        XYAssert(begin != nullptr);
        XYAssert(begin[length] == '\0'); // check for zero terminated str.
    }

    inline CStrSpan(const CStrSpan &rhs) = default;
    inline CStrSpan(CStrSpan &&rhs) = default;
    inline CStrSpan &operator=(const CStrSpan &) = default;
    inline CStrSpan &operator=(CStrSpan &&) = default;

    // Zero-terminated string.
    inline const char *CStr() const { return ptr_; }
};

template<class T> using Span = BasicSpan<T, size_t>;
template<class T> using MutSpan = MutBasicSpan<T, size_t>;
using DataSpan = Span<void>;
using ByteSpan = Span<uint8_t>;
using MutDataSpan = MutSpan<void>;
using MutByteSpan = MutSpan<uint8_t>;
////////////////////////////////////////////////////////////


// Implementation.
template<class Elem, class SizeType>
BasicSpan<Elem, SizeType>::BasicSpan(const Elem *begin, const Elem *end)
    : ptr_(begin)
    , size_((std::byte *)end - (std::byte *)begin)
{}

template<class Elem, class SizeType>
BasicSpan<Elem, SizeType>::BasicSpan(const Elem *begin, const SizeType size)
    : ptr_(begin)
    , size_(size) {
}

template<class Elem, class SizeType>
MutBasicSpan<Elem, SizeType>::MutBasicSpan(Elem *begin, Elem *end) {
    auto sz = (std::byte *)end - (std::byte *)begin;

    ptr_ = begin;
    size_ = static_cast<SizeType>(sz);
}

template<class Elem, class SizeType>
MutBasicSpan<Elem, SizeType>::MutBasicSpan(Elem *begin, SizeType size)
    : ptr_(begin)
    , size_(size) {
}

inline bool operator==(StrSpan left, const char *right) {
    XYAssert(right != nullptr);
    const char *str = left.begin();
    while (*right != 0 && str != left.end()) {
        if (*right++ != *str++) {
            return false;
        }
    }

    return str == left.end() && *right == 0;
}

inline bool operator!=(StrSpan left, const char *right) {
    return !(left == right);
}

inline bool operator==(StrSpan left, StrSpan right) {
    XYAssert(left.Data() != nullptr);
    XYAssert(right.Data() != nullptr);

    if (left.Size() != right.Size()) {
        return false;
    }

    return memcmp(left.Data(), right.Data(), left.Size()) == 0;
}

inline bool operator!=(StrSpan left, StrSpan right) {
    return !(left == right);
}

} // namespace xynq
