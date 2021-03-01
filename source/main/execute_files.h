#pragma once

#include "shared_deps.h"

#include "base/file_stream.h"
#include "slang/slang.h"
#include "task/task.h"
#include "task/task_context.h"

DefineTaggedLog(ExecFiles)

namespace xynq {

struct ExecuteFiles : public TaskDefaults {
    static constexpr auto debug_name = "ExecuteFiles";

    static constexpr auto exec = [](TaskContext *tc, Vec<CStrSpan> files) {
        Dependable<ScratchAllocator> allocator = ScratchAllocator{};

        slang::Context context {
            tc->UserData<SharedDeps>().slang_env,
            allocator,
            &(tc->UserData<SharedDeps>())
        };

        for (CStrSpan filepath : files) {
            InFileStream stream;
            if (!stream.Open(filepath)) {
                XYExecFilesError(tc->Log(), "Cannot read exec file '", filepath, "'.");
                tc->Exit();
                return;
            }

            char buf[512];
            StreamReader reader(MutDataSpan{&buf[0], sizeof(buf)}, stream);
            DummySerializer output_serializer;
            XYExecFilesInfo(tc->Log(), "Executing '", filepath, "'.");
            slang::Execute(reader, output_serializer, context);
        }
    };
};

} // xynq
