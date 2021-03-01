#pragma once

#include "event/event_def.h"

#include <sys/epoll.h>

namespace xynq {

// EventSource is bsd socket wrapper.
class EpollEventSource {
    friend class EpollEventQueue;
public:
    inline EpollEventSource() = default;
    inline EpollEventSource(int fd)
        : fd_(fd)
    {}

    inline int FD() const { return fd_; };
private:
    int fd_ = -1;
    bool is_added_ = false; // Used by epoll queue.
};


// Epoll-based event implementation.
class EpollEvent : public epoll_event {
public:
    // Event source is ready for read.
    inline bool IsRead() const { return (events & EPOLLIN) != 0; }
    // Event source is ready for write.
    inline bool IsWrite() const { return (events & EPOLLOUT) != 0; }
    // Error condition happened on the event source. Event source is no longer valid.
    inline bool IsError() const { return (events & EPOLLERR) != 0; }
    // Event source is closed (ie. connection was closed) and no longer valid.
    inline bool IsClose() const { return (events & (EPOLLHUP | EPOLLRDHUP)) != 0; }
    // Returns user provided handles when this Event was registered with its queue.
    inline EventUserHandle UserHandle() const { return data.ptr; }
};


using Event = EpollEvent;
using EventSource = EpollEventSource;

} // xynq
