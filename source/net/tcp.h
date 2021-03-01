#pragma once

#include "base/dep.h"
#include "base/maybe.h"
#include "base/span.h"
#include "base/stream.h"
#include "base/scratch_allocator.h"
#include "containers/str.h"
#include "containers/vec.h"
#include "task/task_manager.h"

namespace xynq {

// Called when new Tcp connection established.
// name is human readable stream identifier - ie. "tcp://127.0.0.1:5263".
// io_stream allows reading/writing from the connection.
using TcpNewStreamHandler = void(*)(TaskContext *, StrSpan name, InOutStream *io_stream);

// Keep alive settings.
struct TcpKeepAlive {
    bool enable = false;
    int idle_sec = 2;
    int interval_sec = 10;
    int num_probes = 8;
};

// Tcp configuration parameters.
struct TcpParameters {
    // Maximum length to which the queue of pending connections may grow.
    int listen_backlog = 1024;

    // Allows other process to bind to the same port.
    // See SO_REUSEADDR linux docs for more details.
    bool reuse_addr = false;

    // Keep-alive settings.
    TcpKeepAlive keep_alive;
};

// Sockets-based tcp streams implementation.
// Supports both IPv4 and 6.
class TcpManager {
public:
    static Maybe<TcpManager>
    Create(Dep<Log> log,
           Dep<TaskManager> task_manager,
           const TcpParameters &parameters,
           Span<CStrSpan> bind_addrs,
           TcpNewStreamHandler new_stream_handler);
private:
    using Address = std::pair<Str, int>; // Ip and port.
    Vec<Address> bind_addrs_;
};

} // xynq