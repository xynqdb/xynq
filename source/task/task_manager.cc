#include "base/assert.h"
#include "base/system_allocator.h"

#include "task_manager.h"
#include "worker_thread.h"

#include "os/utils.h"

using namespace xynq;
using namespace xynq::detail;

DefineTaggedLog(Task)

TaskManager::TaskManager(Dep<Log> log,
                       size_t max_events_at_once,
                       size_t num_threads,
                       bool pin_threads,
                       bool takeover_current_thread)
    : log_(log)
    , num_threads_(num_threads)
    , pin_threads_(pin_threads)
    , takeover_current_thread_(takeover_current_thread) {
    if (num_threads == kNumThreadsAutoDetect) {
        num_threads_ = platform::NumCores();
        XYTaskInfo(log, "Auto detecting number of threads to use: ", num_threads_);
    }
    event_queue_ = CreateObject<EventQueue>(SystemAllocator::Shared(), log, max_events_at_once, num_threads_);
    XYAssert(num_threads_ >= 1); // Cannot exeute any tasks if there are no threads.
}

void TaskManager::Run() {
    XYAssert(!IsRunning()); // Already running
    XYAssert(threads_ == nullptr); // Should not be initialized yet.
    XYAssert(num_threads_ > 0);

    hooks.before_start.Invoke(num_threads_);

    threads_ = (WorkerThread *)SystemAllocator::Shared().Alloc(sizeof(WorkerThread) * num_threads_);

    size_t index = 1;
    for (WorkerThread *thread_mem = threads_ + 1; thread_mem != threads_ + num_threads_; ++thread_mem) {
        WorkerThread *thread = new(thread_mem) WorkerThread(*this, index, log_, event_queue_, pin_threads_, false, {});
        hooks.before_thread_start.Invoke(index, log_, thread->UserData());
        thread->Start();
        ++index;
    }

    // TODO: round robin spread entrypoints.

    WorkerThread *thread = new(threads_) WorkerThread(*this, 0, log_, event_queue_, pin_threads_, takeover_current_thread_,
                                                      MutSpan<TaskTuple>{entrypoints_.data(), entrypoints_.size()});
    entrypoints_.clear();
    hooks.before_thread_start.Invoke(index, log_, thread->UserData());
    thread->Start();
}

void TaskManager::Stop() {
    XYAssert(IsRunning()); // Already running
    for (size_t i = 0; i < num_threads_; ++i) {
        threads_[i].DelayedStop();
    }

    event_queue_->InterruptAll();
}

bool TaskManager::IsRunning() const {
    return threads_ != nullptr;
}

void TaskManager::StopInternal() {
    size_t num_running;
    do {
        num_running = 0;
        for (size_t i = 0; i < num_threads_; ++i) {
            WorkerThread &thread = threads_[i];

            if (!thread.IsFinished()) {
                ++num_running;
                thread.DelayedStop();
            }
        }

        event_queue_->InterruptAll();
        std::this_thread::yield(); // they all should stop fairly quickly.
    } while (num_running > 0);
}

TaskManager::~TaskManager() {
    if (threads_ != nullptr) {
        StopInternal();
        for (WorkerThread *thread = threads_; thread != threads_ + num_threads_; ++thread) {
            thread->~WorkerThread(); // Will join
        }
        SystemAllocator::Shared().Free(threads_);
    }
}
