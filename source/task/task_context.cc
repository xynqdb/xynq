#include "task_context.h"

using namespace xynq;

DefineTaggedLog(Task)

Dep<Log> TaskContext::Log() {
    XYAssert(thread_ != nullptr);
    return thread_->log_;
}

Dep<EventQueue> TaskContext::EventQueue() {
    XYAssert(thread_ != nullptr);
    return thread_->events_;
}

void TaskContext::WaitEvent(EventSource *event_source, unsigned event_flags) {
    XYAssert(event_source != nullptr);
    XYAssert(thread_ != nullptr);

    WorkerThread::ExecutionState &state = thread_->exec_;

    // We need to add event for wait after Suspend() otherwise race is possible
    // when event will get unblocked on another thread but the task is not suspended.
    state.pending_event_ = event_source;
    state.pending_event_flags_ = event_flags; // force exactly once for all waitable events.
    state.has_pending_event_ = true;
    state.current_task_->Suspend(); // Will switch fiber here and execution will stop until event
}

size_t TaskContext::ThreadIndex() const {
    XYAssert(thread_ != nullptr);
    return thread_->index_;
}

void TaskContext::Exit() {
    XYAssert(thread_ != nullptr);

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    XYTaskInfo(Log(), "Requested exit: ", this->debug_name_);
#endif
    WorkerThread::ExecutionState &state = thread_->exec_;
    thread_->DeferredExit();
    state.current_task_->Suspend();
}

void TaskContext::Yield() {
    XYAssert(thread_ != nullptr);

    WorkerThread::ExecutionState &state = thread_->exec_;
    state.yield_ = true;
    state.current_task_->Suspend();
}