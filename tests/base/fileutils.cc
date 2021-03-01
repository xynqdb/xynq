#include "base/fileutils.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(FileUtilsTest, ReplaceFilename) {
    ScratchAllocator allocator;
    CStrSpan replaced = fileutils::ReplaceFilename("/temp/subdir/test/hello", "new", &allocator);

    ASSERT_STREQ(replaced.CStr(), "/temp/subdir/test/new");
}

TEST(FileUtilsTest, ReplaceFilenameNoFolder) {
    ScratchAllocator allocator;
    CStrSpan replaced = fileutils::ReplaceFilename("hello", "new", &allocator);

    ASSERT_STREQ(replaced.CStr(), "new");
}

TEST(FileUtilsTest, ReplaceFilenameEmpty) {
    ScratchAllocator allocator;
    CStrSpan replaced = fileutils::ReplaceFilename("", "new", &allocator);

    ASSERT_STREQ(replaced.CStr(), "new");
}

TEST(FileUtilsTest, ReplaceFilenameDots) {
    ScratchAllocator allocator;
    CStrSpan replaced = fileutils::ReplaceFilename("..", "new", &allocator);

    ASSERT_STREQ(replaced.CStr(), "../new");
}
