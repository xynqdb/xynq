#pragma once

#include "assert.h"

#include <cstdint>
#include <stddef.h>
#include <utility>

namespace xynq {

// Polymorphic allocator interface.
class BaseAllocator {
public:
    virtual ~BaseAllocator() = default;

    // Equivalent of malloc()
    // Guarantees to return a buffer that is aligned for any data type.
    virtual void *Alloc(size_t size) = 0;

    // Equivalent of aligned_alloc()/posix_memalign()
    virtual void *AllocAligned(size_t alignment, size_t size) = 0;

    // Equivalent of free()
    virtual void Free(void *mem) = 0;
};
////////////////////////////////////////////////////////////


// Helpers to create/destroy C++ objects with ctors/dtors
template<class T, class AllocatorType, class...Args>
inline T *CreateObject(AllocatorType &allocator, Args&&...args) {
    void *buf = allocator.Alloc(sizeof(T));
    XYAssert(buf != nullptr);
    return new (buf) T(std::forward<Args>(args)...);
}

template<class T, class AllocatorType>
inline void DestroyObject(AllocatorType &allocator, T *object) {
    XYAssert(object != nullptr);
    object->~T();
    allocator.Free((void *)object);
}

// Wrapper to make std-compatible allocator out of xynq::allocators.
// For use with std containers when needed.
template<class T, class AllocatorType>
struct StdAlloc {
    using value_type = T;

    StdAlloc () = default;
    StdAlloc(AllocatorType *allocator)
        : allocator_(allocator)
    {}

    //template <class U> constexpr StdAlloc (const StdAlloc<U, AllocatorType>&) {}
    T *allocate(size_t n) {
        XYAssert(allocator_ != nullptr);
        return (T*)allocator_->Alloc(n * sizeof(T));
    }
    void deallocate(T* p, size_t) {
        XYAssert(allocator_ != nullptr);
        allocator_->Free(p);
    }

    bool operator==(const StdAlloc<T, AllocatorType> &other) const {
        return allocator_ == other.allocator_;
    }

    bool operator!=(const StdAlloc<T, AllocatorType> &other) const {
        return allocator_ != other.allocator_;
    }

    AllocatorType *allocator_ = nullptr;
};
////////////////////////////////////////////////////////////


namespace detail {

inline uint8_t *AlignPointer(uint8_t *ptr, uint64_t alignment) {
    const uintptr_t ptr_val = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<uint8_t *>((ptr_val + alignment - 1) & ~(alignment - 1));
}

inline bool IsValidAlignment(uint64_t alignment) {
    return (alignment & (alignment - 1)) == 0;
}

} // detail

} // xynq
