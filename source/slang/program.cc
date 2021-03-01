#include "program.h"
#include "base/stream.h"
#include "base/str_builder.h"
#include "containers/vec.h"

using namespace xynq;
using namespace xynq::slang;

namespace {

void PurgeStackFrame(StackType &stack) {
    size_t sz = stack.size();
    while (sz-- > 0) {
        if (stack[sz].type == XYBasicType(FrameBarrier)) {
            stack.resize(sz);
            break;
        }
    }
}

} // anon

void Program::Execute(ProgramExecuteContext &context) {
    StackType stack{context.stack_allocator};

    CallContext call_context;
    call_context.user_data = context.user_data;

    size_t ip = 0;
    while (ip < code_.size()) {
        const Instruction &instr = code_[ip++];

        switch (instr.code) {
            case OpCode::Call: {
                StackType output_stack{context.output_stack_allocator};
                CallOutput output(output_stack);
                CallArgs args({stack.data(), stack.size()});
                call_context.output = &output;
                call_context.args = &args;
                XYAssert(instr.data.value.ptr != nullptr);
                bool result = ((Call)instr.data.value.ptr)(call_context);
                if (!result) { // Function call failed -> abort program
                    context.serializer->Serialize(call_context.error_text.Buffer());
                    return;
                }
                PurgeStackFrame(stack);

                // Apply function output to current frame.
                stack.insert(stack.end(), output_stack.begin(), output_stack.end());
                break;
            }

            case OpCode::Push: {
                stack.push_back(instr.data);
                break;
            }

            default:
                XYAssert(false);
        }
    }
    context.serializer->Serialize({stack.data(), stack.size()}).FoldLeft([](StrSpan/* err_desc*/) {
        // TODO: log error.
        // Serilizer error means underlying IO error, no other reason for serializer to fail.
        return SerializerSuccess{};
    });
}