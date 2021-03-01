#pragma once

#include "task.h"

#include "base/dep.h"
#include "event/eventqueue.h"
#include "containers/MRSWRing.h"
#include "base/platform_def.h"

#include <atomic>
#include <thread>

namespace xynq {

class TaskManager;
class TaskManager;

class WorkerThread {
    friend class TaskContext;
public:
    WorkerThread(TaskManager &task_manager,
                 size_t index,
                 Dep<Log> log,
                 Dep<EventQueue> events,
                 bool pin_thread,
                 bool take_current_thread,
                 MutSpan<detail::TaskTuple> entrypoints);

    ~WorkerThread();

    // Starts thread.
    void Start();

    // Queues thread for stopping.
    // Does not guarantee that thread will be stopped when this method returns.
    void DelayedStop();

    // Thread function has exited.
    bool IsFinished() const { return finished_; }

    // Platform-specific threadID. Unique identifier of a worker.
    // Only initialized after the thread has started.
    std::thread::id Id() const { return id_; }

    // User-data.
    ThreadUserDataStorage &UserData() { return user_data_; }

    //Dep<Log> Log() { return log_; }
private:
    // Per execution state of a task.
    struct ExecutionState {
        // Task that is currently in progress.
        TaskPtr current_task_ = nullptr;
        // ExecContext of thread.
        platform::ExecContext *main_context_ = nullptr;

        // Event that current task requested to wait on.
        EventSource *pending_event_ = nullptr;
        unsigned pending_event_flags_ = 0;
        bool has_pending_event_ = false;

        bool yield_ = false;

        unsigned tasks_queued_ = 0;
    };

    alignas(k_cache_line_size)
    std::atomic<bool> running_;
    std::atomic<bool> finished_; // ThreadProc has finished.

    TaskManager &task_manager_;
    size_t index_ = 0;
    std::thread::id id_;

    // Per-thread user-data storage.
    ThreadUserDataStorage user_data_;

     // Each thread has its own logging channel that writes index into log.
    Dependable<Log> log_;
    Dep<EventQueue> events_;
    bool pin_thread_ = false;
    bool has_thread_ = false;
    std::thread this_thread_;
    MRSWRing<detail::TaskTuple> local_task_queue_;
    ExecutionState exec_;

    void ThreadProc();

    TaskPtr CreateTask(const detail::TaskTuple &task);
    void DestroyTask(TaskPtr task);
    void QueueTask(TaskPtr task);
    void QueueTask(detail::TaskTuple &&task);
    bool DequeNextTask(detail::TaskTuple &task);

    void PreTask(TaskPtr task);
    void PostTask(TaskPtr task);
    void ExecuteTask(TaskPtr task, TaskArgStorage *args, platform::ExecContext &exec_context);
    void ResumeTask(TaskPtr task, platform::ExecContext &exec_context);

    // Stops all threads.
    // Called from current worker thread.
    void DeferredExit();
};

} // xynq
