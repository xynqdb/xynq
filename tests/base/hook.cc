#include "base/hook.h"
#include "containers/str.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(HookTest, SingleNoArgs) {
    Hook h;
    int result = 0;
    h.Add([&]{
        result = 1;
    });

    h.Invoke();
    ASSERT_EQ(result, 1);
}

TEST(HookTest, MultipleNoArgs) {
    Hook h;
    int result = 0;
    h.Add([&]{ ++result; });
    h.Add([&]{ ++result; });
    h.Add([&]{ ++result; });
    h.Add([&]{ ++result; });

    h.Invoke();
    ASSERT_EQ(result, 4);
}

TEST(HookTest, SingleWithArgs) {
    Hook<int> h;
    int result = 0;
    h.Add([&](int x) { result = x; });

    h.Invoke(251);
    ASSERT_EQ(result, 251);
}

TEST(HookTest, MultipleWithArgs) {
    Hook<int, Str> h;
    int int_result = 0;
    Str str_result;
    h.Add([&](int x, Str s) { int_result += x; str_result += s; });
    h.Add([&](int x, Str s) { int_result += x; str_result += s; });
    h.Add([&](int x, Str s) { int_result += x; str_result += s; });
    h.Add([&](int x, Str s) { int_result += x; str_result += s; });

    h.Invoke(251, "ab");
    ASSERT_EQ(int_result, 251 * 4);
    ASSERT_STREQ(str_result.c_str(), "abababab");
}

TEST(HookTest, MultipleWithConstString) {
    Hook<const Str&> h;
    Str str_result;
    h.Add([&](const Str &s) { str_result += s; });
    h.Add([&](const Str &s) { str_result += s; });
    h.Add([&](const Str &s) { str_result += s; });
    h.Add([&](const Str &s) { str_result += s; });

    h.Invoke("ab");
    ASSERT_STREQ(str_result.c_str(), "abababab");
}

TEST(HookTest, Clear) {
    Hook<int> h;
    int result = 0;
    h.Add([&](int x) { result += x; });
    h.Add([&](int x) { result += x; });
    h.Add([&](int x) { result += x; });
    h.Add([&](int x) { result += x; });

    h.Clear();
    h.Invoke(251);
    ASSERT_EQ(result, 0);
}

TEST(HookTest, Ref) {
    Hook<int&> h;
    int result = 2;
    h.Add([](int &x) { x += x; });
    h.Add([](int &x) { x += x; });
    h.Add([](int &x) { x += x; });
    h.Add([](int &x) { x += x; });

    h.Invoke(result);
    ASSERT_EQ(result, 32);
}
