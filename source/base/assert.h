#pragma once

#include "base/platform_def.h"

#include <cassert>
#include <cstdlib>

#if defined(XYNQ_ENABLE_DEBUG_ASSERTS)
    #define XYAssert(cond) do { \
        assert((cond)); \
        if (!(cond)) { \
           std::abort(); \
        } \
    } while (0)
#else
    #define XYAssert(cond) XYNQ_EXPECT_TRUE(cond)
#endif
