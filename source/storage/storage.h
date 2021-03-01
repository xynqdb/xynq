#pragma once

#include "object.h"
#include "object_vault.h"

#include "base/either.h"
#include "base/span.h"
#include "containers/hash.h"
#include "types/type_vault.h"

namespace xynq {
class Storage;

class Storage {
public:
    Storage();
    ~Storage();

    Either<StrSpan, std::pair<Object::Handle, TypeSchemaPtr>> CreateObject(StrSpan type_name);
    Either<StrSpan, std::pair<Object::Handle, TypeSchemaPtr>> CreateObject(TypeSchemaPtr schema = k_types_invalid_schema);

    Object *LockObject(Object::Handle object_handle);
    void UnlockObject(Object::Handle object_handle);

    // Enumerate all objects of the type type_name.
    template<class T>
    inline bool Enumerate(StrSpan type_name, T handler);

    // TEMP.
    // Finds object vault for type. If doesn't exist but type is registered, will create a storage for it.
    ObjectVault *EnsureVaultWithType(Dep<TypeVault> types, StrSpan type_name);
private:
    std::recursive_mutex lock_; // TEMP
    Dep<TypeVault> types_;
    HashMap<StrSpan, ObjectVault*> objects_; // types to objects list.

    ObjectVault *FindVaultWithType(StrSpan type_name);
};

// Implementation.
template<class T>
bool Storage::Enumerate(StrSpan type_name, T handler) {
    ObjectVault *obj_vault = FindVaultWithType(type_name);
    if (obj_vault == nullptr) {
        return false;
    }

    obj_vault->Enumerate<T>(handler);
    return true;
}

} // xynq
