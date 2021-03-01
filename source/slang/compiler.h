#pragma once

#include "compiler_def.h"

#include "env.h"
#include "lexer.h"
#include "program.h"

#include "base/dep.h"

namespace xynq {
class StreamReader;

namespace slang {

class Compiler {
public:
    explicit Compiler(Dep<Env> env);
    CompileResult Build(StreamReader &reader, Dep<ScratchAllocator> allocator);

private:
    Dep<Env> env_;
    Program *cur_program_ = nullptr;
    Dep<ScratchAllocator> cur_allocator_;
    StrBuilder<128> error_builder_;

    // Lexer handlers.
    template<class T> friend class Lexer;
    LexerHandlerResult LexerBeginOp(StrSpan);
    LexerHandlerResult LexerEndOp();
    LexerHandlerResult LexerStrValue(StrSpan);
    LexerHandlerResult LexerIntValue(int64_t);
    LexerHandlerResult LexerDoubleValue(double);
    LexerHandlerResult LexerUnhandledValue(StrSpan);
    LexerHandlerResult LexerCustomData(uint32_t, StreamReader &);


    void AddStrValue(TypeSchemaPtr type, StrSpan value);
};

} // slang
} // xynq
