#include "base/str_builder.h"

#include "gtest/gtest.h"

#include <limits.h>

using namespace xynq;

TEST(StrBuilder, CtorWithArgs) {
    StrBuilder<164> builder{true, ' ', "strSTRstr", ' ', 325};
    ASSERT_STREQ(builder.MakeCStr().CStr(), "Y strSTRstr 325");
}

TEST(StrBuilder, Bool) {
    StrBuilder<64> builder;
    builder << true;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "Y");
}

TEST(StrBuilder, UChar) {
    StrBuilder<64> builder;
    builder << (unsigned char)225;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "225");
}

TEST(StrBuilder, UShort) {
    StrBuilder<64> builder;
    builder << (unsigned short)65535;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "65535");
}

TEST(StrBuilder, UInt) {
    StrBuilder<64> builder;
    builder << 4294967295u;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "4294967295");
}

TEST(StrBuilder, ULongLong) {
    StrBuilder<64> builder;
    builder << 18446744073709551615ull;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "18446744073709551615");
}

TEST(StrBuilder, ULongLongZero) {
    StrBuilder<64> builder;
    builder << 0;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "0");
}

TEST(StrBuilder, Char) {
    StrBuilder<64> builder;
    builder << 'a';
    ASSERT_STREQ(builder.MakeCStr().CStr(), "a");
}

TEST(StrBuilder, Int) {
    StrBuilder<64> builder;
    builder << -2147483648;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "-2147483648");
}

TEST(StrBuilder, IntZero) {
    StrBuilder<64> builder;
    builder << 0;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "0");
}

TEST(StrBuilder, Short) {
    StrBuilder<64> builder;
    builder << (short)(-32768);
    ASSERT_STREQ(builder.MakeCStr().CStr(), "-32768");
}

TEST(StrBuilder, LongLong) {
    StrBuilder<64> builder;
    builder << -9223372036854775807ll - 1ll;
    ASSERT_STREQ(builder.MakeCStr().CStr(), "-9223372036854775808");
}

TEST(StrBuilder, CStrSpan) {
    StrBuilder<64> builder;
    builder << CStrSpan{"Hello 567"};
    ASSERT_STREQ(builder.MakeCStr().CStr(), "Hello 567");
}

TEST(StrBuilder, ConstChars) {
    StrBuilder<64> builder;
    builder << "Hello 567";
    ASSERT_STREQ(builder.MakeCStr().CStr(), "Hello 567");
}

TEST(StrBuilder, Hex) {
    StrBuilder<64> builder;
    builder << StrHex{0x325abc};
    ASSERT_STREQ(builder.MakeCStr().CStr(), "0x325abc");
}

TEST(StrBuilder, DoubleInf) {
    StrBuilder<64> b1, b2;
    b1 << std::numeric_limits<double>::infinity();
    b2 << -1 * std::numeric_limits<double>::infinity();
    ASSERT_STREQ(b1.MakeCStr().CStr(), "inf");
    ASSERT_STREQ(b2.MakeCStr().CStr(), "-inf");
}

TEST(StrBuilder, DoubleNan) {
    StrBuilder<64> b1;
    b1 << std::numeric_limits<double>::quiet_NaN();
    ASSERT_STREQ(b1.MakeCStr().CStr(), "nan");
}

TEST(StrBuilder, DoubleNumber) {
    StrBuilder<64> b1;
    b1 << 0.625;
    ASSERT_STREQ(b1.MakeCStr().CStr(), "0.625");
}

TEST(StrBuilder, DoubleNumberLonger) {
    StrBuilder<64> b1, b2, b3;
    b1 << 320.625;
    b2 << 999999.0;
    b3 << 0.015625;
    ASSERT_STREQ(b1.MakeCStr().CStr(), "320.625");
    ASSERT_STREQ(b2.MakeCStr().CStr(), "999999.0");
    ASSERT_STREQ(b3.MakeCStr().CStr(), "0.015625");
}

TEST(StrBuilder, DoubleNegativeNumber) {
    StrBuilder<64> b1, b2;
    b1 << -0.625;
    b2 << -32.15625;
    ASSERT_STREQ(b1.MakeCStr().CStr(), "-0.625");
    ASSERT_STREQ(b2.MakeCStr().CStr(), "-32.15625");
}

TEST(StrBuilder, DoubleScientific) {
    StrBuilder<64> b1, b2, b3,b4;
    b1 << 320e10;
    b2 << 1.0 / 3e7;
    b3 << std::numeric_limits<double>::max();
    b4 << std::numeric_limits<double>::min();
    ASSERT_STREQ(b1.MakeCStr().CStr(), "3.2e12");
    ASSERT_STREQ(b2.MakeCStr().CStr(), "3.333333e-8");
    ASSERT_STREQ(b3.MakeCStr().CStr(), "1.797693e308");
    ASSERT_STREQ(b4.MakeCStr().CStr(), "2.225073e-308");
}

TEST(StrBuilder, DoubleWithPrecision) {
    StrBuilder<64> b1;
    b1 << StrPrecision{26.03125, 2};
    ASSERT_STREQ(b1.MakeCStr().CStr(), "26.03");
}

TEST(StrBuilder, Float) {
    StrBuilder<64> b1;
    b1 << 0.625f;
    ASSERT_STREQ(b1.MakeCStr().CStr(), "0.625");
}

TEST(StrBuilder, FloatWithPrecision) {
    StrBuilder<64> b1;
    b1 << StrPrecision{26.015625f, 3};
    ASSERT_STREQ(b1.MakeCStr().CStr(), "26.015");
}

TEST(StrBuilder, Ptr) {
    StrBuilder<64> builder;
    builder << &builder;
    auto str = builder.Buffer();
    ASSERT_GT(str.Size(), 2u);
    ASSERT_EQ(str[0], '0');
    ASSERT_EQ(str[1], 'x');
}

TEST(StrBuilder, CutOff1) {
    StrBuilder<4> builder;
    builder << 123456;
    ASSERT_LT(builder.Buffer().Size(), 4u);
}

TEST(StrBuilder, CutOff2) {
    StrBuilder<4> builder;
    builder << "str str str";
    ASSERT_LT(builder.Buffer().Size(), 4u);
}
