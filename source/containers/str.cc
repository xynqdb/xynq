#include "str.h"

using namespace xynq;

CStrSpan xynq::MakeScratchCStrCopy(StrSpan str, ScratchAllocator &allocator) {
    char *buf = static_cast<char *>(allocator.Alloc(str.Size() + 1));
    memcpy(buf, str.Data(), str.Size());
    buf[str.Size()] = '\0';
    return CStrSpan{buf, str.Size()};
}

StrSpan xynq::MakeScratchStrCopy(StrSpan str, ScratchAllocator &allocator) {
    char *buf = static_cast<char *>(allocator.Alloc(str.Size()));
    memcpy(buf, str.Data(), str.Size());
    return CStrSpan{buf, str.Size()};
}
