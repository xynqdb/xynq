#pragma once

#include "schema.h"

#include "base/allocator.h"
#include "base/log.h"
#include "containers/hash.h"
#include "containers/list.h"
#include "containers/vec.h"

#include <atomic>
#include <unistd.h>
#include <shared_mutex>

namespace xynq {

namespace detail {
struct TypeNode {
    TypeSchemaPtr schema;
    bool owned = false;
    ListNodePtr<TypeNode> next;
};
using TypeList = List<TypeNode, &TypeNode::next>;
} // detail

class TypeManager;


// Type storage and mappings.
class TypeVault {
    friend class TypeManager;
public:
    TypeVault(TypeManager &shared_type_vault, Dep<Log> log, Dep<BaseAllocator> allocator);

    // Adds new schema
    // Handler is expected to be callable with bool callable(TypeSchemaPtr, str_buf);
    // Thread-safe.
    template<class HandlerType>
    TypeSchemaPtr CreateSchema(StrSpan type_name, size_t field_count, size_t fields_buf_size, HandlerType handler);

    // Searches for scheme by its type name.
    // Returns k_invalid_scheme if type_name does not exist.
    // Not thread-safe, expected to be called from a single thread.
    TypeSchemaPtr FindSchema(StrSpan type_name) const;

    // Returns true if type is registered within this vault, false otherwise.
    // Not thread-safe, expected to be called from a single thread.
    bool HasType(StrSpan type_name) const;
private:
    alignas(k_cache_line_size)
    mutable std::atomic_flag updated_ = ATOMIC_FLAG_INIT;
    mutable HashMap<StrSpan, TypeSchemaPtr> schemas_map_; // cache for name->schema
    mutable detail::TypeList::NodePtr current_ = nullptr;

    TypeManager &type_manager_;
    Dep<Log> log_;
    Dep<BaseAllocator> allocator_;

    void InvalidateCache();
};


// Stores list of type schemas.
class TypeManager {
    friend class TypeVault;
public:
    TypeManager(Dep<Log> log,
                Dep<BaseAllocator> allocator,
                std::initializer_list<TypeSchemaPtr> initial_types);
    ~TypeManager();

    // Creates lookup cache.
    Dep<TypeVault> CreateVault(Dep<Log> log);
private:
    Dep<Log> log_;
    Dep<BaseAllocator> allocator_;
    Vec<Dependable<TypeVault *>> vaults_;
    std::shared_mutex rw_lock_;
    detail::TypeList types_list_;

    void EnqueueSchema(TypeSchema *schema, bool owned);
    std::pair<TypeSchema *, char *> AllocateSchema(StrSpan, size_t, size_t);
    void DeallocateSchema(TypeSchema *schema);
    void InvalidateCaches();

    // Executes func() in mutual exclusive fashion.
    template<class Func>
    void RunExclusive(Func func);

    // Executes func() in shared fashion (ie. read mode in rw lock)
    template<class Func>
    void RunShared(Func func);
};
////////////////////////////////////////////////////////////


// Implementation.
template<class HandlerType>
TypeSchemaPtr TypeVault::CreateSchema(StrSpan type_name, size_t field_count, size_t fields_buf_size, HandlerType handler) {
    TypeSchemaPtr result = k_types_invalid_schema;
    type_manager_.RunExclusive([&] { // Can only add types exclusevely to avoid duplicated type names.
        // Check for duplicates.
        // This needs to be done under exclusive lock to avoid possible type with the same name
        // inserted concurrently.

        updated_.test_and_set(std::memory_order_acquire); // avoid updating cache right now, we will mark it dirty at the end.
        TypeSchemaPtr dup_schema = FindSchema(type_name);
        if (dup_schema != k_types_invalid_schema) {
            InvalidateCache();
            return;
        }

        auto [new_schema, fields_buf] = type_manager_.AllocateSchema(type_name, field_count, fields_buf_size);
        if (new_schema == nullptr) { // Out Of memory
            InvalidateCache();
            return;
        }

        if (!handler(*new_schema, fields_buf)) {
            type_manager_.DeallocateSchema(new_schema);
            InvalidateCache();
            return;
        }

        type_manager_.EnqueueSchema(new_schema, true);
        result = new_schema;
    });
    return result;
}

template<class Func>
void TypeManager::RunExclusive(Func func) {
    std::lock_guard<std::shared_mutex> g(rw_lock_);
    func();
}

template<class Func>
void TypeManager::RunShared(Func func) {
    std::shared_lock<std::shared_mutex> g(rw_lock_);
    func();
}

}