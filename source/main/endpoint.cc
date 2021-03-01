#include "endpoint.h"
#include "shared_deps.h"

#include "base/log.h"
#include "base/span.h"
#include "json/json_serializer.h"
#include "slang/slang.h"
#include "task/task_context.h"

using namespace xynq;
using namespace xynq::slang;

DefineTaggedLog(Endpoint)

using namespace xynq;
Endpoint::Endpoint(StrSpan name, InOutStream *io)
    : name_{name}
    , io_{io}
    , allocator_{ScratchAllocator{}} {
    XYAssert(io_);
}

void Endpoint::SetMode(EndpointMode mode) {
    mode_ = mode;
}

void Endpoint::Serve(TaskContext *tc) {
    XYEndpointInfo(tc->Log(), "Start serving endpoint ", name_);

    while (mode_ != EndpointMode::None) {
        switch (mode_) {
            case EndpointMode::Repl:
                ServeCommandMode(tc);
                break;
            default:
                XYAssert(false);
        }

        XYEndpointInfo(tc->Log(), "Endpoint '", name_, "' switched to mode: ", (int)mode_);
    }
}

void Endpoint::ServeCommandMode(TaskContext *tc) {
    slang::Context context {
        tc->UserData<SharedDeps>().slang_env,
        allocator_,
        &(tc->UserData<SharedDeps>())
    };

    StreamReader request_reader(MutDataSpan{&in_buf_[0], sizeof(in_buf_)}, *io_);
    StreamWriter response_writer(MutDataSpan{&out_buf_[0], sizeof(out_buf_)}, *io_);
    JsonSerializer output_serializer(response_writer);

    while (request_reader.IsGood() && response_writer.IsGood()) {
        slang::Execute(request_reader, output_serializer, context);
        allocator_->Purge();
    }

    XYEndpointInfo(tc->Log(), "Data stream closed. Will drop endpoint: ", name_);
    SetMode(EndpointMode::None);
}
