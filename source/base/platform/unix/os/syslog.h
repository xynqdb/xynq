#pragma once

#include "base/assert.h"
#include "base/span.h"

#include <syslog.h>

namespace xynq {
namespace platform {

const int syslog_levels[] = {
    -1,
    LOG_ALERT,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

// Syslog implementation for Apple platforms, and Linux.
class Syslog {
public:
    ~Syslog() {
        if (started_) {
            Stop();
        }
    }

    void Start() {
        XYAssert(!started_);
        ::openlog(nullptr, LOG_PID | LOG_NDELAY | LOG_NOWAIT, 1);
        started_ = true;
    }

    void Stop() {
        XYAssert(started_);
        ::closelog();
        started_ = false;
    }

    // Priority: [1, 4] - 1 lowest, 4 - highest
    void Print(int priority, StrSpan str) {
        XYAssert(started_);
        XYAssert(priority >= 1 && priority <= 4);
        ::syslog(syslog_levels[priority], "%.*s", (int)str.Size(), str.Data());
    }

private:
    bool started_ = false;
};

} // platform
} // xynq
