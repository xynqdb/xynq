#include "object_vault.h"

using namespace xynq;

ObjectVault::ObjectVault(TypeSchemaPtr schema)
    : schema_(schema) {
    XYAssert(schema_ != k_types_invalid_schema);
}

Either<StrSpan, Object::Handle> ObjectVault::CreateObject() {
    std::lock_guard<std::mutex> l(m_lock);
    static uint64_t x = 0;
    Object *obj = new Object;
    obj->guid_ = ++x;
    store_.push_back(obj);
    object_id_index_[obj->guid_] = obj;
    return obj;
}