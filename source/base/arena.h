#pragma once

#include <new>
#include <stddef.h>

namespace xynq {

class Arena {
public:
    using Ptr = Arena*;
    BaseAllocator *allocator = nullptr;
    Arena::Ptr next = nullptr;

    inline uint8_t *Begin() { return bytes_; }
    inline uint8_t *End() { return bytes_end_; }
    inline size_t Size() const { return bytes_end_ - bytes_; }

    template<class AllocatorType>
    inline static Arena::Ptr Create(AllocatorType *allocator, size_t size, size_t alignment) {
        XYAssert(detail::IsValidAlignment(alignment));

        void *data = allocator->Alloc(sizeof(Arena));
        Arena *arena = new(data) Arena;
        arena->bytes_ = (uint8_t *)allocator->AllocAligned(alignment, size);
        arena->bytes_end_ = arena->Begin() + size;
        arena->allocator = allocator;
        return arena;
    }

    inline static void Destroy(Arena::Ptr arena) {
        Arena::Ptr cur = arena;
        while (cur != nullptr) {
            Arena::Ptr next = cur->next;
            BaseAllocator *allocator = cur->allocator;
            XYAssert(allocator != nullptr);
            allocator->Free(cur->bytes_);
            cur->~Arena();
            allocator->Free(cur);
            cur = next;
        }
    }

private:
    uint8_t *bytes_end_ = nullptr;
    uint8_t *bytes_ = nullptr;
};

} // xynq
