#include "slang/compiler.h"

#include "base/dep.h"
#include "base/stream.h"

#include "gtest/gtest.h"

using namespace xynq;
using namespace xynq::slang;

namespace {

Env CreateTestEnv() {
    FuncTable func_table;
    func_table["nop"] = [](slang::CallContext &) {
        return true;
    };

    PayloadHandlerTable payload_handlers;
    return slang::Env{std::move(func_table), std::move(payload_handlers)};
}

} // anon namespace


TEST(SlangCompilerTest, Whitespace) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    char code[] = "                ";

    DummyInStream stream;
    StreamReader reader{MutDataSpan{&code[0], sizeof(code)}, stream, sizeof(code)};

    Compiler compiler{env};
    auto res = compiler.Build(reader, allocator);
    ASSERT_TRUE(res.IsRight());
}

TEST(SlangCompilerTest, Nil) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    char code[] = "()";

    DummyInStream stream;
    StreamReader reader{MutDataSpan{&code[0], sizeof(code)}, stream, sizeof(code)};

    Compiler compiler{env};
    auto res = compiler.Build(reader, allocator);
    ASSERT_TRUE(res.IsRight());
}

TEST(SlangCompilerTest, SyntaxErrorNoFunction) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    char code[] = "(xxx 1 2)";

    DummyInStream stream;
    StreamReader reader{MutDataSpan{&code[0], sizeof(code)}, stream, sizeof(code)};

    Compiler compiler{env};
    auto res = compiler.Build(reader, allocator);
    ASSERT_TRUE(res.IsLeft());
}

TEST(SlangCompilerTest, SyntaxErrorNoClosingPar) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    char code[] = "(nop 1 2 3";

    DummyInStream stream;
    StreamReader reader{MutDataSpan{&code[0], sizeof(code)}, stream, sizeof(code)};

    Compiler compiler{env};
    auto res = compiler.Build(reader, allocator);
    ASSERT_TRUE(res.IsLeft());
}

TEST(SlangCompilerTest, Success) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    char code[] = "(nop 1 2 3)";

    DummyInStream stream;
    StreamReader reader{MutDataSpan{&code[0], sizeof(code)}, stream, sizeof(code)};

    Compiler compiler{env};
    auto res = compiler.Build(reader, allocator);
    ASSERT_TRUE(res.IsRight());
}