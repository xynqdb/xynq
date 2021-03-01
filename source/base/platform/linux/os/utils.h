#pragma once

#include <cstdint>

namespace xynq {
namespace platform {

// Number of hardware cores available on the machine.
unsigned NumCores();

// Pins calling thread to a core.
// Core index should be in [0, NumCores() - 1]
bool PinThread(unsigned core_index);

// Returns platform-specific numeric process id.
uint64_t GetPid();

// Initializes internal platform-specific globals.
// Should be called once per process.
// Guarantees to call exit_handler once. Logger will not be called after exit happened.
void InitExitHandler(void(*exit_handler_)(int exit_code),
                     void(*logger_)(const char *str));

} // platform
} // xynq
