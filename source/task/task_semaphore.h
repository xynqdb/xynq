#pragma once

#include "base/platform_def.h"

#include <atomic>

namespace xynq {

class TaskContext;

// Supports two operations: Wait and Signal.
// On ctor sets internal counter to the value passed in ctor.
// Every time Signal is called - decrements counter.
// Wait blocks until counter is zero.
class TaskSemaphore {
public:
    TaskSemaphore(unsigned count);

    void Signal();

    void Wait(TaskContext &tc);
private:
    alignas(k_cache_line_size) std::atomic<unsigned> count_;
};

} // xynq