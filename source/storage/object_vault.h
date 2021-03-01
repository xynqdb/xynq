#pragma once

#include "object.h"
#include "base/either.h"
#include "containers/hash.h"
#include "containers/vec.h"
#include "types/schema.h"

#include <mutex>

namespace xynq {

// Storage for objects of the same type.
// Can be thought of as a column in a columnar databases.
class ObjectVault {
public:
    explicit ObjectVault(TypeSchemaPtr schema);

    // Enumerates all objects in the store.
    template<class T>
    inline void Enumerate(T handler);

    // Creates new object and puts it into vault.
    Either<StrSpan, Object::Handle> CreateObject();

    // Returns objects schema.
    TypeSchemaPtr Schema() const { return schema_; }
private:
    std::mutex m_lock;

    // Schema if objects have it.
    TypeSchemaPtr schema_;

    // Actual object data.
    Vec<Object*> store_;

    // Index by objects ids
    HashMap<ObjectGuid, Object::Handle> object_id_index_;
};

template<class T>
void ObjectVault::Enumerate(T handler) {
    std::lock_guard guard(m_lock);

    for (auto &object : store_) {
        handler(object, schema_);
    }
}

} // xynq