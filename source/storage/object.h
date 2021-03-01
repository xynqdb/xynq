#pragma once

#include "base/dep.h"

#include "types/schema.h"
#include "types/basic_types.h"

namespace xynq {

class StreamWriter;

using ObjectGuid = uint64_t;

class Object {
public:
    using Handle = Object*;
    ObjectGuid guid_;

    alignas(128) char buf[256]; // TEMP HACK.

    void * Data() { return &buf[0]; }
};

} // xynq
