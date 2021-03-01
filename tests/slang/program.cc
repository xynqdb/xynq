#include "slang/slang.h"

#include "base/dep.h"
#include "base/stream.h"

#include "gtest/gtest.h"

using namespace xynq;
using namespace xynq::slang;

namespace {

struct TestUserData {
    int nop_count = 0; // number of nop function calls
};

Env CreateTestEnv() {
    FuncTable func_table;
    func_table["nop"] = [](slang::CallContext &cc) {
        XYAssert(cc.user_data != nullptr);
        auto it = cc.args->Begin();
        while (!it.IsEnd()) { // skip all args
            ++it;
        }
        ((TestUserData *)cc.user_data)->nop_count++;
        return true;
    };
    func_table["+"] = [](slang::CallContext &call_context) {
        int64_t sum = 0;
        auto it = call_context.args->Begin();
        while (!it.IsEnd()) {
            int64_t v = it.Get<int64_t>().Value();
            sum += v;
            ++it;
        }

        call_context.output->Add(sum);
        return true;
    };
    func_table["-"] = [](slang::CallContext &call_context) {
        int64_t sum = 0;
        auto it = call_context.args->Begin();
        if (!it.IsEnd()) {
            sum = it.Get<int64_t>().Value();
            ++it;
            while (!it.IsEnd()) {
                int64_t v = it.Get<int64_t>();
                sum -= v;
                ++it;
            }
        }

        call_context.output->Add(sum);
        return true;
    };

    PayloadHandlerTable payload_handlers;
    return slang::Env{std::move(func_table), std::move(payload_handlers)};
}

} // anon namespace


TEST(SlangProgramTest, Error) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    Context context {
        env,
        allocator
    };

    char code[] = "(xxx)";

    DummyInStream in_stream;
    DummyOutStream out_stream;

    StreamReader request_reader(MutDataSpan{&code[0], sizeof(code)}, in_stream, sizeof(code));
    DummySerializer output;
    auto result = slang::Execute(request_reader, output, context);
    ASSERT_TRUE(result.IsLeft());
}

TEST(SlangProgramTest, Nop) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();
    TestUserData user_data;

    Context context {
        env,
        allocator,
        &user_data
    };

    char code[] = "(nop (nop (nop) (nop) (nop (nop (nop)))) (nop) (nop (nop    )))";

    DummyInStream in_stream;
    DummyOutStream out_stream;

    StreamReader request_reader(MutDataSpan{&code[0], sizeof(code)}, in_stream, sizeof(code));
    DummySerializer output;
    auto result = slang::Execute(request_reader, output, context);
    ASSERT_TRUE(result.IsRight());
    ASSERT_EQ(user_data.nop_count, 10);
}

TEST(SlangProgramTest, Sum) {
    Dependable<ScratchAllocator> allocator;
    Dependable<Env> env = CreateTestEnv();

    Context context {
        env,
        allocator
    };

    char code[] = "(+ 100 -1000 900 -9223372036854775808 9223372036854775807 25)";

    DummyInStream in_stream;
    DummyOutStream out_stream;

    StreamReader request_reader(MutDataSpan{&code[0], sizeof(code)}, in_stream, sizeof(code));
    DummySerializer output;
    auto result = slang::Execute(request_reader, output, context);
    ASSERT_TRUE(result.IsRight());
}