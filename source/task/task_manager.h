#pragma once

#include "task.h"
#include "task_context.h"

#include "base/hook.h"
#include "base/log.h"
#include "containers/vec.h"
#include "event/eventqueue.h"

namespace xynq {

class WorkerThread;

static const size_t kNumThreadsAutoDetect = ~size_t();

class TaskManager {
    friend class WorkerThread;
public:
    struct {
        // Called once before task manager starts on a thread where Run() is called.
        Hook<size_t /* num_threads*/> before_start;

        // Called from every worker thread before it goes into task loop.
        Hook<size_t /*thread_index */, Dep<Log> /*log*/, ThreadUserDataStorage & /*user_data*/> before_thread_start;

        // Called for every thread after it exited task loop but before it's destroyed.
        Hook<size_t /*thread_index */, ThreadUserDataStorage & /*user_data*/> after_thread_stop;
    } hooks;


    TaskManager(Dep<Log> log,
               size_t max_events_at_once,
               size_t num_threads,
               bool pin_threads,
               bool takeover_current_thread);
    ~TaskManager();

    // Runs task threads and blocks current thread.
    void Run();

    // Asynchronously stops all threads. Queued tasks might never finish.
    // Can be called from any thread.
    void Stop();

    // Number of threads in the pool.
    inline size_t NumThreads() const { return num_threads_; }

    // Add task with arguments.
    template<class T, class...Args>
    inline void AddEntryPoint(Args...args);
private:
    Dep<Log> log_;
    Dependable<EventQueue*> event_queue_;
    Vec<detail::TaskTuple> entrypoints_;
    WorkerThread *threads_ = nullptr;
    size_t num_threads_ = 0;
    bool pin_threads_ = true;
    bool takeover_current_thread_ = false;

    bool IsRunning() const;

    // Will stop all threads and block until they all exit.
    void StopInternal();
};
////////////////////////////////////////////////////////////


// Implementation
template<class T, class...Args>
void TaskManager::AddEntryPoint(Args...args) {
    XYAssert(!IsRunning()); // Only allow entry points be added before threads started.

    entrypoints_.emplace_back(detail::TaskCtorWrap<T>(), std::forward<Args>(args)...);
}

} // xynq