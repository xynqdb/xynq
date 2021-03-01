#include "storage.h"

using namespace xynq;

Storage::Storage() {
}

Storage::~Storage() {
    for (auto &v : objects_) {
        delete v.second;
    }
}

Either<StrSpan, std::pair<Object::Handle, TypeSchemaPtr>> Storage::CreateObject(StrSpan type_name) {
    auto it = objects_.find(type_name);
    if (it == objects_.end() || it->second == nullptr) {
        return StrSpan{"No storage for type"};
    }

    ObjectVault *vault = it->second;
    return vault->CreateObject().MapRight([&](Object::Handle obj) {
        return std::make_pair(obj, vault->Schema());
    });
}

Either<StrSpan, std::pair<Object::Handle, TypeSchemaPtr>> Storage::CreateObject(TypeSchemaPtr schema) {
    return CreateObject(schema->name);
}

Object *Storage::LockObject(Object::Handle object_handle) {
    // TEMP: for now just returning raw ptr
    return object_handle;
}

void Storage::UnlockObject(Object::Handle object_handle) {
}

ObjectVault *Storage::FindVaultWithType(StrSpan type_name) {
    std::lock_guard l(lock_);

    auto it = objects_.find(type_name);
    if (it != objects_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

ObjectVault *Storage::EnsureVaultWithType(Dep<TypeVault> types, StrSpan type_name) {
    std::lock_guard l(lock_);

    auto it = objects_.find(type_name);
    if (it == objects_.end()) {
        TypeSchemaPtr schema = types->FindSchema(type_name);
        if (schema == k_types_invalid_schema || schema->IsBasic()) {
            return nullptr;
        }

        ObjectVault *vault = new ObjectVault{schema};
        objects_.insert(std::make_pair(schema->name, vault));
        return vault;
    } else {
        return it->second;
    }
}