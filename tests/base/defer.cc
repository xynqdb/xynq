#include "base/defer.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(DeferTest, Defer) {
    int x = 1;
    {
        Defer test([&]{
            x = 322;
        });
        ASSERT_EQ(x, 1);
    }

    ASSERT_EQ(x, 322);
}
