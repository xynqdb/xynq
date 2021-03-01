#include "gtest/gtest.h"
#include "base/system_allocator.h"

using namespace xynq;

int main(int argc, char **argv) {
    SystemAllocator::Initialize();
    ::testing::InitGoogleTest(&argc, argv);
    int err = RUN_ALL_TESTS();
    SystemAllocator::Shutdown();
    return err;
}
