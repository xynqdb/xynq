#pragma once

#include "base/assert.h"

#include <functional>
#include <unistd.h>

namespace xynq {

static const size_t k_hook_default_max_handlers = 4;

// Allows binding max_handlers callbacks with arguments and invoking them in order.
// When Invoke() is called args are passed to all the registered callbacks.
// Handler functions are called in order of how they were added.
template<size_t max_handlers, class...Args>
class BaseHook {
public:
    using HandlerFunc = std::function<void(Args...)>;

    ~BaseHook() {
        Clear();
    }

    // Adds a callback.
    void Add(HandlerFunc &&func) {
        XYAssert(funcs_size_ < max_handlers);
        funcs_[funcs_size_++] = std::move(func);
    }

    // Clears all callbacks.
    void Clear() {
        while (funcs_size_--) {
            funcs_[funcs_size_] = nullptr;
        }
        funcs_size_ = 0;
    }

    // Executes all the added callbacks.
    void Invoke(Args...args) {
        for (size_t i = 0; i < funcs_size_; ++i) {
            funcs_[i](std::forward<Args>(args)...);
        }
    }

private:
    HandlerFunc funcs_[max_handlers];
    size_t funcs_size_ = 0;
};

template<class...Args>
class Hook : public BaseHook<k_hook_default_max_handlers, Args...> {
};

} // xynq
