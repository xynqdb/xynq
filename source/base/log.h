#pragma once

#include "base/either.h"
#include "base/maybe.h"
#include "base/span.h"
#include "base/str_builder.h"
#include "base/str_build_types.h"
#include "os/syslog.h"

#include <cstdio>


namespace xynq {

using LogBuilder = StrBuilder<2048>;

struct LogFlags {
    enum {
        StdOut = 1 << 0,
        SysLog = 1 << 1,
    };
};

// Logger verbosity level.
enum class LogLevel {
    None    =  0, // Will output nothing.
    Error   =  1,
    Warning =  2,
    Info    =  3,
    Verbose =  4,
};

// Log initialization failures.
enum class LogFailure {
    // Failed to open log file for write. Might not have permissions.
    CannotOpenFile,
    // Invalid logging level specified.
    InvalidLevel,
};

// Returns log level from its name.
Either<LogFailure, LogLevel> LogLevelFromString(CStrSpan level_name);


// Logger.
class Log {
public:
    // Creates new logger.
    // If file is not specified - will not log into file.
    static Either<LogFailure, Log> Create(LogLevel level, unsigned flags, Maybe<CStrSpan> log_file);

    // Creates new log with prefix added to the existing Log.
    Log(const Log &another, CStrSpan prefix);
    Log(Log &&other);
    ~Log();

    LogLevel Level() const { return level_; }
    bool ShouldLog(LogLevel level) const { return (int)level <= (int)level_; }

    template<class...Args>
    void Error(Args&&...args);

    template<class...Args>
    void Warning(Args&&...args);

    template<class...Args>
    void Info(Args&&...args);

    template<class...Args>
    void Verbose(Args&&...args);
private:
    LogLevel level_ = LogLevel::Info;
    FILE *log_fp_ = nullptr;
    bool own_file_ = false;
    bool is_stdout_ = false;
    bool is_syslog_ = false;
    platform::Syslog syslog_;
    char prefix_[64];
    size_t prefix_size_ = 0;

    Log(LogLevel level, unsigned flags);
    bool HasFile() const;
    bool StartLogFile(const char *log_file);
    void AppendLogFile(StrSpan str);
    void CloseLogFile();
    void AppendPrefix(const char *str, size_t size);
    LogBuilder &BeginThreadSafeLog();
    void EndThreadSafeLog(LogBuilder &log_builder, LogLevel level);
    template<class T>
    inline void BuildLog(LogLevel level, T func);
};

template<class T>
void Log::BuildLog(LogLevel level, T func) {
    LogBuilder &log_builder = BeginThreadSafeLog();
    func(log_builder);
    EndThreadSafeLog(log_builder, level);
}

template<class...Args>
void Log::Error(Args&&...args) {
    BuildLog(LogLevel::Error, [&](LogBuilder &builder) {
        builder.Append("[error] ", std::forward<Args>(args)...);
    });
}

template<class...Args>
void Log::Warning(Args&&...args) {
    BuildLog(LogLevel::Warning, [&](LogBuilder &builder) {
        builder.Append("[warning] ", std::forward<Args>(args)...);
    });
}

template<class...Args>
void Log::Info(Args&&...args) {
    BuildLog(LogLevel::Info, [&](LogBuilder &builder) {
        builder.Append(std::forward<Args>(args)...);
    });
}

template<class...Args>
void Log::Verbose(Args&&...args) {
    BuildLog(LogLevel::Verbose, [&](LogBuilder &builder) {
        builder.Append(std::forward<Args>(args)...);
    });
}

} // xynq

// Prints out file and line, for debugging purposes.
#define XYLogControlPoint(log_instance) (log_instance)->Info("CP: ", __FILE__, ':', __LINE__)

// Helper to define simpler to use log per-subsystem log functions.
// Usage:
//      DefineTaggedLog(Hello)
// The above will define helper functions like: XYHelloError, XYHelloWarning,
// XYHelloInfo, XYHelloVerbose. That will call logger with tag "Hello". ie.:
//      XYHelloError(log, "Hello %s", "World"); // Will output error with tag Hello:
//      [Error] <Hello> Hello World
#define DefineTaggedLog(tag) \
namespace {                                                                     \
    template<class LogClass, class...Args>                                      \
    inline static void XY##tag##Error(LogClass &&log, Args&&...args) {          \
        log->Error("<" #tag "> ", std::forward<Args>(args)...);                 \
    }                                                                           \
    template<class LogClass, class...Args>                                      \
    inline static void XY##tag##Warning(LogClass &&log, Args&&...args) {        \
        log->Warning("<" #tag "> ", std::forward<Args>(args)...);               \
    }                                                                           \
    template<class LogClass, class...Args>                                      \
    inline static void XY##tag##Info(LogClass &&log, Args&&...args) {           \
        log->Info("<" #tag "> ", std::forward<Args>(args)...);                  \
    }                                                                           \
    template<class LogClass, class...Args>                                      \
    inline static void XY##tag##Verbose(LogClass &&log, Args&&...args) {        \
        log->Verbose("<" #tag "> ", std::forward<Args>(args)...);               \
    }                                                                           \
}