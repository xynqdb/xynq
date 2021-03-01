#include "slang.h"
#include "compiler.h"

#include "base/stream.h"
#include "base/str_build_types.h"

using namespace xynq;
using namespace slang;

namespace {

void BuildCompileErrorText(const CompileError &error, StrBuilder<128> &str_builder) {
    if (error.error_type == CompileError::SyntaxError) {
        str_builder << "Error(ln " << error.err_line_no_ << ", col " << error.err_line_offset_ << "): ";
        str_builder << error.err_msg_;
    } else {
        str_builder << "IOError";
    }
}

} // anon namespace

ExecuteResult xynq::slang::Execute(StreamReader &reader, Serializer &output_serializer, Context &context) {
    Compiler compiler(context.env);
    auto result = compiler.Build(reader, context.allocator);
    if (result.IsLeft()) {
        StrBuilder<128> err_desc; // temp buffer - will build string, serialize and trash this buffer.
        BuildCompileErrorText(result.Left(), err_desc);
        output_serializer.Serialize(err_desc.Buffer());
        return result.Left();
    }

    Program &program = result.Right();
    ProgramExecuteContext program_context;
    program_context.serializer = &output_serializer;
    program_context.user_data = context.user_data;
    program_context.stack_allocator = context.allocator;
    program_context.output_stack_allocator = context.allocator;
    program.Execute(program_context);
    return ExecuteSuccess{};
}

ExecuteResult xynq::slang::Execute(StrSpan code, Serializer &output_serializer, Context &context) {
    DummyInStream dummy_stream;
    StreamReader reader{MutDataSpan{(void *)code.Data(), code.Size()}, dummy_stream};
    return Execute(reader, output_serializer, context);
}
