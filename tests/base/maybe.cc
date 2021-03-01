#include "base/maybe.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(MaybeTest, DefaultCtor) {
    Maybe<int> m;
    ASSERT_FALSE(m.HasValue());
}

TEST(MaybeTest, ValueCtor) {
    Maybe<int> m = 333;
    ASSERT_TRUE(m.HasValue());
}

TEST(MaybeTest, ValueGet) {
    Maybe<int> m = 333;
    ASSERT_EQ(m.Get(), 333);
}

TEST(MaybeTest, ValueDefault1) {
    Maybe<int> m = {};
    ASSERT_EQ(m.GetOrDefault(325), 325);
}

TEST(MaybeTest, ValueDefault2) {
    Maybe<int> m = 312;
    ASSERT_EQ(m.GetOrDefault(325), 312);
}

TEST(MaybeTest, Fold1) {
    Maybe<int> m;
    int result = m.Fold([]{
        return 325;
    });
    ASSERT_EQ(result, 325);
}

TEST(MaybeTest, Fold2) {
    Maybe<int> m = 333;
    int result = m.Fold([] {
        return 325;
    });
    ASSERT_EQ(result, 333);
}

TEST(MaybeTest, Map1) {
    Maybe<int> m = 325;
    auto m2 = m.Map([&](int x) -> bool {
        return x == 325;
    });

    ASSERT_TRUE(m2.HasValue());
    ASSERT_TRUE(m2.Get());
}

TEST(MaybeTest, Map2) {
    Maybe<int> m;
    auto m2 = m.Map([&](int x) -> bool {
        return x == 325;
    });

    ASSERT_FALSE(m2.HasValue());
}

TEST(MaybeTest, Bool1) {
    Maybe<int> m;
    ASSERT_FALSE(!!(m));
}

TEST(MaybeTest, Bool2) {
    Maybe<int> m = 325;
    ASSERT_TRUE(!!(m));
}
