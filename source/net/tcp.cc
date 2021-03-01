#include "tcp.h"

#include "base/defer.h"
#include "base/log.h"
#include "base/stream.h"
#include "task/task.h"
#include "task/task_context.h"

#include <unistd.h>
#include <string.h>
#include <cerrno>
#include <utility>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>


using namespace xynq;

DefineTaggedLog(Tcp);

namespace {

 // 6 is for "tcp://", 8 - is for port, 1 is for zero.
 // Tcp stream name looks like: "tcp://127.0.0.1:3456". Can be IPv6 as well.
const size_t kStreamNameMaxSize = 6 + INET6_ADDRSTRLEN + 8 + 1;

// Checks if error code is a nonblocking socket error to retry.
inline bool IsInProgress(int error_code) {
    return error_code == EAGAIN || error_code == EWOULDBLOCK || error_code == EINPROGRESS;
}


// Sets keep-alive settings on the socket.
bool TcpSetKeepAlive(Log *log, int sock, const TcpKeepAlive &keep_alive) {
    // Turning on
    int opt_enable = (int)keep_alive.enable;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt_enable, sizeof(opt_enable)) < 0) {
        XYTcpError(log, "Failed to set keep-alive option for socket (",
                   errno, ", ",
                   strerror(errno), ')');
        return false;
    }

    if (!keep_alive.enable) {
        return true;
    }

    // Idle time to send keep-alives after that
    int opt_idle = keep_alive.idle_sec;
#if defined(XYNQ_APPLE)
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, &opt_idle, sizeof(opt_idle)) < 0) {
#else
    if (setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &opt_idle, sizeof(opt_idle)) < 0) {
#endif
        XYTcpError(log, "Failed to set keep-alive idle time option for socket (",
                   errno, ", ",
                   strerror(errno), ')');
        return false;
    }

    // Interval between probes
    int opt_interval = keep_alive.interval_sec;
#if defined(XYNQ_APPLE)
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &opt_interval, sizeof(opt_interval)) < 0) {
#else
    if (setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &opt_interval, sizeof(opt_interval)) < 0) {
#endif
        XYTcpError(log, "Failed to set keep-alive interval option for socket (",
                   errno, ", ",
                   strerror(errno), ')');
        return false;
    }

    // Number of probes
    int opt_probes = keep_alive.num_probes;
#if defined(XYNQ_APPLE)
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &opt_probes, sizeof(opt_probes)) < 0) {
#else
    if (setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &opt_probes, sizeof(opt_probes)) < 0) {
#endif
        XYTcpError(log, "Failed to set keep-alive number of probes option for socket (",
                   errno, ", ",
                   strerror(errno), ')');
        return false;
    }

    return true;
}

void TcpEnableReuseAddr(Log *log, int sock) {
    uint32_t enable = 1;
    int err = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    if (err < 0) {
        XYTcpWarning(log, "Failed to set SO_REUSEPORT. (",
                     errno, ", ",
                     strerror(errno), ')');
    }
}

// Returns ip:port from the socket.
// Uses ip_buf to store ip string.
std::pair<CStrSpan, int> SocketGetAddress(int sock, char *ip_buf, int ip_buf_size) {
    bool success = false;
    sockaddr_storage addr_store;
    socklen_t len = sizeof(addr_store);

    int port = 0;
    if (getpeername(sock, (sockaddr*)&addr_store, &len) != -1) {
        if (addr_store.ss_family == AF_INET) { // IPv4
            sockaddr_in *addr4 = (sockaddr_in *)&addr_store;
            port = ntohs(addr4->sin_port);
            success = inet_ntop(AF_INET, &addr4->sin_addr, ip_buf, ip_buf_size) != nullptr;
        } else { // IPv6
            sockaddr_in6 *addr6 = (sockaddr_in6 *)&addr_store;
            port = ntohs(addr6->sin6_port);
            success = inet_ntop(AF_INET6, &addr6->sin6_addr, ip_buf, ip_buf_size) != nullptr;
        }
    }

    if (success) {
        return std::make_pair(CStrSpan{ip_buf}, port);
    } else {
        return std::make_pair(CStrSpan{"n/a"}, 0);
    }
}

// Takes string like 127.0.0.1:325 and returns address and port part of it.
Maybe<std::pair<Str, int>> ParseIPAddress(CStrSpan addr_str) {
    size_t i = addr_str.Size();
    while (i-- > 0) {
        if (addr_str[i] == ':') {
            return std::make_pair<Str, int>(StrSpan{addr_str.Data(), i}, atoi(addr_str.CStr() + i + 1));
        }
    }

    return {};
}

// xynq::Stream over tcp connection.
class TcpStream final : public InOutStream {
public:
    TcpStream(TaskContext &tc, int sock, StrSpan name)
        : tc_(tc)
        , sock_(sock)
        , event_source_(sock)
        , name_(name)
    {}

    ~TcpStream() {
        tc_.EventQueue()->RemoveEvent(event_source_);
    }

    Either<StreamError, size_t> DoRead(MutDataSpan read_buf) override {
        ssize_t received;
        do {
            tc_.WaitEvent(&event_source_, EventFlags::Read | EventFlags::ExactlyOnce);
            received = recv(sock_, (char *)read_buf.Data(), read_buf.Size(), 0);
        } while (received < 0 && IsInProgress(errno));

        if (received == 0) {
            XYTcpInfo(tc_.Log(), "Disconnected: ", name_);
            return StreamError::Closed;
        } else if (received < 0) {
            XYTcpWarning(tc_.Log(), "Socket error on recv (", name_, "). ",
                                    "Disconnecting. Error=", errno, ", ", strerror(errno));
            return StreamError::IOError;
        }

        return static_cast<size_t>(received);
    }

    Either<StreamError, StreamWriteSuccess> DoWrite(DataSpan send_buf) override {
        char *to_send = (char *)send_buf.Data();
        char *to_send_end = to_send + send_buf.Size();

        while (to_send != to_send_end) {
            size_t to_send_size = to_send_end - to_send;
            int sent = send(sock_, to_send, to_send_size, 0);
            if (sent < 0 && IsInProgress(sent)) {
                tc_.WaitEvent(&event_source_, EventFlags::Write | EventFlags::ExactlyOnce);
                continue;
            }

            if (sent < 0) {
                XYTcpInfo(tc_.Log(), "Socket error on send (", name_, "). ",
                                     "Disconnecting. Error=", errno, ", ", strerror(errno));
                return StreamError::IOError;
            }

            to_send += sent;
        }

        return StreamWriteSuccess{};
    }

private:
    TaskContext &tc_;
    int sock_ = 0;
    EventSource event_source_;
    StrSpan name_;
};

} // anon namespace


// Handles single tcp stream.
struct TcpConnectionHandler : public TaskDefaults {
    static constexpr unsigned stack_size = 8 * 1024;
    static constexpr auto debug_name = "TcpConnectionHandler";

    static constexpr auto exec = [](TaskContext *tc, int sock, TcpNewStreamHandler stream_handler) {
        StrBuilder<kStreamNameMaxSize> stream_name{"tcp://"};

        int src_port = 0;
        stream_name.Write(INET6_ADDRSTRLEN + 1, [sock, &src_port](MutStrSpan buf) {
            CStrSpan src_ip;
            std::tie(src_ip, src_port) = SocketGetAddress(sock, buf.Data(), buf.Size());
            return src_ip.Size();
        });
        stream_name.Append(':', src_port);
        XYTcpInfo(tc->Log(), "Starting new stream: ", stream_name.Buffer());

        {
            TcpStream stream{*tc, sock, stream_name.Buffer()};
            XYAssert(stream_handler != nullptr);
            stream_handler(tc, stream_name.Buffer(), &stream);
        }

        // Closng socket.
        close(sock);
        XYTcpVerbose(tc->Log(), "Closed socket for ", stream_name.Buffer());
    };
};


// Accepts incoming tcp connections and spawns new tasks to maintain those connections.
// Supports both IPv4 and IPv6.
struct TcpSocketAccept : public TaskDefaults {
    static const unsigned stack_size = 8 * 1024;
    static constexpr auto debug_name = "TcpSocketAccept";

    static constexpr auto exec = [](TaskContext *tc,
                                    CStrSpan bind_addr,
                                    int bind_port,
                                    TcpNewStreamHandler stream_handler,
                                    const TcpParameters &params) {
        XYTcpInfo(tc->Log(), "Prepare listening on ", bind_addr.CStr(), ':', bind_port);
        // Get socket address to bind to.
        sockaddr_storage addr_store;
        sockaddr *addr = (sockaddr *)&addr_store;
        memset(&addr_store, 0, sizeof(addr_store));
        sockaddr_in *addr4 = (sockaddr_in *)addr;
        addr4->sin_family = AF_INET; // try IPv4 first.
        if (inet_pton(AF_INET, bind_addr.CStr(), &(addr4->sin_addr)) == 1) {
            addr4->sin_port = htons(bind_port);
        } else {
            // Failed to get IPv4 addr. Try IPv6.
            memset(&addr_store, 0, sizeof(addr_store));
            sockaddr_in6 *addr6 = (sockaddr_in6 *)addr;
            addr6->sin6_family = AF_INET6;
            if (inet_pton(AF_INET6, bind_addr.CStr(), &(addr6->sin6_addr)) != 1) {
                XYTcpError(tc->Log(), "Failed to get bind address from '", bind_addr.CStr(), "' (", errno, ", ", strerror(errno), ')');
                return;
            }
            addr6->sin6_port = htons(bind_port);
        }

        // Create socket with a family depending whether bind_addr is ipv4 or 6.
        int accept_socket = socket(addr->sa_family, SOCK_STREAM, 0);
        if (accept_socket < 0) {
            XYTcpError(tc->Log(), "Failed to create socket. Error=(", errno, ", ", strerror(errno), ')');
            return;
        }

        // Close socket if any error or on function exit.
        Defer close_socket([accept_socket] {
            close(accept_socket);
        });

        // Make it non-blocking.
        int flags = fcntl(accept_socket, F_GETFL, 0);
        if (flags == -1) {
            XYTcpError(tc->Log(), "Failed to setup nonblocking socket. F_GETFL failed with error=(", errno, ", ", strerror(errno), ')');
            return;
        }
        flags |= O_NONBLOCK;
        if (fcntl(accept_socket, F_SETFL, flags) != 0) {
            XYTcpError(tc->Log(), "Failed to setup nonblocking socket. F_SETFL failed with error=(", errno, ", ", strerror(errno), ')');
            return;
        }

        // Apply keep-alive parameters.
        TcpSetKeepAlive(tc->Log(), accept_socket, params.keep_alive);

        // Set addr reuse if needed.
        if (params.reuse_addr) {
            TcpEnableReuseAddr(tc->Log(), accept_socket);
        }

        // Bind it to address.
        socklen_t addr_len = addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
        if (bind(accept_socket, addr, addr_len) == -1) {
            XYTcpError(tc->Log(), "Failed to bind address", bind_addr.CStr(), ':', bind_port, ". ",
                                  "Maybe address is already in use. (", errno, ", ", strerror(errno), ')');
            return;
        }

        if (listen(accept_socket, params.listen_backlog) < 0) {
            XYTcpError(tc->Log(), "Listen call on '", bind_addr.CStr(), ':', bind_port, "' failed. ",
                                  "Error=(", errno, ", ", strerror(errno), ')');
            return;
        }

        // Accept connections until the task shutdown.
        EventSource event_source{accept_socket};
        while (true) {
            sockaddr_storage accept_addr_store;
            socklen_t len = addr_len;

            tc->WaitEvent(&event_source, EventFlags::Read | EventFlags::ExactlyOnce);

            int accepted_socket = accept(accept_socket, (sockaddr *)&accept_addr_store, &len);
            if (accepted_socket < 0) {
                if (IsInProgress(errno) || errno == EINTR) {
                    continue;
                }

                XYTcpError(tc->Log(), "Failed to accept incoming connection, error=(", errno, ", ", strerror(errno), ')');
                continue;
            }

            if (tc->Log()->ShouldLog(LogLevel::Info)) {
                char buf[INET6_ADDRSTRLEN + 1];
                auto [ip, port] = SocketGetAddress(accepted_socket, buf, sizeof(buf));
                XYTcpInfo(tc->Log(), "Accepted new connection: ", ip.CStr(), ':', port);
            }
            tc->PerformAsync<TcpConnectionHandler>(accepted_socket, stream_handler);
        }
    };
};
////////////////////////////////////////////////////////////


// TcpManager.
Maybe<TcpManager>
TcpManager::Create(Dep<Log> log,
                   Dep<TaskManager> task_manager,
                   const TcpParameters &parameters,
                   Span<CStrSpan> bind_addrs,
                   TcpNewStreamHandler new_stream_handler) {
    TcpManager manager;
    for (const CStrSpan &str : bind_addrs) {
        auto res = ParseIPAddress(str);
        if (!res) {
            XYTcpError(log, "Invalid address: ", str.CStr());
            return {};
        }

        manager.bind_addrs_.push_back(res.Value());
    }

    for (const auto &address : manager.bind_addrs_) {
        task_manager->AddEntryPoint<TcpSocketAccept>(address.first, address.second, new_stream_handler, parameters);
    }
    return std::move(manager);
}
