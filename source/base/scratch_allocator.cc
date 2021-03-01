#include "scratch_allocator.h"

#include <algorithm>

using namespace xynq;

ScratchAllocator::ScratchAllocator(size_t reserve_size, BaseAllocator *allocator)
    : real_allocator_(allocator) {

    if (reserve_size > 0) {
        head_arena_ = Arena::Create(real_allocator_, reserve_size, alignof(max_align_t));
        total_size_ = head_arena_->Size();
        cur_arena_ = head_arena_;
        cur_ptr_ = head_arena_->Begin();
    }
}

ScratchAllocator::ScratchAllocator(ScratchAllocator &&other) {
    *this = std::move(other);
}

ScratchAllocator& ScratchAllocator::operator=(ScratchAllocator &&other) {
    std::swap(real_allocator_, other.real_allocator_);
    std::swap(head_arena_, other.head_arena_);
    std::swap(total_size_, other.total_size_);
    std::swap(cur_arena_, other.cur_arena_);
    std::swap(cur_ptr_, other.cur_ptr_);
    return *this;
}

ScratchAllocator::~ScratchAllocator() {
    Arena::Destroy(head_arena_);
}

void *ScratchAllocator::Alloc(size_t size) {
    return AllocAligned(alignof(max_align_t), size);
}

void *ScratchAllocator::AllocAligned(size_t alignment, size_t size) {
    XYAssert(detail::IsValidAlignment(alignment));
    XYAssert(cur_ptr_ != nullptr);
    XYAssert(cur_arena_ != nullptr);

    uint8_t *ptr_aligned = detail::AlignPointer(cur_ptr_, alignment);
    while (ptr_aligned + size > cur_arena_->End()) {
        if (cur_arena_->next == nullptr) {
            cur_arena_->next = Arena::Create(real_allocator_, std::max(total_size_ * 2, size), alignment);
            XYAssert(cur_arena_->next != nullptr);
            cur_arena_ = cur_arena_->next;
            cur_ptr_ = cur_arena_->Begin();
            total_size_ += cur_arena_->Size();
            ptr_aligned = cur_ptr_;
            break;
        } else {
            cur_arena_ = cur_arena_->next;
            cur_ptr_ = cur_arena_->Begin();
            ptr_aligned = detail::AlignPointer(cur_ptr_, alignment);
        }
    }

    XYAssert(ptr_aligned + size <= cur_arena_->End());
    cur_ptr_ = ptr_aligned + size;
    return ptr_aligned;
}

void ScratchAllocator::Free(void */*mem*/) {
    // Doing nothing, real cleanup happens when allocator is destroyed.
    // Memory is reused after allocator is Purge()'ed.
}

void ScratchAllocator::Purge() {
    cur_arena_ = head_arena_;
    cur_ptr_ = head_arena_->Begin();
}

size_t ScratchAllocator::SizeAllocated() const {
    Arena::Ptr cur = head_arena_;
    size_t sz = 0;
    while (cur != nullptr && cur_arena_ != cur) {
        sz += cur->Size();
        cur = cur->next;
    }

    sz += cur_ptr_ - cur_arena_->Begin();
    return sz;
}