#pragma once

#include <utility>

#include <ucontext.h>

namespace xynq {
namespace platform {

// Linux ucontext_t based implementation of ExecContext.
class ExecContext {
public:
    inline ExecContext() = default;
    ExecContext(const ExecContext &) = delete;
    ExecContext(ExecContext &&) = delete;
    ExecContext& operator=(const ExecContext &) = delete;
    ExecContext& operator=(ExecContext &&) = delete;

    template<class Func, class...Args>
    inline void Execute(ExecContext &prev, void *stack_buf, size_t stack_size, Func func, Args...args);

    inline void Suspend() {
        swapcontext(&ctx_, prev_);
    }

    inline void Resume(ExecContext &prev) {
        prev_ = &prev.ctx_;
        swapcontext(prev_, &ctx_);
    }

    ucontext_t *prev_ = nullptr;
    ucontext_t ctx_;
};


template<class Func, class...Args>
void ExecContext::Execute(ExecContext &prev, void *stack_buf, size_t stack_size, Func func, Args...args) {
    void (*func_ptr) (ExecContext *, Func, Args...) = [](ExecContext *cur, Func func, Args...args) {
        func(std::forward<Args>(args)...);
        swapcontext(&cur->ctx_, cur->prev_);
    };

    getcontext(&ctx_);
    ctx_.uc_stack.ss_sp = stack_buf;
    ctx_.uc_stack.ss_size = stack_size;
    ctx_.uc_stack.ss_flags = 0;
    ctx_.uc_link = nullptr;
    prev_ = &prev.ctx_;
    makecontext(&ctx_, reinterpret_cast<void (*)()>(func_ptr), 2 + sizeof...(Args), this, func, std::forward<Args>(args)...);
    swapcontext(prev_, &ctx_);
}

} // platform
} // xynq