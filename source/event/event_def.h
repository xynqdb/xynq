#pragma once

#include <cstdint>

namespace xynq {

// Flags to subscribe to certain events types.
struct EventFlags {
    enum {
        Read        = 1 << 0, // Eventsource has new incoming data ready for read.
        Write       = 1 << 1, // Eventsource is ready to accept new outgoing data.
        ExactlyOnce = 1 << 2, // Will only hit once.
    };
};

using EventUserHandle = void*;

} // xynq