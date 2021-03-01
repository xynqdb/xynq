#include "task.h"

using namespace xynq;

void Task::Execute(WorkerThread *from_thread, platform::ExecContext &prev, TaskArgStorage *args) {
    XYAssert(from_thread != nullptr);
    XYAssert(func_);
    XYAssert(state_ == TaskState::NotStarted);

    thread_ = from_thread;
    state_ = TaskState::Executing;
#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
    DebugFillStack();
#endif
    context_.Execute(prev, stack_buf_, sizeof(stack_buf_), func_, (TaskContext *)this, args);
#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
    DebugCheckStack();
#endif
}

void Task::Suspend() {
    XYAssert(thread_ != nullptr);
    XYAssert(state_ == TaskState::Executing);

    state_ = TaskState::Suspended;
    context_.Suspend();
}

void Task::Resume(WorkerThread *from_thread, platform::ExecContext &prev) {
    XYAssert(from_thread != nullptr);
    XYAssert(state_ == TaskState::Suspended);

    thread_ = from_thread;
    state_ = TaskState::Executing;
    context_.Resume(prev);
}

#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
void Task::DebugFillStack() {
    uint32_t *buf_cur = (uint32_t *)&stack_buf_[0];
    uint32_t *buf_end = buf_cur + (StackSize() >> 2);
    while (buf_cur != buf_end) {
        *buf_cur++ = 0xC1D2E3F4;
    }
}

void Task::DebugCheckStack() {
    uint32_t *buf_cur = (uint32_t *)&stack_buf_[0];
    uint32_t *buf_end = buf_cur + (StackSize() >> 2);

    while (buf_cur != buf_end && *buf_cur == 0xC1D2E3F4) {
        ++buf_cur;
    }
    used_stack_size_ = sizeof(uint32_t) * (buf_end - buf_cur);
}
#endif