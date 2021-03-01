#pragma once

#include "config_detail.h"

#include "base/either.h"
#include "base/maybe.h"
#include "base/scratch_allocator.h"
#include "base/span.h"
#include "base/stream.h"
#include "base/str_builder.h"
#include "containers/vec.h"

#include <algorithm>
#include <cstdio>

namespace xynq {

// Loading error code.
enum class ConfigLoadError {
    FileNotFound,    // Config file is not found or permission denied on it.
    IOError,         // I/O error, ie. some IO operation failed. See logs for more info.
    InvalidArgs,     // Wrongly formatted command line args.
    SyntaxError      // Syntax error in Slang config.
};

// Error type for key does not exist.
enum class ConfigKeyError {
    DoesNotExist, // Key does not exist.
    InvalidType   // Value at the given key is of different type than requested.
};

// Result of key lookup.
template<class T>
using ConfigKeyResult = Either<ConfigKeyError, T>;


// List of configuration values.
// List can contain values of different types.
class ConfigList {
    friend class Config;
public:
    // Allows std-like iteration over config lists value. ie:
    // ConfigList cfg_list = ...;
    // for (auto it : cfg_list) {
    //     DoSomething(it->Get<int>("my-key")); // do something with int list item.
    // }
    class Iterator {
        friend class ConfigList;
    public:
        // Returns the underlying value. Can be used like:
        // list.Get<int>("my-key");
        template<class T>
        ConfigKeyResult<T> Get() const;

        Iterator &operator++() {
            XYAssert(node_ != nullptr);
            node_ = node_->next_;
            return *this;
        }

        Iterator operator++(int) {
            XYAssert(node_ != nullptr);
            auto temp = Iterator(node_);
            node_ = node_->next_;
            return temp;
        }

        const Iterator &operator*() const { return *this; }
        bool operator==(const Iterator &other) const { return node_ == other.node_; }
        bool operator!=(const Iterator &other) const { return node_ != other.node_; }
    private:
        const detail::ConfigValueNode *node_ = nullptr;

        Iterator(const detail::ConfigValueNode *node)
            : node_(node)
        {}
    };

    // Creates list with provided values.
    // Can be used for creating default lists.
    template<class...Args>
    static ConfigList Make(Args&&...args);

    ConfigList(ConfigList &&other)
        : head_(other.head_)
        , owned_(other.owned_) {
        other.owned_ = false;
    }
    ConfigList(const ConfigList &) = delete;
    ConfigList& operator=(const ConfigList &) = delete;

    ~ConfigList() {
        if (owned_) {
            XYAssert(head_ != nullptr);
            detail::ConfigValueNode::Destroy(head_);
        }
    }

    // Returns whole list as array of elements of type T.
    // Fails if the list doesn't consist of values of the same types.
    template<class T>
    Maybe<Vec<T>> AsArray() const;

    Iterator begin() const { return Iterator(head_); }
    Iterator end() const { return Iterator(nullptr); }

private:
    const detail::ConfigValueNode *head_ = nullptr;
    bool owned_ = false;

    ConfigList(const detail::ConfigValueNode *head, bool owned)
        : head_(head)
        , owned_(owned)
    {}
};



// Immutable key-value map to store configurration keys.
// Configuration files are made of s-expressions like (key value):
// ie.:
//      (mykey "string value") ; string value
//      (mykey 325)            ; int value
//      (mykey 325.0)          ; double value
// can be hierarchical:
//      (parentkey
//          (childkey 325))
// Hierarchical keys are flattened when loaded, and can
// be accessed with dot syntax like this:
//      auto value = config.Get<int>("parentkey.childkey");
//
// Can hold multiple values (lists of values) on a single key:
//      (fib-seq 0 1 1 2 3 5 8 13 21)
//
// Use static Load*() methods to create config from files, memory buffer
// or command line arguments. See LoadFromArgs() comments for command line args
// format.
// Defaults can be implemented with Either:
//      Config conf = Config::LoadFromFile("my.conf").Right();
//      auto value = conf.Get<CStrSpan>("mykey").FoldLeft([]() {
//          return "MyDefaultValue";
//      })
class Config {
    friend class ConfigParser;
    friend class ConfigList;
public:
    Config() = default;
    Config(Config &&) = default;
    Config &operator=(Config &&) = default;
    ~Config();

    // Result of config load.
    using LoadResult = Either<ConfigLoadError, Config>;

    // Loads from the config file specified by config_path.
    static LoadResult LoadFromFile(const char *config_path);

    // Loads from in-mem buffer.
    static LoadResult LoadFromBuffer(MutStrSpan buffer);

    /// Load from the input stream.
    static LoadResult LoadFromStream(StreamReader &reader);

    // Loads from unix-style command line arguments.
    // It actually converts all args into valid Slang expressions and parses
    // them as normal Slang text. Only takes arguments prefixed with '/' all others
    // are ignored. ie.: /key.subkey 123 is equivalent to (key (subkey 123)) in config file.
    static LoadResult LoadFromArgs(int argc, const char * const *argv);
    ////////////////////////////////////////////////////////////


    // Returns value for the key.
    // If the key doesn't exist - returns DoesNotExist.
    // If the key exist but is of different type - returns InvalidType.
    // Types supported: string/CStrSpan, bool, int, long, int64_t, double.
    template<class T>
    ConfigKeyResult<T> Get(CStrSpan key) const;

    // Returns list of values. If there's only one value set for the key
    // - will still return a list with that single element.
    // Nested lists (trees) are not allowed.
    ConfigKeyResult<ConfigList> GetList(CStrSpan key) const;
    ////////////////////////////////////////////////////////////


    // Takes all values from the right config and merges them into left.
    // If some keys exist in both Configs - values from the right will override.
    // Both left and right are moved after after merge and should not be used.
    static Config Merge(Config &&left, Config &&right);

    // Allows enumerating all the keys set in the config. Values all have string representation.
    // Callback must be callable with cb(CStrSpan key, CStrSpan value)
    // Output is sorted by keys.
    // It's quite inefficient and supposed to be used only for debug output.
    template<class Callback>
    void Enumerate(Callback callback) const;
private:
    detail::ConfigMap values_;
    mutable ScratchAllocator allocator_;

    Config(detail::ConfigMap &&values, ScratchAllocator &&allocator);
    static Config::LoadResult LoadFromBuffer(MutStrSpan buffer, const char *source_file);
    static void MergeValues(detail::ConfigMap &left, detail::ConfigMap &&right, ScratchAllocator *allocator);
};
////////////////////////////////////////////////////////////


template<class...Args>
ConfigList ConfigList::Make(Args&&...args) {
    return ConfigList{detail::ConfigCreateList(std::forward<Args>(args)...), true};
}

template<class T>
ConfigKeyResult<T> ConfigList::Iterator::Get() const {
    XYAssert(node_ != nullptr);
    if (node_->value_.index() != detail::ConfigTypeIndex<T>::index) {
        return ConfigKeyError::InvalidType;
    }

    return (T)std::get<detail::ConfigTypeIndex<T>::index>(node_->value_);
}

template<class T>
Maybe<Vec<T>> ConfigList::AsArray() const {
    auto it = begin();
    size_t num_items = 0;
    while (it != end()) {
        auto get_value = it.Get<T>();
        if (get_value.IsLeft()) {
            return {};
        }
        ++it;
        ++num_items;
    }

    Vec<T> result;
    result.resize(num_items);
    size_t i = 0;

    for (auto it = begin(); it != end(); ++it) {
        auto get_value = it.Get<T>();
        XYAssert(i < result.size());
        XYAssert(get_value.IsRight());

        result[i++] = get_value.Right();
    }

    return result;
}

template<class T>
ConfigKeyResult<T> Config::Get(CStrSpan key) const {
    auto it = values_.find(key.CStr());
    if (it == values_.end()) {
        return ConfigKeyError::DoesNotExist;
    }
    if (it->second.value_.index() != detail::ConfigTypeIndex<T>::index) {
        return ConfigKeyError::InvalidType;
    }

    return (T)std::get<detail::ConfigTypeIndex<T>::index>(it->second.value_);
}

template<class Callback>
void Config::Enumerate(Callback callback) const {
    allocator_.Scoped([&](ScratchAllocator &allocator) {
        // Sorting by keys to get pretty (always ordered) output.
        size_t num_keys = values_.size();
        CStrSpan *sorted_keys = (CStrSpan *)allocator.Alloc(sizeof(CStrSpan) * num_keys);
        CStrSpan *key = sorted_keys;
        for (auto it = values_.begin(); it != values_.end(); ++it, ++key) {
            *key = it->first;
        }

        std::sort(sorted_keys, sorted_keys + num_keys, [](const CStrSpan &left, const CStrSpan &right) {
            return strcmp(left.CStr(), right.CStr()) < 0;
        });

        // Now iterate over keys and call handler on formatted output.
        for (key = sorted_keys; key != sorted_keys + num_keys; ++key) {
            auto it = values_.find(*key);
            XYAssert(it != values_.end());

            const detail::ConfigValueNode &node = it->second;
            StrBuilder<256> value_builder;
            if (node.next_ == nullptr) { // Single value.
                detail::ConfigValueToString(node.value_, value_builder);
                callback(*key, value_builder.MakeCStr());
            } else { // Formatting list into pretty string like [a,b,c...n]
                value_builder << '[';
                for (const detail::ConfigValueNode *i = &node; i != nullptr; i = i->next_) {
                    detail::ConfigValueToString(i->value_, value_builder);
                    if (i->next_ != nullptr) {
                        value_builder << ", ";
                    }
                }
                value_builder << ']';
                callback(*key, value_builder.MakeCStr());
            }

        }
    });
}

}