#pragma once

#include "slang/env.h"

#include "base/dep.h"
#include "storage/storage.h"
#include "types/type_vault.h"

namespace xynq {

struct SharedDeps {
    Dep<slang::Env> slang_env;
    Dep<Storage> storage;
    Dep<TypeVault> types;
};

} // xynq
