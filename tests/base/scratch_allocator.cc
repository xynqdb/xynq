#include "base/scratch_allocator.h"
#include "containers/str.h"
#include "containers/vec.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(ScratchAllocatorTest, Alloc) {
    ScratchAllocator allocator{1024};

    void *ptr = allocator.Alloc(512);
    ASSERT_TRUE(ptr != nullptr);
    allocator.Free(ptr);
}

TEST(ScratchAllocatorTest, Alloc2) {
    ScratchAllocator allocator{1024};

    void *ptr = allocator.Alloc(2048);
    ASSERT_TRUE(ptr != nullptr);
    allocator.Free(ptr);
}

TEST(ScratchAllocatorTest, AllocAligned) {
    ScratchAllocator allocator{1024};

    void *ptr = allocator.AllocAligned(512, 50);
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_EQ(((uintptr_t)ptr) % 512, 0ul);
    allocator.Free(ptr);
}

TEST(ScratchAllocatorTest, AllocAlignedMultipleChunks) {
    ScratchAllocator allocator{1024};

    void *ptr1 = allocator.Alloc(512);
    void *ptr2 = allocator.Alloc(512);
    void *ptr3 = allocator.Alloc(512);
    void *ptr4 = allocator.Alloc(512);
    ASSERT_TRUE(ptr1 != nullptr);
    ASSERT_TRUE(ptr2 != nullptr);
    ASSERT_TRUE(ptr3 != nullptr);
    ASSERT_TRUE(ptr4 != nullptr);
}

TEST(ScratchAllocatorTest, AllocScoped) {
    ScratchAllocator allocator{1024};

    ASSERT_EQ(allocator.SizeAllocated(), 0u);
    allocator.Scoped([](ScratchAllocator &allocator) {
        allocator.Alloc(2048);
    });
    ASSERT_EQ(allocator.SizeAllocated(), 0u);
}

TEST(ScratchAllocatorTest, String) {
    ScratchAllocator allocator{1024};
    ScratchStr str{&allocator};
    str = "Test";
    str += "Str";
    str += "0123456789012345678901234567890123456789";
    ASSERT_STREQ(str.c_str(), "TestStr0123456789012345678901234567890123456789");
}

TEST(ScratchAllocatorTest, Vector) {
    ScratchAllocator allocator{1024};
    ScratchVec<int> vec{&allocator};

    vec.resize(100);
    for (int i = 0; i < 100; ++i) {
        vec[i] = i;
    }

    for (int i = 0; i < 100; ++i) {
        ASSERT_EQ(vec[i], i);
    }
}

TEST(ScratchAllocatorTest, AssignStr) {
    ScratchAllocator allocator1{128};
    ScratchAllocator allocator2{128};

    ScratchStr s1{&allocator1};
    ScratchStr s2{&allocator2};

    s2 = "test str 3256!";

    s1 = s2;
    ASSERT_EQ(s1.get_allocator().allocator_, &allocator1);
    ASSERT_STREQ(s1.c_str(), "test str 3256!");
}
