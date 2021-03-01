#pragma once

#include "allocator.h"
#include "arena.h"
#include "system_allocator.h"

namespace xynq {

// Grow-only. Not thread-safe.
// Use for short-lived data with clear life-time.
// Not good for use with dynamic containers, cause reallocations might cause enormous spike in
// memory usage.
class ScratchAllocator final : public BaseAllocator {
public:
    ScratchAllocator(size_t reserve_size = 4096, BaseAllocator *allocator = &SystemAllocator::Shared());
    ScratchAllocator(ScratchAllocator &&other);
    ScratchAllocator& operator=(ScratchAllocator &&other);
    ~ScratchAllocator() final;

    // Allocator interface implementation.
    void *Alloc(size_t size) final;
    void *AllocAligned(size_t alignment, size_t size) final;
    // Note: this allocator is not returning memory to the system when free is called.
    void Free(void *mem) final;

    // Purges all the memory currently allocated.
    // Note: this will not really return memory to the system,
    // will only clean internal buffers.
    void Purge();

    // Handler is callable(ScratchAllocator &).
    // All allocations done inside the handler will be purged upun exit.
    template<class HandlerType>
    inline void Scoped(HandlerType handler);

    // Number of bytes currently allocated.
    // Mostly used for statistics/debugging/testing.
    // Linear time on number of chunks - should not be used in runtime.
    size_t SizeAllocated() const;

    // Disable assignemnts & copies.
    ScratchAllocator(const ScratchAllocator &) = delete;
    ScratchAllocator& operator=(const ScratchAllocator &) = delete;
private:
    BaseAllocator *real_allocator_ = nullptr; // What actually allocates memory from the system.
    Arena::Ptr head_arena_ = nullptr;
    Arena::Ptr cur_arena_ = nullptr;
    uint8_t *cur_ptr_ = nullptr;
    size_t total_size_ = 0;
};

template<class HandlerType>
void ScratchAllocator::Scoped(HandlerType handler) {
    Arena::Ptr prev_arena = cur_arena_;
    uint8_t *prev_ptr = cur_ptr_;

    handler(*this);

    cur_arena_ = prev_arena;
    cur_ptr_ = prev_ptr;
}

} // xynq
