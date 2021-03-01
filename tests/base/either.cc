#include "base/either.h"
#include "base/str_builder.h"
#include "containers/str.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(EitherTest, InitLeft) {
    Either<int, bool> e = 123456;

    ASSERT_TRUE(e.IsLeft());
    ASSERT_FALSE(e.IsRight());
    ASSERT_EQ(e.Left(), 123456);
}

TEST(EitherTest, InitRight) {
    Either<int, bool> e = true;

    ASSERT_FALSE(e.IsLeft());
    ASSERT_TRUE(e.IsRight());
    ASSERT_TRUE(e.Right());
}

TEST(EitherTest, Copy) {
    Either<int, bool> e = true;
    auto e_copy = e;

    ASSERT_FALSE(e_copy.IsLeft());
    ASSERT_TRUE(e_copy.IsRight());
    ASSERT_TRUE(e_copy.Right());
}

TEST(EitherTest, MoveCtor1) {
    struct MoveTestObj
    {
        const char *str_ = nullptr;

        explicit MoveTestObj(const char *str)
            : str_(str)
        {}

        MoveTestObj(MoveTestObj &&other) {
            str_ = other.str_;
            other.str_ = nullptr;
        }
    };

    Either<int, MoveTestObj> e = MoveTestObj{"MoveCtor"};
    auto e_moved = std::move(e);

    ASSERT_EQ(e.Right().str_, nullptr);
    ASSERT_STREQ(e_moved.Right().str_, "MoveCtor");
}

TEST(EitherTest, MoveCtor2) {
    struct MoveTestObj
    {
        const char *str_ = nullptr;

        explicit MoveTestObj(const char *str)
            : str_(str)
        {}

        MoveTestObj(MoveTestObj &&other) {
            str_ = other.str_;
            other.str_ = nullptr;
        }
    };

    MoveTestObj t("MoveCtor");
    Either<MoveTestObj, int> e = std::move(t);
    ASSERT_EQ(t.str_, nullptr);
    ASSERT_STREQ(e.Left().str_, "MoveCtor");
}

TEST(EitherTest, TempVar) {
    struct TempTestObj
    {
        const char *str_ = nullptr;

        // No copying.
        TempTestObj(const TempTestObj &) = delete;
        TempTestObj& operator=(const TempTestObj &) = delete;

        explicit TempTestObj(const char *str)
            : str_(str)
        {}

        TempTestObj(TempTestObj &&other) {
            str_ = other.str_;
            other.str_ = nullptr;
        }
    };

    Either<TempTestObj, int> e = TempTestObj{"TempVar"};
    ASSERT_STREQ(e.Left().str_, "TempVar");
}

TEST(EitherTest, Dtor) {
    int dtor_called = 0;

    struct DtorTestObj
    {
        int &dtor_called_;

        DtorTestObj(int &dtor_var)
            : dtor_called_(dtor_var)
        {}

        ~DtorTestObj() { ++dtor_called_; }
    };

    {
        Either<DtorTestObj, int> e = DtorTestObj(dtor_called);
        (void)e;
    }

    ASSERT_EQ(dtor_called, 2); // One for temp, and one for either
}

TEST(EitherTest, Fold1) {
    Either<int, bool> e = 123456;

    int result = e.Fold([](int x) -> int {
        return x;
    },
                        [](bool x) -> int {
        return (int)x;
    });

    ASSERT_EQ(result, 123456);
}

TEST(EitherTest, Fold2) {
    Either<int, bool> e = true;

    int result = e.Fold([](int x) -> int {
        return x;
    },
                        [](bool x) -> int {
        return (int)x;
    });

    ASSERT_EQ(result, 1);
}

TEST(EitherTest, Fold3) {
    Either<int, bool> e = true;

    int result = e.Fold(
        []() -> int {
            return 576;
        },
        []() -> int {
            return 576;
        });

    ASSERT_EQ(result, 576);
}

TEST(EitherTest, FoldLeftWithArg) {
    Either<int, bool> e = 123;

    bool result = e.FoldLeft([](int x) {
        EXPECT_EQ(x, 123);
        return true;
    });

    ASSERT_EQ(result, true);
}

TEST(EitherTest, FoldLeftNoArg) {
    Either<int, bool> e = 123;

    bool result = e.FoldLeft([]() {
        return true;
    });

    ASSERT_EQ(result, true);
}

TEST(EitherTest, FoldRightWithArg) {
    Either<int, bool> e = true;

    int result = e.FoldRight([](bool x) {
        EXPECT_EQ(x, true);
        return 123;
    });

    ASSERT_EQ(result, 123);
}

TEST(EitherTest, FoldRightNoArg) {
    Either<int, bool> e = true;

    int result = e.FoldRight([]() {
        return 123;
    });

    ASSERT_EQ(result, 123);
}

TEST(EitherTest, LeftOrDefault1) {
    Either<int, bool> e = true;

    int result = e.LeftOrDefault(326);

    ASSERT_EQ(result, 326);
}

TEST(EitherTest, LeftOrDefault2) {
    Either<int, bool> e = 329;

    int result = e.LeftOrDefault(326);

    ASSERT_EQ(result, 329);
}

TEST(EitherTest, RightOrDefault1) {
    Either<int, bool> e = 333;

    bool result = e.RightOrDefault(true);
    ASSERT_TRUE(result);
}

TEST(EitherTest, RightOrDefault2) {
    Either<int, bool> e = true;

    bool result = e.RightOrDefault(false);
    ASSERT_TRUE(result);
}

TEST(EitherTest, MapLeft) {
    Either<int, bool> l = 1;
    Either<int, bool> r = true;

    auto mapped_left = l.MapLeft([](int x) {
        return Str{"test"} + StrBuilder<32>{x}.Buffer();
    });

    auto mapped_right = r.MapLeft([](int x) {
        return Str{"test"} + StrBuilder<32>{x}.Buffer();
    });

    ASSERT_TRUE(mapped_left.IsLeft());
    ASSERT_STREQ(mapped_left.Left().c_str(), "test1");
    ASSERT_TRUE(mapped_right.IsRight());
    ASSERT_EQ(mapped_right.Right(), true);
}

TEST(EitherTest, MapRight) {
    Either<int, bool> l = 1;
    Either<int, bool> r = true;

    auto mapped_left = l.MapRight([](int x) {
        return std::string{"test"} + std::to_string(x);
    });

    auto mapped_right = r.MapRight([](bool x) {
        return std::string{"test"} + std::to_string((int)x);
    });

    ASSERT_TRUE(mapped_left.IsLeft());
    ASSERT_EQ(mapped_left.Left(), 1);
    ASSERT_TRUE(mapped_right.IsRight());
    ASSERT_STREQ(mapped_right.Right().c_str(), "test1");
}

TEST(EitherTest, JoinLeft) {
    Either<Either<int, bool>, bool> l = Either<int, bool>{25};
    Either<Either<int, bool>, bool> r = true;

    auto lj = l.JoinLeft();
    ASSERT_TRUE(lj.IsLeft());
    ASSERT_EQ(lj.Left(), 25);

    auto rj = r.JoinLeft();
    ASSERT_TRUE(rj.IsRight());
    ASSERT_EQ(rj.Right(), true);
}

TEST(EitherTest, JoinRight) {
    Either<int, Either<int, bool>> l = 25;
    Either<int, Either<int, bool>> r = Either<int, bool>{true};

    auto lj = l.JoinRight();
    ASSERT_TRUE(lj.IsLeft());
    ASSERT_EQ(lj.Left(), 25);

    auto rj = r.JoinRight();
    ASSERT_TRUE(rj.IsRight());
    ASSERT_EQ(rj.Right(), true);
}
