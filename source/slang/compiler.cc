#include "compiler.h"
#include "base/stream.h"

using namespace xynq;
using namespace xynq::slang;

Compiler::Compiler(Dep<Env> env)
    : env_(env) {
}

CompileResult Compiler::Build(StreamReader &reader, Dep<ScratchAllocator> allocator) {
    Program program;
    Lexer<Compiler&> lexer(*this);

    cur_program_ = &program;
    cur_allocator_ = allocator;
    auto parse_result = lexer.Run(reader, *allocator, true);
    if (parse_result.IsLeft()) {
        CompileError error{parse_result.Left()};
        return error;
    }
    std::reverse(program.code_.begin(), program.code_.end());
    return program;
}

LexerHandlerResult Compiler::LexerBeginOp(StrSpan op_name) {
    Call call = env_->FindCall(op_name);
    if (call == nullptr) {
        error_builder_ << "Unknown function '" << op_name << "'";
        return error_builder_.Buffer();
    }

    cur_program_->code_.emplace_back(OpCode::Call,
                                     TypedValue{k_types_invalid_schema, (void *)call});
    return LexerSuccess{};
}

LexerHandlerResult Compiler::LexerEndOp() {
    cur_program_->code_.emplace_back(OpCode::Push,
                                     TypedValue{XYBasicType(FrameBarrier), 0});
    return LexerSuccess{};
}

LexerHandlerResult Compiler::LexerStrValue(StrSpan value) {
    AddStrValue(XYBasicType(StrSpan), value);
    return LexerSuccess{};
}

LexerHandlerResult Compiler::LexerIntValue(int64_t value) {
    cur_program_->code_.emplace_back(OpCode::Push,
                                     TypedValue{XYBasicType(int64_t), value});
    return LexerSuccess{};
}

LexerHandlerResult Compiler::LexerDoubleValue(double value) {
    cur_program_->code_.emplace_back(OpCode::Push,
                                     TypedValue{XYBasicType(double), value});
    return LexerSuccess{};
}

LexerHandlerResult Compiler::LexerUnhandledValue(StrSpan value) {
    if (value.Size() > 1 && *value.begin() == ':') { // field name.
        AddStrValue(k_slang_field_type_ptr, StrSpan{value.Data() + 1, value.Size() - 1});
        return LexerSuccess{};
    } else {
        return LexerStrValue(value); // should be something like Identifier
    }
}

LexerHandlerResult Compiler::LexerCustomData(uint32_t token, StreamReader &reader) {
    PayloadHandler *payload_handler = env_->FindPayloadHandler(token);
    if (payload_handler == nullptr) {
        error_builder_ << "Unknown payload type: " << token;
        return error_builder_.Buffer();
    }

    return payload_handler->ProcessPayload(reader, *cur_allocator_).Fold([](StrSpan &&error) -> LexerHandlerResult {
        return error;
    }, []() -> LexerHandlerResult {
        return LexerSuccess{};
    });
}

void Compiler::AddStrValue(TypeSchemaPtr type, StrSpan value) {
    char *buf = (char *)cur_allocator_->Alloc(value.Size());
    memcpy(buf, value.Data(), value.Size());
    cur_program_->code_.emplace_back(OpCode::Push,
                                     TypedValue{type, StrSpan{buf, value.Size()}});

}
