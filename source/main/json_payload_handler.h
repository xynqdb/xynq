#pragma once

#include "slang/env.h"
#include "storage/storage.h"

namespace xynq {

class JsonPayloadHandler final : public slang::PayloadHandler {
public:
    JsonPayloadHandler(Dep<Storage> storage);

    slang::PayloadResult ProcessPayload(StreamReader &reader, ScratchAllocator &allocator) override;

private:
    Dep<Storage> storage_;
};

} // namesapce xynq
