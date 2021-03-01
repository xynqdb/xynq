#pragma once

#include "base/stream.h"
#include "task/task.h"

namespace xynq {

class TaskContext;

// Example of an echo server implemented on xynq tasks.
// Only used for debugging.
struct EchoServer : public TaskDefaults {
    static constexpr auto debug_name = "EchoServer";

    static constexpr auto exec = [](TaskContext *, StrSpan /* name */, InOutStream *stream) {
        char buf[1024];
        bool running = true;
        while (running) {
            running = stream->Read(MutDataSpan{&buf[0], sizeof(buf)})
                .MapRight([&] (size_t bytes_read) { // If got buffer -> try write it back.
                    return stream->Write(DataSpan{&buf[0], bytes_read});
                })           // Chain of calls will return Either<Error, Either<Error, WriteSuccess>>
                .JoinRight() // Join to Either<Error, WriteSuccess>
                .IsRight();  // Check if it's WriteSuccess.
        }
    };
};

} // xynq
