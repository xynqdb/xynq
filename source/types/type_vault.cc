#include "type_vault.h"
#include "base/log.h"

using namespace xynq;
using namespace xynq::detail;

DefineTaggedLog(TypeVault);


// TypeVault.
TypeVault::TypeVault(TypeManager &shared_type_vault, Dep<Log> log, Dep<BaseAllocator> allocator)
    : type_manager_(shared_type_vault)
    , log_(log)
    , allocator_(allocator) {
}

bool TypeVault::HasType(StrSpan type_name) const {
    return FindSchema(type_name) != k_types_invalid_schema;
}

TypeSchemaPtr TypeVault::FindSchema(StrSpan type_name) const {
    if (!updated_.test_and_set(std::memory_order_relaxed)) { // types map cache is dirty.
        type_manager_.RunShared([&] {
            TypeList::NodePtr node = current_ ? current_->next : type_manager_.types_list_.Head();
            while (node != nullptr) {
                schemas_map_.insert({node->schema->name, node->schema});
                node = node->next;
            }
            current_ = type_manager_.types_list_.Last();
        });
    }

    auto it = schemas_map_.find(type_name);
    return it == schemas_map_.end() ? k_types_invalid_schema : it->second;
}

void TypeVault::InvalidateCache() {
    updated_.clear(std::memory_order_relaxed);
}


// TypeManager.
TypeManager::TypeManager(Dep<Log> log,
                         Dep<BaseAllocator> allocator,
                         std::initializer_list<TypeSchemaPtr> initial_types)
    : log_(log)
    , allocator_(allocator)
    , vaults_(allocator_) {

    for (TypeSchemaPtr schema : initial_types) {
        EnqueueSchema(schema, false);
    }
}

Dep<TypeVault> TypeManager::CreateVault(Dep<Log> log) {
    vaults_.emplace_back(CreateObject<TypeVault>(*allocator_, *this, log, allocator_));
    return vaults_.back();
}

std::pair<TypeSchema *, char *> TypeManager::AllocateSchema(StrSpan type_name, size_t num_fields, size_t fields_buf_size) {
    size_t schema_sz = sizeof(TypeSchema) + num_fields * sizeof(FieldSchema);
    void *buf = allocator_->AllocAligned(alignof(TypeSchema), schema_sz + type_name.Size() + fields_buf_size);
    char *str_buf = (char *)buf + schema_sz;

    TypeSchema *schema = new(buf) TypeSchema;
    memcpy(str_buf, type_name.Data(), type_name.Size());
    schema->name = StrSpan{str_buf, type_name.Size()};
    str_buf += type_name.Size();
    schema->field_count = num_fields;
    return {schema, str_buf};
}

void TypeManager::DeallocateSchema(TypeSchema *schema) {
    allocator_->Free(schema);
}

TypeManager::~TypeManager() {
    while (TypeList::NodePtr node = types_list_.PopFront()) {
        if (node->owned) {
            DeallocateSchema(node->schema);
        }
        DestroyObject(*allocator_, node);
    }
}

void TypeManager::EnqueueSchema(TypeSchema *schema, bool owned) {
    XYAssert(schema != nullptr);

    TypeList::NodePtr node = CreateObject<TypeList::NodeType>(*allocator_);
    node->owned = owned;
    node->schema = schema;
    types_list_.PushBack(node);

    XYTypeVaultInfo(log_, "Added new type '", schema->name, "'");

    // Now mark all TypeVaults dirty.
    InvalidateCaches();
}

void TypeManager::InvalidateCaches() {
    for (Dep<TypeVault> vault : vaults_) {
        vault->InvalidateCache();
    }
}
