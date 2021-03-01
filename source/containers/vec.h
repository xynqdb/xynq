#pragma once

#include <vector>
#include "base/allocator.h"
#include "base/system_allocator.h"
#include "base/scratch_allocator.h"

namespace xynq {

template<class T>
using VecBaseType = std::vector<T, StdAlloc<T, BaseAllocator>>;

template<class T>
using ScratchVecBaseType = std::vector<T, StdAlloc<T, ScratchAllocator>>;


// Dynamically allocated array. Equivalent of std::vector.
template<class T>
class Vec final : public VecBaseType<T> {
public:
    Vec()
        : VecBaseType<T>{&SystemAllocator::Shared()}
    {}

    Vec(const Vec<T> &other)
        : VecBaseType<T>(other)
    {}

    Vec(Vec<T> &&other)
        : VecBaseType<T>(std::forward<Vec<T>>(other))
    {}

    Vec(std::initializer_list<T> init, BaseAllocator *allocator = &SystemAllocator::Shared())
        : VecBaseType<T>(init, allocator)
    {}

    Vec(BaseAllocator *allocator)
        : VecBaseType<T>(allocator)
    {}

    using VecBaseType<T>::operator=;
};

template<class T>
class ScratchVec final : public ScratchVecBaseType<T> {
public:
    explicit ScratchVec(ScratchAllocator *allocator)
        : ScratchVecBaseType<T>{allocator}
    {
    }

    ScratchVec(const ScratchVec &other) = delete;
    using ScratchVecBaseType<T>::operator=;
};

} // xynq
