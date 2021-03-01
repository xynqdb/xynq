#pragma once

#include "slang/env.h"
#include "json_payload_handler.h"

namespace xynq {

slang::Env CreateSlangEnv(Dep<JsonPayloadHandler> json_payload_handler);

} // xynq
