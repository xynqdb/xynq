#include "base/span.h"
#include "containers/str.h"
#include "containers/vec.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(SpanTest, DefaultCtor) {
    DataSpan v{};
    ASSERT_EQ(v.Data(), nullptr);
    ASSERT_EQ(v.Size(), 0u);
}

TEST(SpanTest, CtorWithDataSize) {
    const char test_str[] = "test";
    DataSpan v{test_str, 5};
    ASSERT_STREQ((const char *)v.Data(), test_str);
    ASSERT_EQ(v.Size(), 5u);
}

TEST(SpanTest, CtorWithBeginEnd) {
    const char test_str[] = "test";
    DataSpan v{std::begin(test_str), std::end(test_str)};
    ASSERT_STREQ((const char *)v.Data(), test_str);
    ASSERT_EQ(v.Size(), 5u);
}

TEST(SpanTest, CtorWithStdContainer) {
    Vec<int> vec{0, 1,2,3,4,5,6,7,8,9};

    Span<int> span(vec);
    ASSERT_EQ(span.Size(), 10u);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(i, vec[i]);
    }
}

TEST(SpanTest, Iterate) {
    uint8_t bytes[] = {1,2,3,4,5};
    ByteSpan v{&bytes[0], 5};

    size_t i = 0;
    for (auto b : v) {
        ASSERT_LE(i, 5u);
        ASSERT_EQ(bytes[i], b);

        ++i;
    }
}

TEST(SpanTest, DataSpanIndexed) {
    const char test_str[] = "test";
    DataSpan v{test_str, 5};
    for (size_t i = 0; i < v.Size(); ++i) {
        ASSERT_EQ(test_str[i], v[i]);
    }
}

TEST(SpanTest, StrSpanIndexed) {
    const char *test_str = "test";
    CStrSpan v{test_str, 4};
    for (size_t i = 0; i < v.Size(); ++i) {
        ASSERT_EQ(test_str[i], v[i]);
    }
}

TEST(SpanTest, MutIterate) {
    uint8_t bytes[] = {0,1,2,3,4};
    MutByteSpan v{&bytes[0], 5};

    for (auto &b : v) {
        ++b;
    }

    size_t i = 0;
    for (auto b : v) {
        ASSERT_LE(i, 5u);
        ASSERT_EQ(bytes[i], b);
        ASSERT_EQ(bytes[i], i + 1);

        ++i;
    }
}

TEST(SpanTest, CStrCtor) {
    CStrSpan s = "test str";
    ASSERT_EQ(s.Size(), 8u);
    ASSERT_STREQ(s.CStr(), "test str");
}

TEST(SpanTest, CStrCtorSize) {
    const char *str = "test str";
    CStrSpan s{str, 8u};
    ASSERT_EQ(s.Size(), 8u);
    ASSERT_STREQ(s.CStr(), "test str");
}

TEST(SpanTest, CStrCtorStartEnd) {
    const char *str_begin = "test str";
    const char *str_end = str_begin + 8;

    CStrSpan s{str_begin, str_end};
    ASSERT_EQ(s.Size(), 8u);
    ASSERT_STREQ(s.CStr(), "test str");
}

TEST(SpanTest, CStrCtorStdString) {
    Str str = "test str";
    CStrSpan s = str;
    ASSERT_EQ(s.Size(), 8u);
    ASSERT_STREQ(s.CStr(), "test str");
}

TEST(SpanTest, StrSpanEq) {
    StrSpan s1{"test_str"};
    StrSpan s2{""};
    ASSERT_TRUE(s1 == "test_str");
    ASSERT_FALSE(s1 == "test_str2");
    ASSERT_FALSE(s1 == "");

    ASSERT_TRUE(s2 == "");
    ASSERT_FALSE(s2 == "test_str");
}