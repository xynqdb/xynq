#pragma once

#include "span.h"
#include "base/scratch_allocator.h"

namespace xynq {
namespace fileutils {
    // Replaces filename in a path.
    // ie. path=/temp/hello, new_filename=hi -> will return /temp/hi
    CStrSpan ReplaceFilename(StrSpan path, StrSpan filename, ScratchAllocator *allocator);
}

}