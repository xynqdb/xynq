#pragma once

#include "allocator.h"
#include "dep.h"

namespace xynq {

// Global thread-safe allocator, pretty much the same as malloc/free.
// The slowest allocator there is.
// Use for non-realtime data mostly. Or for rare allocations.
class SystemAllocator final : public BaseAllocator {
public:

    ~SystemAllocator() final;

    // Allocator interface.
    void *Alloc(size_t size) final;
    void *AllocAligned(size_t alignment, size_t size) final;
    void Free(void *mem) final;

    // Global shared instance of system allocator.
    static void Initialize();
    static void Shutdown();
    static SystemAllocator &Shared();
    static Dependable<SystemAllocator*> &SharedDep();
private:
};

} // xynq
