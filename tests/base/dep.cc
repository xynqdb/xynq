#include "base/dep.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(DepTest, DependableWithValue) {
    struct Obj { int x = 0; };
    Obj obj;
    obj.x = 325;

    Dependable<Obj> dependable = std::move(obj);
    ASSERT_EQ(dependable->x, 325);
}

TEST(DepTest, DependableWithPtr) {
    struct Obj { int x = 0; };
    Obj *obj = new Obj{};
    obj->x = 325;

    Dependable<Obj *> dependable = obj;
    ASSERT_EQ(dependable->x, 325);
}
