#pragma once

#include "call.h"
#include "types/serializer.h"

#include <vector>

namespace xynq {

class StreamWriter;

namespace slang {

struct ProgramExecuteContext {
    Serializer *serializer = nullptr;
    void *user_data = nullptr;
    Dep<ScratchAllocator> stack_allocator;
    Dep<ScratchAllocator> output_stack_allocator;
};

// Immutable program.
class Program {
    friend class Compiler;
public:
    void Execute(ProgramExecuteContext &context);

private:
   Vec<Instruction> code_;
};

} // slang
} // xynq