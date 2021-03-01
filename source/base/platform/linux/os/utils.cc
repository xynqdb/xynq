#include "os/utils.h"

#include <thread>
#include <signal.h>
#include <sched.h>
#include <unistd.h>

using namespace xynq;
using namespace xynq::platform;

namespace {

// Exit handler.
std::atomic_flag platform_init_exit = ATOMIC_FLAG_INIT;
std::atomic_flag exit_called = ATOMIC_FLAG_INIT;
void(*exit_logger)(const char *) = nullptr;
void(*exit_handler)(int) = nullptr;

void exit_log(const char *str) {
    if (exit_logger != nullptr && str != nullptr) {
        exit_logger(str);
    }
}

void ExitHandler(int sig) {
    // Prevent duplicated calls.
    if (exit_called.test_and_set()) {
        return;
    }

    switch (sig) {
        case SIGINT:
            exit_log("Caught SIGINT. Will exit.");
            break;

        case SIGTERM:
            exit_log("Caught SIGTERM. Will exit.");
            break;

        default:
            exit_log("Exit called.");
    }

    if (exit_handler != nullptr) {
        exit_handler(0);
    } else {
        kill(getpid(), SIGKILL);
    }
}
} // anon


unsigned xynq::platform::NumCores() {
    return std::thread::hardware_concurrency();
}

bool xynq::platform::PinThread(unsigned core_index) {
    if (core_index >= NumCores()) {
        return false;
    }

    cpu_set_t cpus;
    CPU_ZERO(&cpus);

    CPU_SET(core_index, &cpus);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpus) == 0;
}

uint64_t xynq::platform::GetPid() {
    return getpid();
}

void xynq::platform::InitExitHandler(void(*exit_handler_)(int),
                                     void(*logger_)(const char *str)) {

    // Guard multiple calls, ie. multiple xynqs running in the same process -
    // and each calling platform::Init() which should only be called once per process.
    if (platform_init_exit.test_and_set()) {
        // Just crash if that happened.
        // Cannot continue - only one exit handler is supported.
        *(volatile int *)0 = 1;
        return;
    }

    exit_logger = logger_;
    exit_handler = exit_handler_;

    exit_logger("Setting up exit handler for Linux");
    signal(SIGINT, ExitHandler);
    signal(SIGTERM, ExitHandler);
}
