#pragma once

#include "endpoint.h"

#include "base/span.h"
#include "task/task.h"

namespace xynq {

class TaskContext;
class InOutStream;

// Created one per every endpoint.
struct EndpointHandler : public TaskDefaults {
    static constexpr auto debug_name = "EndpointHandler";
    static constexpr auto exec = [](TaskContext *tc, StrSpan name, InOutStream *stream) {
        Endpoint endpoint(name, stream);
        endpoint.Serve(tc);
    };
};

} // xynq
