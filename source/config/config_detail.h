#pragma once

#include "base/span.h"
#include "base/str_builder.h"
#include "base/scratch_allocator.h"
#include "containers/hash.h"

#include <memory>
#include <variant>

namespace xynq {
namespace detail {

using ConfigValue = std::variant<std::monostate, int64_t, double, bool, CStrSpan>;

struct ConfigValueTypeIndex {
    enum {
        Int = 1,
        Double,
        Bool,
        String,
    };
};

template<class T>
struct ConfigTypeIndex {
    static const size_t index = 0;
};

template<>
struct ConfigTypeIndex<int> {
    static const size_t index = ConfigValueTypeIndex::Int;
};

template<>
struct ConfigTypeIndex<int64_t> {
    static const size_t index = ConfigValueTypeIndex::Int;
};

template<>
struct ConfigTypeIndex<size_t> {
    static const size_t index = ConfigValueTypeIndex::Int;
};

template<>
struct ConfigTypeIndex<double> {
    static const size_t index = ConfigValueTypeIndex::Double;
};

template<>
struct ConfigTypeIndex<float> {
    static const size_t index = ConfigValueTypeIndex::Double;
};

template<>
struct ConfigTypeIndex<bool> {
    static const size_t index = ConfigValueTypeIndex::Bool;
};

template<>
struct ConfigTypeIndex<CStrSpan> {
    static const size_t index = ConfigValueTypeIndex::String;
};

struct ConfigValueNode {
    ConfigValue value_;
    ConfigValueNode *next_ = nullptr;

    static ConfigValueNode *CreateScratch(ScratchAllocator &allocator);
    static ConfigValueNode *Create();
    static void Destroy(const ConfigValueNode *node);
    static void CloneNode(const ConfigValueNode &head, ConfigValueNode &cloned, ScratchAllocator *allocator);
};

struct ConfigKeyHasher {
    size_t operator()(CStrSpan str) const {
        return std::hash<std::string_view>()(std::string_view{str.Data(), str.Size()});
    }
};
struct ConfigKeyEqual {
    bool operator()(CStrSpan a, CStrSpan b) const {
        return strcmp(a.CStr(), b.CStr()) == 0;
    }
};
using ConfigMap = HashMap<CStrSpan, detail::ConfigValueNode, ConfigKeyHasher, ConfigKeyEqual>;

// Converts to string representation for logging and debugging use.
void ConfigValueToString(const ConfigValue &value, StrBuilder<256> &builder);

template<class T>
struct ConfigValueConvertType {
    using Type = T;
};

template<>
struct ConfigValueConvertType<const char*> {
    using Type = CStrSpan;
};

template<>
struct ConfigValueConvertType<char*> {
    using Type = CStrSpan;
};

template<>
struct ConfigValueConvertType<int> {
    using Type = int64_t;
};

template<>
struct ConfigValueConvertType<long> {
    using Type = int64_t;
};

template<>
struct ConfigValueConvertType<size_t> {
    using Type = int64_t;
};

template<class...Args>
ConfigValueNode *ConfigCreateList() {
    return nullptr;
}

template<class T, class...Args>
ConfigValueNode *ConfigCreateList(T head, Args...tail) {
    ConfigValueNode *node = ConfigValueNode::Create();
    node->value_ = typename ConfigValueConvertType<T>::Type{head};
    node->next_ = ConfigCreateList(std::forward<Args>(tail)...);
    return node;
}

} // namespace detail
} // namespace xynq
