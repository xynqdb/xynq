#pragma once

#include "lexer.h"
#include "program.h"

namespace xynq {
namespace slang {

struct CompileError : public slang::LexerFailure {
    enum ErrorType {
        IOError,
        SyntaxError,
    };
    ErrorType error_type;

    CompileError(const LexerFailure &lexer_fail) {
        error_type = ErrorType::SyntaxError;
        err_line_no_ = lexer_fail.err_line_no_;
        err_line_offset_ = lexer_fail.err_line_offset_;
        err_msg_ = lexer_fail.err_msg_;
    }
};

using CompileResult = Either<CompileError, Program>;

} // slang
} // xynq
