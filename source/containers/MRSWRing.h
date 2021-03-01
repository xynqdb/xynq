#pragma once

#include <mutex>
#include "containers/vec.h"

namespace xynq {

// Dummy.
template<class T>
class MRSWRing {
public:
    MRSWRing(size_t max_size) {
        data_.resize(max_size);
    }

    bool Push(T &&value) {
        std::lock_guard<std::mutex> l(mutex_);
        if (IsFull()) {
            return false;
        }
        data_[w_++ % data_.size()] = std::move(value);
        return true;
    }

    bool Pop(T &value) {
        std::lock_guard<std::mutex> l(mutex_);
        if (IsEmpty()) {
            return false;
        }

        value = std::move(data_[r_++ % data_.size()]);
        return true;
    }

private:
    std::mutex mutex_;
    Vec<T> data_;
    unsigned r_ = 0;
    unsigned w_ = 0;

    bool IsFull() const {
        return (r_ % data_.size()) == (w_ % data_.size()) && !IsEmpty();
    }

    bool IsEmpty() const {
        return r_ == w_;
    }
};

}