#include "env.h"

using namespace xynq;
using namespace xynq::slang;

Env::Env(FuncTable &&functions, PayloadHandlerTable &&payload_handlers)
    : functions_(functions)
    , payload_handlers_(payload_handlers) {

}

Call Env::FindCall(StrSpan name) const {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return nullptr;
    }

    return it->second;
}

PayloadHandler *Env::FindPayloadHandler(uint32_t token) {
    auto it = payload_handlers_.find(token);
    if (it == payload_handlers_.end()) {
        return nullptr;
    }

    return it->second;
}