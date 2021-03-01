#include "fileutils.h"

#include "containers/str.h"
#include "base/platform_def.h"

using namespace xynq;
using namespace xynq::fileutils;

CStrSpan xynq::fileutils::ReplaceFilename(StrSpan path, StrSpan new_filename, ScratchAllocator *allocator) {
    ScratchStr str(allocator);
    str.reserve(path.Size() + new_filename.Size() + 1);

    if ((path.Size() == 1 && (path[0] == '.' || path[0] == '~'))
        || (path.Size() == 2 && path[0] == '.' && path[1] == '.')) {
        str.assign(path.Data(), path.Size());
        str += XYNQ_PATH_SEP;
        str.append(new_filename.Data(), new_filename.Size());
        return CStrSpan{str.c_str(), str.size()};
    }

    size_t path_size = path.Size();
    const char *cur = path.Data() + path_size;
    while (cur-- != path.Data() && *cur != XYNQ_PATH_SEP) {
        --path_size;
    }

    str.assign(path.Data(), path_size);
    str.append(new_filename.Data(), new_filename.Size());
    return CStrSpan{str.c_str(), str.size()};
}