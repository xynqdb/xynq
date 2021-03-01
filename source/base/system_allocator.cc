#include "system_allocator.h"

#include <stdlib.h>

using namespace xynq;

namespace {
Dependable<SystemAllocator *> shared_instance = nullptr;
} // anon

SystemAllocator &SystemAllocator::Shared() {
    XYAssert(shared_instance.IsValid());
    return shared_instance.Get();
}

Dependable<SystemAllocator*> &SystemAllocator::SharedDep() {
    return shared_instance;
}

void SystemAllocator::Initialize() {
    shared_instance = new SystemAllocator;
}

void SystemAllocator::Shutdown() {
    shared_instance = nullptr;
}

SystemAllocator::~SystemAllocator() {

}

void *SystemAllocator::Alloc(size_t size) {
    return malloc(size);
}

void *SystemAllocator::AllocAligned(size_t alignment, size_t size) {
    XYAssert(detail::IsValidAlignment(alignment));
    return aligned_alloc(alignment, size);
}

void SystemAllocator::Free(void *mem) {
    free(mem);
}