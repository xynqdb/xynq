#include "event/eventqueue.h"

#include "base/assert.h"
#include "base/system_allocator.h"
#include "base/platform_def.h"
#include "event/event_def.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <climits>
#include <string.h>
#include <unistd.h>

DefineTaggedLog(Event)

using namespace xynq;

EpollEventQueue::EpollEventQueue(Dep<Log> log,
                                 size_t thread_max_events_at_once,
                                 size_t num_threads)
    : log_(log) {
    XYAssert(thread_max_events_at_once <= INT_MAX);

    epoll_fd_ = ::epoll_create1(0);
    // add padding between buffers for multiple threads to avoid cache line sharing.
    size_t padding = (k_cache_line_size + sizeof(epoll_event)) / sizeof(epoll_event);
    thread_events_size_ = thread_max_events_at_once + padding;
    events_ = reinterpret_cast<epoll_event *>(SystemAllocator::Shared().Alloc(
                                                sizeof(epoll_event) * thread_events_size_ * num_threads));
    thread_max_events_ = static_cast<int>(thread_max_events_at_once);
    wakeup_fd_ = EpollEventSource{eventfd(0, EFD_NONBLOCK)};

    XYAssert(wakeup_fd_.FD() >= 0);
    XYAssert(events_ != nullptr);
    XYAssert(epoll_fd_ >= 0);
    AddEvent(wakeup_fd_, EventFlags::Read, nullptr);
}

EpollEventQueue::~EpollEventQueue() {
    close(wakeup_fd_.FD());

    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
    }

    SystemAllocator::Shared().Free(events_);
}

void EpollEventQueue::AddEvent(EpollEventSource &event_source, uint32_t event_flags, void *user_handle) {
    XYAssert(event_source.FD() >= 0);

    epoll_event event;
    event.events = EPOLLERR | EPOLLHUP;
    event.data.ptr = user_handle;

    // Allow events on one descriptor(ie. one connection) only come to a single thread exclusevely.
    if (event_flags & EventFlags::Read) {
        event.events |= EPOLLIN;
    }
    if (event_flags & EventFlags::Write) {
        event.events |= EPOLLOUT;
    }
    if (event_flags & EventFlags::ExactlyOnce) {
        event.events |= EPOLLONESHOT;
    }

    int err;
    if (event_source.is_added_) {
        err = ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event_source.FD(), &event);
    } else {
        err = ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_source.FD(), &event);
        event_source.is_added_ = true;
    }

    if (err < 0) {
        XYEventError(log_, "epoll_ctl add failed with ", errno, '(', ::strerror(errno), ')');
    }
}

void EpollEventQueue::RemoveEvent(EpollEventSource &event_source) {
    XYAssert(event_source.FD() >= 0);

    int err = ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_source.FD(), nullptr);
    if (err < 0) {
        XYEventError(log_, "epoll_ctl del failed with ", errno, '(', ::strerror(errno), ')');
    } else {
        event_source.is_added_ = false;
    }
}

Span<Event> EpollEventQueue::Wait(size_t thread_index, int timeout_msec) {
    static_assert(sizeof(Event) == sizeof(struct epoll_event),
        "Invalid event type. Should be epoll_event.");
    XYAssert(epoll_fd_ >= 0);

    epoll_event *events_buf = events_ + thread_events_size_ * thread_index;
    int nevents = ::epoll_wait(epoll_fd_, events_buf, thread_max_events_, timeout_msec);

    if (nevents < 0) {
        if (errno != EINTR) {
            XYEventError(log_, "epoll_wait failed with error=", errno, '(', ::strerror(errno), ')');
        }
        return {};
    }

    // Drain interrupt event before waiting.
    int drain_result = -1;
    do {
        char buf[64];
        drain_result = read(wakeup_fd_.FD(), buf, sizeof(buf));
    } while (drain_result > 0);

    return {static_cast<const Event *>(events_buf),
            static_cast<size_t>(nevents)};
}

void EpollEventQueue::Interrupt(size_t /*thread_index*/) {
    uint64_t value = 1;
    write(wakeup_fd_.FD(), &value, 8);
}

void EpollEventQueue::InterruptAll() {
    uint64_t value = 1;
    write(wakeup_fd_.FD(), &value, 8);
}
