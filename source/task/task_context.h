#pragma once

#include "task.h"
#include "worker_thread.h"
#include "task_manager.h"

#include "base/dep.h"
#include "base/log.h"
#include "event/event.h"
#include "event/eventqueue.h"

#include <cstdint>
#include <utility>

namespace xynq {

// Passed into task on execution.
// Provides interface for tasks to fork to other tasks etc.
class TaskContext : public Task {
public:
    // Log for logging from inside of the task.
    Dep<Log> Log();

    // Shared event queue.
    Dep<EventQueue> EventQueue();

    // Returns unique index assigned to a thread.
    size_t ThreadIndex() const;

    // Retrieves shared user data with type T.
    template<class T>
    inline T &UserData();

    // Will stop task and shut down task manager.
    // Queued tasks might not finish and thereforce will not give up on
    // resource they might hold. (ie. opened files)
    void Exit();

    // Yields task and lets other tasks to run for a while.
    void Yield();

    // Queues new task for execution.
    // Returns immediately - does not wait for task completion.
    template<class T, class...Args>
    inline void PerformAsync(Args&&...args);

    // Performs task immediately. Blocks calling task until this task is finished.
    template<class T, class...Args>
    inline void PerformSync(Args&&...args);

    // Suspend task until the event is thrown.
    void WaitEvent(EventSource *event_source, unsigned event_flags);
};

static_assert(sizeof(Task) == sizeof(TaskContext),
    "TaskContext should not have any data members. It's a Task wrapper used for task public api.");
////////////////////////////////////////////////////////////



template<class T, class...Args>
void TaskContext::PerformAsync(Args&&...args) {
    XYAssert(thread_ != nullptr);

    detail::TaskTuple task(detail::TaskCtorWrap<T>(), std::forward<Args>(args)...);
    thread_->QueueTask(std::move(task));

    //static const size_t kQueuedThreshold = 1;
    //bool was_under_threshold = thread_->exec_.tasks_queued_++ < kQueuedThreshold;

    // This is quite expensive call, so probably should move to
    // a single place instead of calling on every task.
    thread_->events_->InterruptAll();
}

template<class T, class...Args>
void TaskContext::PerformSync(Args&&...args) {
    T::exec(this, std::forward<Args>(args)...);
}

template<class T>
T &TaskContext::UserData() {
    return *(T *)&thread_->UserData();
}

}