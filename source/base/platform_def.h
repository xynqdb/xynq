#pragma once

// Ideally all platform dependend stuff should go into platform module
// instead of putting code under ifdefs. However it's not always simple to do.
// When not possible use ifdef XYNQ_* platform defines ie. XYNQ_APPLE, XYNQ_LINUX etc.

namespace xynq {
// Cache line size in bytes.
// As of clang-11 std::hardware_destructive_interference_size is not supported so
// hardcode instead.
static constexpr unsigned k_cache_line_size = 64;
} // xynq

#if __APPLE__
    #define XYNQ_UNIX 1
    #define XYNQ_APPLE 1
    #include <TargetConditionals.h>

    #if TARGET_OS_MAC
        #define XYNQ_MAC 1
    #else
        #error "Unsupported Apple platform"
    #endif
#endif

#if __linux__
    #define XYNQ_LINUX 1
    #define XYNQ_UNIX 1
#endif

#if defined(DEBUG) || defined(_DEBUG)
    #define XYNQ_DEBUG 1
    #define XYNQ_ENABLE_DEBUG_ASSERTS 1
    #define XYNQ_BUILD_FLAVOUR "dbg"
#else
    #define XYNQ_RELEASE 1
    #define XYNQ_BUILD_FLAVOUR "rel"
#endif

#define XYNQ_EXPECT_TRUE(cond) __builtin_expect(!(cond), 0)

#if defined(XYNQ_DEBUG)
    #define XYNQ_TRACK_DEP_LIFETIME
#endif

#define XYNQ_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define XYNQ_PRETTY_FUNCTION_STR #__PRETTY_FUNCTION__

#if defined(XYNQ_UNIX)
    #define XYNQ_PATH_SEP '/'
#endif
