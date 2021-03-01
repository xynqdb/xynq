#include "log.h"
#include "assert.h"

#include <cstdarg>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iostream>

#include <sys/time.h>
#include <time.h>
#include <math.h>

using namespace xynq;

namespace {

const char *log_level_names[] = {
    "none",
    "error",
    "warning",
    "info",
    "verbose"
};

} // namespace

Either<LogFailure, LogLevel> xynq::LogLevelFromString(CStrSpan level_name) {
    const size_t sz = sizeof(log_level_names)/sizeof(log_level_names[0]);

    for (size_t i = 0; i < sz; ++i) {
        if (level_name == log_level_names[i]) {
            return static_cast<LogLevel>(i);
        }
    }

    return LogFailure::InvalidLevel;
}

Either<LogFailure, Log> Log::Create(LogLevel level,
                                    unsigned flags,
                                    Maybe<CStrSpan> log_file) {
    Log log{level, flags};
    if (log_file.HasValue() && !log.StartLogFile(log_file.Value().Data())) {
        return LogFailure::CannotOpenFile;
    }

    return std::move(log);
}

Log::Log(LogLevel level, unsigned flags)
    : level_(level) {

    is_stdout_ = (flags & LogFlags::StdOut) != 0;
    is_syslog_ = (flags & LogFlags::SysLog) != 0;

    if (is_syslog_) {
        syslog_.Start();
    }
}

Log::Log(const Log &another, CStrSpan prefix)
    : level_(another.level_)
    , log_fp_(another.log_fp_)
    , own_file_(false)
    , is_stdout_(another.is_stdout_)
    , is_syslog_(another.is_syslog_) {

    memcpy(&prefix_[0], &another.prefix_[0], another.prefix_size_);
    prefix_size_ = another.prefix_size_;

    // append new prefix
    AppendPrefix("[", 1);
    AppendPrefix(prefix.CStr(), prefix.Size());
    AppendPrefix("] ", 2);
}

Log::Log(Log &&another) {
    is_stdout_ = another.is_stdout_;
    is_syslog_ = another.is_syslog_;
    log_fp_ = another.log_fp_;
    own_file_ = another.own_file_;
    level_ = another.level_;
    memcpy(&prefix_[0], &another.prefix_[0], another.prefix_size_);
    prefix_size_ = another.prefix_size_;

    another.own_file_ = false;
    another.log_fp_ = nullptr;
}

Log::~Log() {
    if (is_syslog_) {
        syslog_.Stop();
    }

    CloseLogFile();
}

LogBuilder &Log::BeginThreadSafeLog() {
    thread_local LogBuilder log_builder;
    log_builder.Clear();

    bool is_add_time = is_stdout_ || HasFile();

    if (is_add_time) { // Timestamp.
        std::time_t cur_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        log_builder.Write(32, [&](MutStrSpan buf){
            tm *timeinfo = ::localtime(&cur_time);
            return ::strftime(buf.Data(), buf.Size(), "[%Y-%m-%d %T", timeinfo);
        });

        // Microseconds.
        timeval tv;
        ::gettimeofday(&tv, NULL);
        unsigned usec = (unsigned)tv.tv_usec;
        log_builder.Append('.', usec, "] ");
    }

    log_builder << StrSpan{&prefix_[0], prefix_size_};
    return log_builder;
}

void Log::EndThreadSafeLog(LogBuilder &log_builder, LogLevel level) {
    if (is_syslog_) {
        syslog_.Print((int)level, log_builder.Buffer());
    }

    log_builder.Append('\n');
    StrSpan str = log_builder.Buffer();
    if (is_stdout_) {
        fwrite(str.Data(), 1, str.Size(), stdout);
    }

    AppendLogFile(str);
}

bool Log::HasFile() const {
    return log_fp_ != nullptr;
}

bool Log::StartLogFile(const char *log_file) {
    XYAssert(log_fp_ == nullptr);

    log_fp_ = ::fopen(log_file, "wb");
    own_file_ = true;
    return log_fp_ != nullptr;
}

void Log::CloseLogFile() {
    if (own_file_ && log_fp_ != nullptr) {
        ::fclose(log_fp_);
    }

    log_fp_ = nullptr;
}

void Log::AppendLogFile(StrSpan str) {
    if (log_fp_ == nullptr) {
        return;
    }

    ::fwrite(str.Data(), 1, str.Size(), log_fp_);
}

void Log::AppendPrefix(const char *str, size_t size) {
    XYAssert(str != nullptr);

    size_t size_left = sizeof(prefix_) - prefix_size_;
    size_t new_prefix_size = std::min(size, size_left);
    if (new_prefix_size > 0) {
        memcpy(&prefix_[0] + prefix_size_, str, new_prefix_size);
        prefix_size_ += new_prefix_size;
    }
}
