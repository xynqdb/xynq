#include "worker_thread.h"
#include "task_manager.h"

#include "os/utils.h"

using namespace xynq;
using namespace xynq::detail;

DefineTaggedLog(Task)


WorkerThread::WorkerThread(TaskManager &task_manager,
                           size_t index,
                           Dep<Log> log,
                           Dep<EventQueue> events,
                           bool pin_thread,
                           bool take_current_thread,
                           MutSpan<TaskTuple> entrypoints)
    : running_(true)
    , finished_(false)
    , task_manager_(task_manager)
    , index_(index)
    , log_(Log{*log, StrBuilder<32>(index).MakeCStr()})
    , events_(events)
    , pin_thread_(pin_thread)
    , has_thread_(!take_current_thread)
    , local_task_queue_(1024) {

    for (TaskTuple &task : entrypoints) {
        local_task_queue_.Push(std::move(task));
    }

    if (entrypoints.Size() > 0) {
        events_->InterruptAll();
    }
}

WorkerThread::~WorkerThread() {
    if (has_thread_) {
        this_thread_.join();
    }
}

void WorkerThread::Start() {
    XYAssert(!this_thread_.joinable());

    if (has_thread_) {
        this_thread_ = std::thread(&WorkerThread::ThreadProc, this);
    } else {
        ThreadProc();
    }
}

void WorkerThread::DelayedStop() {
    running_.store(false, std::memory_order_relaxed);
}

void WorkerThread::ThreadProc() {
    id_ = std::this_thread::get_id();

    if (pin_thread_) {
        size_t core_index = index_ % platform::NumCores();
        XYTaskInfo(log_, "Pinning thread to cpu ", core_index);
        if (!platform::PinThread(index_)) {
            XYTaskWarning(log_, "Failed to pin thread: ", index_, " to core ", core_index);
        }
    }


    while (running_.load(std::memory_order_relaxed)) {
        Span<Event> triggered_events = events_->Wait(index_, -1);

        // Process events.
        for (const Event &e : triggered_events) {
            TaskPtr task = static_cast<Task *>(e.UserHandle());
            if (task == nullptr) {
                // Event does not have any fiber bound to it.
                // ie. forced wakeup event.
                continue;
            }

             QueueTask(task);
        }

        // Execute pending tasks.
        TaskTuple task_data;
        while (DequeNextTask(task_data)) {
            if (task_data.task_ == nullptr) { // Not bound with the fiber.
                task_data.task_ = CreateTask(task_data);
            }

            TaskPtr task = task_data.task_;
            XYAssert(task != nullptr);
            XYAssert(task->State() == TaskState::NotStarted
                  || task->State() == TaskState::Suspended);

            platform::ExecContext main_context;
            if (task->State() == TaskState::NotStarted) {
                ExecuteTask(task, &task_data.args_store_, main_context);
            } else if (task->State() == TaskState::Suspended) {
                ResumeTask(task, main_context);
            }
        }
    }

    finished_ = true;
    task_manager_.StopInternal(); // Will block until all threads finish

    task_manager_.hooks.after_thread_stop.Invoke(index_, UserData());
}

void WorkerThread::QueueTask(TaskPtr task) {
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    XYTaskInfo(log_, "Queueing: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME

    TaskTuple t(task);
    local_task_queue_.Push(std::move(t));
}

void WorkerThread::QueueTask(TaskTuple &&task) {
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    XYTaskInfo(log_, "Queueing: ", task.task_ ? task.task_->debug_name_ : task.debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
    local_task_queue_.Push(std::move(task));
}

bool WorkerThread::DequeNextTask(TaskTuple &task) {
    if (local_task_queue_.Pop(task)) {
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
        XYTaskInfo(log_, "Dequeued: ", task.task_ ? task.task_->debug_name_ : task.debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
        return true;
    }

    for (size_t i = index_; i < index_ + task_manager_.num_threads_; ++i) {
        WorkerThread &thread = task_manager_.threads_[i % task_manager_.num_threads_];

        if (thread.local_task_queue_.Pop(task)) {
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
            XYTaskInfo(log_, "Dequeued: ", task.task_ ? task.task_->debug_name_ : task.debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
            return true;
        }
    }

    return false;
}

TaskPtr WorkerThread::CreateTask(const detail::TaskTuple &task_data) {
    // TEMP: should be pooled. For now just create new task every time.
    TaskPtr task = new Task;
    task->Bind(task_data.func_);

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    task->debug_name_ = task_data.debug_name_;
    XYTaskInfo(log_, "Created: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
    return task;
}

void WorkerThread::DestroyTask(TaskPtr task) {
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    XYTaskInfo(log_, "Destroying: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
    // TEMP: should be pooled. For now just destroy the task every time.
    delete task;
}

void WorkerThread::PreTask(TaskPtr task) {
    XYAssert(task != nullptr);
    XYAssert(exec_.current_task_ == nullptr);

    exec_.tasks_queued_ = 0;
    exec_.current_task_ = task;

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
        XYTaskInfo(log_, "Will start: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
}

void WorkerThread::PostTask(TaskPtr task) {

#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
    int stack_load_factor = (int)(float(task->used_stack_size_) / float(task->StackSize()) * 100.f);
    if (stack_load_factor >= 85.f) { // if we have high load factor -> print out error
        XYTaskError(log_, "Detected INSUFFICIENT or highly loaded stack for the task '", task->debug_name_, "': ",
                          task->used_stack_size_, " bytes (stack_size=",
                          task->StackSize(), ", load=", stack_load_factor, "%)");
    } else if (stack_load_factor >= 75.f) {
        XYTaskWarning(log_, "Detected high stack load for '", task->debug_name_, "': ",
                            task->used_stack_size_, " bytes (stack_size=", task->StackSize(),
                            ", load=", stack_load_factor, "%)");
    } else {
        XYTaskInfo(log_, "Detected stack size for '", task->debug_name_, "': ",
                         task->used_stack_size_, " bytes (stack_size=", task->StackSize(),
                         ", load=", stack_load_factor, "%)");
    }
#endif // XYNQ_TASK_TRACK_STACK_SIZE

    exec_.main_context_ = nullptr;
    if (exec_.has_pending_event_) {
        XYAssert(task->State() == TaskState::Suspended);
        XYAssert(exec_.pending_event_ != nullptr);
        events_->AddEvent(*exec_.pending_event_, exec_.pending_event_flags_, task);
        exec_.pending_event_ = nullptr;
        exec_.has_pending_event_ = false;

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
        XYTaskInfo(log_, "Will suspend: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
    } else if (exec_.yield_) {
        QueueTask(exec_.current_task_);
        exec_.yield_ = false;

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
        XYTaskInfo(log_, "Yielding: ", task->debug_name_);
#endif // XYNQ_TASK_HAS_DEBUGNAME
    } else { // Destroy
        DestroyTask(exec_.current_task_);
    }

    exec_.current_task_ = nullptr;
}

void WorkerThread::ExecuteTask(TaskPtr task, TaskArgStorage *args, platform::ExecContext &exec_context) {
    PreTask(task);
    exec_.current_task_->Execute(this, exec_context, args);
    PostTask(task);
}

void WorkerThread::ResumeTask(TaskPtr task, platform::ExecContext &exec_context) {
    PreTask(task);
    exec_.current_task_->Resume(this, exec_context);
    PostTask(task);
}

void WorkerThread::DeferredExit() {
    XYTaskInfo(log_, "Exit requested");
    DelayedStop();
}
