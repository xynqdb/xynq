#pragma once

#include "event.h"

#include "base/dep.h"
#include "base/log.h"
#include "base/span.h"

#include <vector>

namespace xynq {

// Linux/epoll-based EventQueue implementation.
class EpollEventQueue {
public:
    //  thread_max_events_at_once: is number of events that can be processed
    //                             on a single epoll_wait hit per single thread.
    //  num_threads: number of threads this queue will be running on.
    EpollEventQueue(Dep<Log> log,
                    size_t thread_max_events_at_once,
                    size_t num_threads);
    ~EpollEventQueue();

    EpollEventQueue(const EpollEventQueue &) = delete;
    EpollEventQueue &operator=(const EpollEventQueue &) = delete;

    // Adds event to epoll.
    // fs is expected to be valid file descriptor ie. valid socket.
    void AddEvent(EpollEventSource &event_source, uint32_t event_flags, void *user_handle);

    // Removes file descriptor from epoll.
    void RemoveEvent(EpollEventSource &event_source);

    // Blocks and waits for events, basically epoll_wait.
    Span<Event> Wait(size_t thread_index, int timeout_msec);

    // Interrupts specific thread.
    // Guarantees st least one thread woken up. Does not guarantee it will be preferred_thread_index;
    // preferred_thread_index = -1 - means no preference.
    void Interrupt(size_t preferred_thread_index = ~size_t());

    // Interrupts waits of all threads.
    void InterruptAll();
private:
    Dep<Log> log_;
    epoll_event *events_ = nullptr;
    int thread_max_events_ = 0;
    size_t thread_events_size_ = 0;
    int epoll_fd_ = -1;
    EpollEventSource wakeup_fd_; // dummy event to wakeup threads.
};

using EventQueue = EpollEventQueue;

} // xynq