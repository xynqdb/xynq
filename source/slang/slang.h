#pragma once

#include "base/either.h"
#include "base/dep.h"
#include "base/span.h"
#include "base/stream.h"

#include "env.h"
#include "compiler_def.h"

namespace xynq {
namespace slang {

// Execution context for slang code.
struct Context {
    Dep<Env> env;
    Dep<ScratchAllocator> allocator;
    void *user_data = nullptr;
};

struct ExecuteSuccess{};
using ExecuteResult = Either<CompileError, ExecuteSuccess>;

////////////////////////////////////////////////////////////

// Helpers to execute slang code.
// Reads code from reader, and writes program output into output.
ExecuteResult Execute(StreamReader &reader, Serializer &output_serializer, Context &context);

// Reads code from code and executes.
ExecuteResult Execute(StrSpan code, Serializer &output_serializer, Context &context);

} // slang
} // xynq
