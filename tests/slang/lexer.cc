#include "slang/lexer.h"
#include "base/stream.h"

#include "gtest/gtest.h"

using namespace xynq;
using namespace xynq::slang;

namespace {

// Test handler that is doing nothing - just returning success on every call.
struct NopHandler {
    LexerHandlerResult LexerBeginOp(StrSpan) { return LexerSuccess{}; }
    LexerHandlerResult LexerEndOp() { return LexerSuccess{}; }
    LexerHandlerResult LexerStrValue(StrSpan) { return LexerSuccess{}; }
    LexerHandlerResult LexerIntValue(int64_t) { return LexerSuccess{}; }
    LexerHandlerResult LexerDoubleValue(double) { return LexerSuccess{}; }
    LexerHandlerResult LexerUnhandledValue(StrSpan) { return StrSpan{""}; }
    LexerHandlerResult LexerCustomData(uint32_t, StreamReader &) { return LexerSuccess{}; }
};

struct TestStream : public InStream {
    MutStrSpan str_;
    size_t offset_ = 0;

    TestStream(MutStrSpan str)
        : str_(str)
    {}

    Either<StreamError, size_t> DoRead(MutDataSpan read_buf) override {
        if (offset_ >= str_.Size()) { // Close when end is reached.
            return StreamError::Closed;
        }

        size_t sz = std::min(read_buf.Size(), str_.Size() - offset_);
        memcpy(read_buf.Data(), str_.Data() + offset_, sz);
        offset_ += sz;
        return sz;
    }
};

struct TestStreamError : public InStream {
    MutStrSpan str_;
    bool should_fail = false;

    TestStreamError(MutStrSpan str)
        : str_(str)
    {}

    Either<StreamError, size_t> DoRead(MutDataSpan read_buf) override {
        if (should_fail) {
            return StreamError::IOError;
        }

        size_t sz = std::min(read_buf.Size(), str_.Size());
        memcpy(read_buf.Data(), str_.Data(), sz);
        should_fail = true; // fail after first call
        return sz;
    }
};

struct EchoHandler : public NopHandler{
    LexerHandlerResult LexerBeginOp(StrSpan op) {
        result += "(";
        result.append(op.Data(), op.Size());
        result += " ";
        return LexerSuccess{};
    }

    LexerHandlerResult LexerEndOp() {
        result += ") ";
        return LexerSuccess{};
    }

    LexerHandlerResult LexerIntValue(int64_t x) {
        result.append(std::to_string(x));
        result += " ";
        return LexerSuccess{};
    }

    LexerHandlerResult LexerStrValue(StrSpan str) {
        result += '"';
        result.append(str.Data(), str.Size());
        result += "\" ";
        return LexerSuccess{};
    }

    std::string result;
};

}

TEST(SlangLexerTest, EmptyString) {
    char code[] = "";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsRight());
}

TEST(SlangLexerTest, WrongFormat1) {
    char code[] = "(hello";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, WrongFormat2) {
    char code[] = "(h(e))llo))";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, WrongFormat3) {
    char code[] = "(a (b 2) (c 1)))";
    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, WrongFormat4) {
    char code[] = "kldfs dslk dsfl";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, EmptyParentheses) {
    char code[] = "()";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsRight());
}

TEST(SlangLexerTest, InvalidOpName1) {
    char code[] = "([]hello% 1 2 3)";
    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, InvalidOpName2) {
    char code[] = "(0987 1 2)";
    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, OpNameWithDigits) {
    char code[] = "(a789 1 2)";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsRight());
}

TEST(SlangLexerTest, AssignHandler) {
    NopHandler nop;
    Lexer<NopHandler&> lexer(nop);
    ASSERT_EQ(&nop, &lexer.handler());
}

TEST(SlangLexerTest, ParseInt) {
    struct Handler : public NopHandler{
        int64_t val_ = 0;

        LexerHandlerResult LexerIntValue(int64_t x) {
            val_ = x;
            return LexerSuccess{};
        }
    };

    Handler handler;
    Lexer<Handler&> lexer(handler);
    char test_str[] = "(+ 25)";
    auto result = lexer.Run(test_str);
    ASSERT_EQ(handler.val_, 25);
}

TEST(SlangLexerTest, ParseNegativeInt) {
    struct Handler : public NopHandler{
        int64_t val_ = 0;

        LexerHandlerResult LexerIntValue(int64_t x) {
            val_ = x;
            return LexerSuccess{};
        }
    };

    Handler handler;
    Lexer<Handler&> lexer(handler);
    char test_str[] = "(+ -25)";
    auto result = lexer.Run(test_str);
    ASSERT_EQ(handler.val_, -25);
}

TEST(SlangLexerTest, ParseDouble) {
    struct Handler : public NopHandler{
        double val_ = 0;

        LexerHandlerResult LexerDoubleValue(double x) {
            val_ = x;
            return LexerSuccess{};
        }
    };

    Handler handler;
    Lexer<Handler&> lexer(handler);
    char test_str[] = "(+ 35.67)";
    auto result = lexer.Run(test_str);
    ASSERT_EQ(handler.val_, 35.67);
}

TEST(SlangLexerTest, ParseStrValue1) {
    struct Handler : public NopHandler{
        std::string val_;

        LexerHandlerResult LexerStrValue(StrSpan str) {
            val_ = std::string{str.Data(), str.Size()};
            return LexerSuccess{};
        }
    };

    char code[] = "(+ \"Test Str 325\")";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);
    ASSERT_STREQ(handler.val_.c_str(), "Test Str 325");
}

TEST(SlangLexerTest, ParseStrValue2) {
    struct Handler : public NopHandler{
        std::string val_str_;
        int64_t val_int_ = 0;
        double val_dbl_ = 0.0;

        LexerHandlerResult LexerStrValue(StrSpan str) {
            val_str_ = std::string{str.Data(), str.Size()};
            return LexerSuccess{};
        }

        LexerHandlerResult LexerIntValue(int64_t value) {
            val_int_ = value;
            return LexerSuccess{};
        }

        LexerHandlerResult LexerDoubleValue(double value) {
            val_dbl_ = value;
            return LexerSuccess{};
        }
    };

    char code[] = "(+ 572\"Test Str 325\"654.52)";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);
    ASSERT_STREQ(handler.val_str_.c_str(), "Test Str 325");
    ASSERT_EQ(handler.val_int_, 572);
    ASSERT_EQ(handler.val_dbl_, 654.52);
}

TEST(SlangLexerTest, ParseEmptyStrValue) {
    struct Handler : public NopHandler{
        size_t size = 1;
        size_t calls = 0;

        LexerHandlerResult LexerStrValue(StrSpan str) {
            size = str.Size();
            ++calls;
            return LexerSuccess{};
        }
    };

    char code[] = "(+ \"\")";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);
    ASSERT_EQ(handler.calls, 1u);
    ASSERT_EQ(handler.size, 0u);
}

TEST(SlangLexerTest, ParseUnhandledValue) {
    struct Handler : public NopHandler{
        std::string val_str_;
        LexerHandlerResult LexerUnhandledValue(StrSpan str) {
            val_str_ = std::string{str.Data(), str.Size()};
            return LexerSuccess{};
        }
    };

    char code[] = "(hello world)";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);
    ASSERT_TRUE(result.IsRight());
    ASSERT_STREQ(handler.val_str_.c_str(), "world");
}

TEST(SlangLexerTest, ParseEscaptedStrValue) {
    struct Handler : public NopHandler{
        std::string val_;

        LexerHandlerResult LexerStrValue(StrSpan str) {
            val_ = std::string{str.Data(), str.Size()};
            return LexerSuccess{};
        }
    };

    char code[] = "(+ \"Test Str \\\"325\\\"\")";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);
    ASSERT_STREQ(handler.val_.c_str(), "Test Str \"325\"");
}

TEST(SlangLexerTest, NestedOperations) {
    EchoHandler handler;
    Lexer<EchoHandler&> lexer(handler);
    char test_str[] = "(+ (foo (* 1 \"two\" ) ) (+ 3 \"three\" \"four\" 5 ) ) ";
    auto result = lexer.Run(test_str);
    ASSERT_STREQ(test_str, handler.result.c_str());
}

TEST(SlangLexerTest, SyntaxError) {
    struct Handler : public NopHandler {
        LexerHandlerResult LexerBeginOp(StrSpan op) {
            if (op[0] != 'x') {
                return StrSpan("<error>");
            } else {
                return LexerSuccess{};
            }
        }
    };

    char code[] = R"(
(x
    (y 1 2))
)";

    Lexer<Handler> lexer;
    auto result = lexer.Run(code);

    ASSERT_TRUE(result.IsLeft());
    ASSERT_EQ(result.Left().err_line_no_, 3u);
    ASSERT_EQ(result.Left().err_line_offset_, 7u);
    ASSERT_STREQ(result.Left().err_msg_.Data(), "<error>");
}

TEST(SlangLexerTest, Comment) {
    char code[] = R"(
        (x           ; this is comment1
            (y 1 2)) ; comment2 325 abcdefg
        ; comment 3 3 3 3 3;
        ;;; comment 4 4 4 4 4
        )";

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(code);

    ASSERT_TRUE(result.IsRight());
}

TEST(SlangLexerTest, Streaming) {
    std::string test_str = "(+ (foo (* 1 \"two\" ) ) (+ 3 \"three\" \"four\" 5 ) ) ";
    std::string to_parse = test_str;

    char buffer[4]; // streaming by chunks by 4 bytes.
    TestStream stream{MutStrSpan{to_parse.data(), to_parse.size()}};
    StreamReader reader{buffer, stream};

    ScratchAllocator allocator;

    EchoHandler handler;
    Lexer<EchoHandler&> lexer(handler);
    auto result = lexer.Run(reader, allocator);
    ASSERT_STREQ(test_str.c_str(), handler.result.c_str());
}

TEST(SlangLexerTest, StreamError) {
    char code[] = "(+ (foo (* 1 \"two\" ) ) (+ 3 \"three\" \"four\" 5 ) ) ";

    char buffer[4]; // streaming by chunks by 4 bytes.
    TestStreamError stream{MutStrSpan{&code[0], sizeof(code)}};
    StreamReader reader{buffer, stream};

    ScratchAllocator allocator;

    Lexer<NopHandler> lexer;
    auto result = lexer.Run(reader, allocator);
    ASSERT_TRUE(result.IsLeft());
    ASSERT_FALSE(reader.IsGood());
}

TEST(SlangLexerTest, StreamSyntaxError) {
    std::string test_str = "(+ 0 1 2 3 4 5 6 7 8 9";
    std::string to_parse = test_str;

    char buffer[4]; // streaming by chunks by 4 bytes.
    TestStream stream{MutStrSpan{to_parse.data(), to_parse.size()}};
    StreamReader reader{buffer, stream};

    ScratchAllocator allocator;

    EchoHandler handler;
    Lexer<EchoHandler&> lexer(handler);
    auto result = lexer.Run(reader, allocator, true);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, CustomData) {
    struct Handler : public NopHandler {
        char test_buf[32] = {0};
        uint32_t test_token = 0;

        LexerHandlerResult LexerCustomData(uint32_t token, StreamReader &reader) {
            test_token = token;
            for (int i = 0; i < 26; ++i) {
                test_buf[i] = reader.ReadValue<char>().FoldLeft([](StreamError) {
                    return '\0';
                });
            }

            test_buf[26] = 0;
            return LexerSuccess{};
        }
    };

    char code[] = "(test !blah[1234567890!@#$%^&*()qwerty])";

    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);

    ASSERT_TRUE(result.IsRight());
    ASSERT_EQ(handler.test_token, (uint32_t)(('b' << 24u) | ('l' << 16u) | ('a' << 8u) | 'h'));
    ASSERT_STREQ(handler.test_buf, "1234567890!@#$%^&*()qwerty");
}

TEST(SlangLexerTest, CustomDataError) {
    struct Handler : public NopHandler {
        LexerHandlerResult LexerCustomData(uint32_t, StreamReader &) {
            return LexerSuccess{};
        }
    };

    char code[] = "(test !blah[1234567890!@#$%^&*()qwerty])";
    Lexer<Handler> lexer;
    auto result = lexer.Run(code);

    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangLexerTest, CustomDataNoToken) {
    struct Handler : public NopHandler {
        uint32_t token_ = 1;
        LexerHandlerResult LexerCustomData(uint32_t token, StreamReader &) {
            token_ = token;
            return LexerSuccess{};
        }
    };

    char code[] = "(test ![])";
    Handler handler;
    Lexer<Handler&> lexer(handler);
    auto result = lexer.Run(code);

    ASSERT_TRUE(result.IsRight());
    ASSERT_EQ(handler.token_, 0u);
}
