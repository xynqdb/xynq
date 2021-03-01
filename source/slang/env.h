#pragma once

#include "call.h"

#include "base/dep.h"
#include "base/either.h"
#include "base/scratch_allocator.h"
#include "base/span.h"
#include "base/stream.h"
#include "containers/hash.h"

namespace xynq {
namespace slang {

struct PayloadSuccess{};
using PayloadResult = Either<StrSpan, PayloadSuccess>;

constexpr uint32_t MakePayloadHandlerToken(const char str[4]) {
    return (uint32_t)((str[0] << 24u) | (str[1] << 16u) | (str[2] << 8u) | str[3]);
}

class PayloadHandler {
public:
    virtual ~PayloadHandler() = default;

    virtual PayloadResult ProcessPayload(StreamReader &reader, ScratchAllocator &allocator) = 0;
};

using FuncTable = HashMap<StrSpan, Call>;
using PayloadHandlerTable = HashMap<uint32_t, Dep<PayloadHandler>>;

class Env {
public:
    explicit Env(FuncTable &&functions, PayloadHandlerTable &&payload_handlers);

    Call FindCall(StrSpan name) const;
    PayloadHandler *FindPayloadHandler(uint32_t token);
private:
    FuncTable functions_;
    PayloadHandlerTable payload_handlers_;

};

} // slang
} // xynq
