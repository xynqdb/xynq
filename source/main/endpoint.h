#pragma once

#include "base/dep.h"
#include "base/scratch_allocator.h"
#include "base/stream.h"

namespace xynq {
class TaskContext;

// Endpoints operate in one of the below modes.
enum class EndpointMode {
    // Endpoint is not operational.
    // (for example: underlying connection is closed)
    None,
    // REPL mode: request-response with slang commands.
    Repl,
    // This endpoint is replica.
    Replica,
};

// Data format.
enum class EndpointDataFormat {
    // JSON.
    Json,
};

class Endpoint {
public:
    Endpoint(StrSpan name, InOutStream *io);

    // Human readable endpoint name. Mostly for debugging/logging.
    StrSpan Name() const { return name_; }

    // Stream mode. See EndpointMode comments.
    EndpointMode Mode() const { return mode_; }

    void Serve(TaskContext *tc);
private:
    StrSpan name_;
    InOutStream *io_ = nullptr;
    EndpointMode mode_ = EndpointMode::Repl;
    Dependable<ScratchAllocator> allocator_; // per entry point memory.

    // Buffer used for IO buffering.
    char in_buf_[1024];
    char out_buf_[1024];

    void ServeCommandMode(TaskContext *tc);
    void SetMode(EndpointMode mode);
};

} // xynq
