#pragma once

#include "base/assert.h"
#include "os/exec_context.h"

#include <tuple>
#include <type_traits>

#if defined(XYNQ_DEBUG)
    #define XYNQ_TASK_HAS_DEBUGNAME
    #define XYNQ_TASK_TRACK_STACK_SIZE
#endif

namespace xynq {

class TaskContext;
class WorkerThread;

static constexpr size_t kTaskMaxArgsSize = 128;
using TaskArgStorage = typename std::aligned_storage<kTaskMaxArgsSize>::type;
using TaskFunc = void(*)(TaskContext *, TaskArgStorage *);

static constexpr size_t kThreaduserDataSize = 128;
using ThreadUserDataStorage = typename std::aligned_storage<kThreaduserDataSize>::type;

enum class TaskState {
    NotStarted,
    Executing,
    Suspended
};

// Tasks are defined as POD structs with some static fields:
//   exec:       static function that must take TaskContext as the first argument.
//   stack_size: min stack size needed for the task.
//   debug_name: name used for task manager logging.
// Example:
// struct HelloTask {
//      static constexpr unsigned stack_size = 512;
//      static constexpr char *debug_name = "Hello";
//      static constexpr auto exec = [](TaskContext *context, const char *arg) {
//          printf("Hello %s\n", arg);
//      }
// };
// Can also inhert from TaskDefaults to initialize some fields to default:
struct TaskDefaults {
    // Debug name only used for some debug logging when extra
    // debugging features for fibers are enabled.
    static constexpr auto debug_name = "";

    // Min stack size required for the task.
    static constexpr unsigned stack_size = 1024;
};

// Fiber-based task.
class Task {
public:
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    const char *debug_name_ = TaskDefaults::debug_name;
#endif
#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
    size_t used_stack_size_ = (size_t)-1;
#endif

    // Starts execution of a task.
    // Passes args into task's exec function.
    void Execute(WorkerThread *from_thread, platform::ExecContext &prev, TaskArgStorage *args);

    // Called from within the task.
    // Suspends execution and switches back to previous execution context.
    void Suspend();

    // Called from outside the task.
    // Resumes execution of the previously suspended task - switches execution context
    // back to task context.
    void Resume(WorkerThread *from_thread, platform::ExecContext &prev);

    // Binds a function to this task.
    template<class T>
    inline void Bind(T &&func) { func_ = std::forward<T>(func); }

    // Stack size of this task.
    inline size_t StackSize() const { return sizeof(stack_buf_); }

    // Execution context of this task.
    inline platform::ExecContext &ExecContext() { return context_; }

    // Task state.
    inline TaskState State() const { return state_; }

    // Thread that this fiber is currently running on.
    // Only valid for tasks that are in the executing state.
    inline WorkerThread *Thread() const {
        XYAssert(state_ == TaskState::Executing);
        return thread_;
    }

protected:
    platform::ExecContext context_;
    TaskFunc func_;
    TaskState state_ = TaskState::NotStarted;
    WorkerThread *thread_ = nullptr;

    alignas(alignof(uint32_t)) char stack_buf_[16384];
    static_assert((sizeof(stack_buf_) & 3u) == 0, "Stack size should be divisible by 4.");

#if defined(XYNQ_TASK_TRACK_STACK_SIZE)
    void DebugFillStack();
    void DebugCheckStack();
#endif
};

using TaskPtr = Task*;

namespace detail {

// Helper to always enforce correct task constructor.
template<class T> struct TaskCtorWrap{};

// Internal task representation.
struct TaskTuple {
    TaskPtr task_ = nullptr;
    TaskArgStorage args_store_;
    TaskFunc func_;
    unsigned stack_size_ = 0;

#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    const char *debug_name_ = nullptr;
#endif

    inline TaskTuple() = default;
    inline TaskTuple(const TaskTuple &) = default;
    inline TaskTuple(TaskTuple &&) = default;
    inline TaskTuple &operator=(const TaskTuple &) = default;
    inline TaskTuple &operator=(TaskTuple &&) = default;

    // Initializes with task.
    template<class TaskType, class...Args>
    inline explicit TaskTuple(TaskCtorWrap<TaskType>, Args...args);

    // Initializes with existing task.
    // This usually happens when task was blocked waiting on some
    // operation to complete and then resumed.
    inline explicit TaskTuple(TaskPtr task);
};
////////////////////////////////////////////////////////////


template<class TaskType, class...Args>
TaskTuple::TaskTuple(TaskCtorWrap<TaskType>, Args...args) {
    using T = std::tuple<TaskContext *, Args...>;
    static_assert(sizeof(T) <= kTaskMaxArgsSize, "Maximum task arguments size must be <= 128 bytes");

    // Passing nullptr as context first.
    // it will be set at the task execution.
    new (&args_store_) T{nullptr, std::forward<Args>(args)...};
    func_ = [](TaskContext *tc, TaskArgStorage *arg_buf) {
        T *args_tuple_ptr = (T *)arg_buf;
        std::get<0>(*args_tuple_ptr) = tc; // Assigning right context.

        // Invoke Task::exec with arguments restored from the buffer
        std::apply(TaskType::exec, *args_tuple_ptr);
    };

    stack_size_ = TaskType::stack_size;
#if defined(XYNQ_TASK_HAS_DEBUGNAME)
    debug_name_ = TaskType::debug_name;
#endif
}

TaskTuple::TaskTuple(TaskPtr task)
    : task_(task) {
}

} // detail

} // xynq