#include "task_semaphore.h"
#include "task_context.h"

#include "base/assert.h"

using namespace xynq;


TaskSemaphore::TaskSemaphore(unsigned count) {
    count_.store(count, std::memory_order_release);
}

void TaskSemaphore::Signal() {
    XYAssert(count_ > 0);
    count_.fetch_sub(1, std::memory_order_release);
}

void TaskSemaphore::Wait(TaskContext &tc) {
    unsigned value = 0;
    while (!count_.compare_exchange_weak(value, 0, std::memory_order_acq_rel,  std::memory_order_relaxed)) {
        tc.Yield();
        value = 0;
    }
}
