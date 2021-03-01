#pragma once

#include "base/assert.h"
#include "base/either.h"
#include "base/defer.h"
#include "base/maybe.h"
#include "base/span.h"
#include "base/stream.h"
#include "base/str_builder.h"

#include "containers/str.h"

namespace xynq {
namespace slang {

struct LexerSuccess {};

struct LexerFailure {
    // Error line number.
    size_t err_line_no_ = 0;
    // Error char offset.
    size_t err_line_offset_ = 0;
    // Error message.
    StrSpan err_msg_;
};

using LexerResult = Either<LexerFailure, LexerSuccess>;
using LexerHandlerResult = Either<StrSpan, LexerSuccess>;

namespace detail {

enum class TermType {
    kOp,    // Operation. ie. term after '('
    kValue, // Value, ie. 123 or 123.0 or `TypeName
    kStr    // String ie. inside ".."
};

static constexpr char k_lexer_invalid_char = 0;

struct LexerState {
    StreamReader *stream_ = nullptr;
    ScratchAllocator *allocator_ = nullptr;
    bool single_expr_ = false;
    bool is_running_ = false;

    size_t cur_line_ = 1;
    size_t cur_line_offset_ = 0;
    int op_depth_ = 0;
    bool is_escaped_ = false;
    bool was_escaped_ = false;

    TermType term_type_ = TermType::kValue;
    char *term_begin_ = nullptr;
    ScratchStr term_buf_;

    LexerState(StreamReader *stream, ScratchAllocator *allocator, bool single_expr)
        : stream_(stream)
        , allocator_(allocator)
        , single_expr_(single_expr)
        , term_buf_(allocator)
    {
        XYAssert(stream_ != nullptr);
        XYAssert(allocator_ != nullptr);
    }

    inline char NextChar() {
        if (stream_->Available().IsEmpty() && !RefillBuffer()) {
            return k_lexer_invalid_char;
        }

        ++cur_line_offset_;
        return stream_->ReadAvailableCharUnsafe();
    }

    inline bool RefillBuffer() {
        bool hasTerm = HasTerm();
        SaveTerm(BufferEnd());

        return stream_->RefillAvailable()
            .Fold([&](StreamError) -> bool {
                return false;
            }, [&](MutDataSpan) -> bool {
                if (hasTerm) { // continue term.
                    term_begin_ = Buffer();
                }
                return true;
            });
    }

    inline void NewLine() { ++cur_line_; cur_line_offset_ = 0; }
    inline void Escape() { is_escaped_ = true; was_escaped_ = true; }
    inline void ResetEscape() { is_escaped_ = false; }
    inline bool IsRunning() const { return is_running_; }

    inline void StartTerm(TermType type, char *term_ptr) { term_type_ = type; term_begin_ = term_ptr; }
    inline void FinishTerm() {
        term_type_ = TermType::kValue;
        term_begin_ = nullptr;
        term_buf_.clear();
    }

    inline bool HasTerm() const {
        return term_begin_ != nullptr;
    }

    inline void SaveTerm(char *term_end) { // Buffer term for (if it got chunked while streaming)
        if (term_begin_ != nullptr) {
            term_buf_.append(term_begin_, term_end);
            term_begin_ = nullptr;
        }
    }

    inline MutStrSpan Term() {
        if (term_buf_.size() > 0) {
            SaveTerm(Buffer() - 1);
            return MutStrSpan{term_buf_.data(), term_buf_.size()};
        } else if (term_begin_ != nullptr) {
            return MutStrSpan{term_begin_, Buffer() - 1};
        }

        return {};
    }

    LexerFailure Fail(StrSpan err_msg) {
        return LexerFailure{cur_line_, cur_line_offset_, err_msg};
    }

    inline char *Buffer() const {
        return (char *)stream_->Available().begin();
    }

    inline char *BufferEnd() const {
        return (char *)stream_->Available().end();
    }
};

bool LexerCheckOpName(const char *begin, const char *end);
Maybe<int64_t> LexerParseInt64(const char *begin, const char *end);
Maybe<double> LexerParseDouble(const char *begin, const char *end);
StrSpan LexerParseString(char *begin, char *end, bool was_escaped);

} // namespace detail


// Lexer of prefix S-expressions.
// ie. (+ 1 2 3 4)
//     ()
//     (func arg1 arg2)
// Handler must satisfy this interface:
//      Either<StrSpan, *> Handler::LexerOpBegin();
//      Either<StrSpan, *> Handler::LexerOpEnd();
//      Either<StrSpan, *> Handler::LexerValueStr();
// Either second parameter is a error message.
template<class Handler>
class Lexer {
public:
    Lexer();
    explicit Lexer(Handler handler);

    Lexer(const Lexer &) = delete;
    Lexer(Lexer &&) = delete;
    Lexer &operator=(const Lexer &) = delete;
    Lexer &operator=(Lexer &&) = delete;

    Handler &handler() { return handler_; }
    const Handler &handler() const { return handler_; }

    // Helper for passing StrSpan.
    LexerResult Run(MutStrSpan text);

    // Parse slang code from the stream.
    // If single_expr is true - will parse only single S-expression out of the stream.
    LexerResult Run(StreamReader &stream, ScratchAllocator &allocator, bool single_expr = false);

private:
    Handler handler_;
    StrBuilder<64> err_description_;

    LexerResult FinalizeTerm(detail::LexerState &state);
};
////////////////////////////////////////////////////////////


// Lexer Implementation.
template<class Handler>
Lexer<Handler>::Lexer()
    : handler_(Handler{}) {
}

template<class Handler>
Lexer<Handler>::Lexer(Handler handler)
    : handler_(handler) {
}

template<class Handler>
LexerResult Lexer<Handler>::Run(MutStrSpan text) {
    DummyInStream in_stream;
    StreamReader reader{MutDataSpan{text.Data(), text.Size()}, in_stream, text.Size()};
    ScratchAllocator allocator{0};

    return Run(reader, allocator);
}

template<class Handler>
LexerResult Lexer<Handler>::Run(StreamReader &stream, ScratchAllocator &allocator, bool single_expr) {
    using namespace detail;

    LexerState state{&stream, &allocator, single_expr};
    LexerResult result = LexerSuccess{};

    // Parse either until the end of data or a error.
    while (result.IsRight()) {
        char cur_char = state.NextChar();
        if (cur_char == k_lexer_invalid_char) {
            break;
        }

        bool cur_escaped = state.is_escaped_;
        state.ResetEscape();

        if (state.term_type_ == TermType::kStr) { // We are inside string literal.
            if (cur_char != '"' || cur_escaped) {

                // If it's escape char \ => mark state escaped so that the next char is affected.
                if (cur_char == '\\' && !cur_escaped) {
                    state.Escape();
                }

                if (cur_char == '\n') {
                    state.NewLine();
                }

                continue;
            }
        }

        switch (cur_char) {
            case '(': { // Operation begin.
                result = FinalizeTerm(state);
                ++state.op_depth_;
                state.StartTerm(TermType::kOp, state.Buffer());
                break;
            }

            case ')': { // Operation end.
                result = FinalizeTerm(state)
                    .Fold([&](LexerFailure &&failure) -> LexerResult {
                        return failure;
                    }, [&](LexerSuccess) -> LexerResult {
                        return handler_.LexerEndOp().MapLeft([&](StrSpan &&err_msg) {
                            return state.Fail(err_msg);
                        });
                    });

                --state.op_depth_;
                if (state.op_depth_ < 0) {
                    return state.Fail("Redundant closing parenthesis");
                }

                // If we only care for a single S-expression - interrupt parsing here.
                if (state.single_expr_ && result.IsRight() && state.op_depth_ == 0) {
                    return LexerSuccess{};
                }
                break;
            }

            case '"': { // String literal
                if (state.term_type_ == TermType::kStr) { // Already inside string -> finalize it.
                    result = FinalizeTerm(state);
                } else { // It's a beginning of a new string.
                    result = FinalizeTerm(state)
                        .Fold([&](LexerFailure &&failure) -> LexerResult {
                            return failure;
                        }, [&](LexerSuccess) -> LexerResult {
                            state.StartTerm(TermType::kStr, state.Buffer());
                            return LexerSuccess{};
                        });
                }

                break;
            }

            case '!': { // Custom payload.
                uint32_t data_token = 0;
                int token_size = 0;
                char last_char = 0;
                while ((last_char = state.NextChar()) != k_lexer_invalid_char
                       && last_char != '['
                       && token_size < 4) {

                    data_token <<= 8;
                    data_token |= static_cast<uint8_t>(last_char);
                    ++token_size;
                }

                if (last_char == '[') {
                    result = handler_.LexerCustomData(data_token, stream).MapLeft([&](StrSpan &&err_msg) {
                        return state.Fail(err_msg);
                    });

                    if (state.NextChar() != ']') {
                        result = state.Fail("No closing ] for custom data");
                    }
                } else {
                    result = state.Fail("Invalid openning tag for custom data");
                }
                state.FinishTerm();
                break;
            }

            case ';': { // Comment.
                // Just skip until the end of line or the end of stream.
                while (true) {
                    char cur_char_comment = state.NextChar();
                    if (cur_char_comment == k_lexer_invalid_char || cur_char_comment == '\n') {
                        break;
                    }
                }

                state.NewLine();
                break;
            }

            case '\n':
                state.NewLine();
            case ' ':
            case '\t':
            case '\r':
                result = FinalizeTerm(state);
                break;

            default: { // Identifier or value.
                if (!state.HasTerm()) {
                    state.StartTerm(TermType::kValue, state.Buffer() - 1);
                }
            }
        }
    }

    if (result.IsLeft()) { // Syntax error while parsing.
        return result;
    }

    XYAssert(state.op_depth_ >= 0); // Some logic error if depth is negative here.
    if (state.op_depth_ > 0) { // Extra left parentheses.
        return state.Fail("Missing closing parenthesis");
    }

    if (state.term_type_ == TermType::kStr) { // Left side opened string like "abc
        return state.Fail("Invalid string literal - not closed");
    }

    return LexerSuccess{};
}

// Finalize parsing of a terminal token.
template<class Handler>
LexerResult Lexer<Handler>::FinalizeTerm(detail::LexerState &state) {
    using namespace detail;

    LexerResult result = LexerSuccess{};
    if (!state.HasTerm()) {
        return result;
    }

    if (state.op_depth_ <= 0) { // Got term without any surrounding operation.
        return state.Fail("Expected opening bracket");
    }

    MutStrSpan term = state.Term();
    switch (state.term_type_) {
        case TermType::kOp: {
            if (term.IsEmpty()) {
                break;
            }

            if (!LexerCheckOpName(term.begin(), term.end())) {
                err_description_.Append("Invalid op name: ");
                err_description_.Append(term);
                result = state.Fail(err_description_.Buffer());
                break;
            }

            result = handler_.LexerBeginOp(StrSpan{term.Data(), term.Size()})
                .MapLeft([&](StrSpan &&err_msg) {
                    return state.Fail(err_msg);
                });
            break;
        }

        case TermType::kValue: {
            if (term.IsEmpty()) {
                break;
            }

            if (auto lval = LexerParseInt64(term.begin(), term.end()); lval.HasValue()) {
                result = handler_.LexerIntValue(lval.Get())
                    .MapLeft([&](StrSpan &&err_msg) {
                        return state.Fail(err_msg);
                    });
            } else if (auto dval = LexerParseDouble(term.begin(), term.end()); dval.HasValue()) {
                result = handler_.LexerDoubleValue(dval.Get())
                    .MapLeft([&](StrSpan &&err_msg) {
                        return state.Fail(err_msg);
                    });
            } else {
                result = handler_.LexerUnhandledValue(term)
                    .MapLeft([&](StrSpan &&err_msg) {
                        if (err_msg.Size() > 0) {
                            return state.Fail(err_msg);
                        } else {
                            err_description_.Append("Invalid value type: ");
                            err_description_.Append(term);
                            return state.Fail(err_description_.Buffer());
                        }
                    });
            }
            break;
        }

        case TermType::kStr: {
            XYAssert(!state.is_escaped_);

            auto str_val = LexerParseString(term.begin(), term.end(), state.was_escaped_);
            result = handler_.LexerStrValue(str_val)
                .MapLeft([&](StrSpan &&err_msg) {
                    return state.Fail(err_msg);
                });
            state.was_escaped_ = false;
            break;
        }
        default:
            break;
    }

    state.FinishTerm();
    return result;
}

} // namespace slang
} // namespace xynq
