#include "config.h"
#include "base/assert.h"
#include "base/file_stream.h"
#include "base/fileutils.h"
#include "base/output.h"
#include "containers/str.h"
#include "slang/lexer.h"

#include <cerrno>
#include <cstring>
#include <utility>

using namespace xynq;
using namespace xynq::slang;
using namespace xynq::detail;

namespace {

bool CheckArg(const char *arg) {
    XYAssert(arg != nullptr);
    if (*arg == '\0') { // Cannot be empty
        return false;
    }

    while (*arg != '\0') {
        if (*arg == ' '
         || *arg == '\t'
         || *arg == '\n'
         || *arg == '\r'
         || *arg == '\f'
         || *arg == '\v'
         || *arg == '('
         || *arg == ')') {
            return false;
        }
        ++arg;
    }

    return true;
}

bool MakeSlangExprFromArgs(const char *key, const char *value, Str &buf) {
    XYAssert(key != nullptr);
    XYAssert(value != nullptr);

    if (!CheckArg(key)) {
        return false;
    }

    if (!CheckArg(value)) {
        return false;
    }

    buf += '(';
    buf += key;
    buf += ' ';
    buf += value;
    buf += ')';
    buf += '\n';
    return true;
}

} // anon namespace


// ConfigParser
namespace xynq {

// Handler of slang parser that extracts key->value pairs from s-expressions
// and puts them into config map.
class ConfigParser {
public:
    using Result = Either<StrSpan, LexerSuccess>;

    ConfigParser(ConfigMap &config_map,
                 StrSpan source_path,
                 ScratchAllocator *allocator)
       : config_map_(config_map)
       , allocator_(allocator)
       , source_path_(source_path)
       , cur_key_(allocator)
       , cur_key_stack_(allocator)
    {
        cur_key_.reserve(128);
        cur_key_stack_.reserve(32);
    }

    ~ConfigParser() = default;

    Result LexerBeginOp(StrSpan key) {
        if (cur_value_last_ != nullptr ) { // already have some values.
            return StrSpan{"Invalid list. Lists cannot have nested keys."};
        }

        cur_key_stack_.push_back(cur_key_.size());
        cur_key_ += '.';

        cur_key_.append(key.Data(), key.Size());
        is_nested_list_ = false;
        return LexerSuccess{};
    }

    Result LexerEndOp() {
        if (cur_key_.empty()) {
            return StrSpan{"No key"};
        }

        XYAssert(cur_key_stack_.size() > 0);

        StrSpan cur_op = StrSpan{cur_key_.data() + cur_key_stack_.back() + 1, cur_key_.data() + cur_key_.size()};

        if (cur_op == "@locate") { // Locate files and pass it to whatever upper level key is.
            for (auto it = &cur_value_head_; it != nullptr; it = it->next_) {
                auto &value = it->value_;
                if (value.index() != ConfigValueTypeIndex::String) {
                    return StrSpan{"Expected filename string for @locate."};
                }

                CStrSpan value_str = std::get<ConfigValueTypeIndex::String>(value);

                CStrSpan filepath = fileutils::ReplaceFilename(source_path_, value_str, allocator_);
                value = filepath;
            }
        } else {
            if (cur_op == "@include") { // Load another config file.
                if (cur_key_stack_.size() > 1) {
                    return StrSpan{"@include is expected at top level"};
                }

                for (auto it = &cur_value_head_; it != nullptr; it = it->next_) {
                    auto &value = it->value_;
                    if (value.index() != ConfigValueTypeIndex::String) {
                        return StrSpan{"Invalid include filename. Should be a string."};
                    }

                    CStrSpan value_str = std::get<ConfigValueTypeIndex::String>(value);
                    auto loaded = Config::LoadFromFile(value_str.Data());
                    if (loaded.IsLeft()) {
                        return StrSpan{"Failed to load config"};
                    }

                    Config::MergeValues(config_map_, std::move(loaded.Right().values_), allocator_);
                }
            } else if (*cur_op.begin() == '@') {
                return StrSpan{"Unknown directive."};
            } else if (cur_value_head_.value_.index() != 0) {
                CStrSpan new_key = MakeScratchCStrCopy(CStrSpan{cur_key_.c_str() + 1, cur_key_.c_str() + cur_key_.size()}, *allocator_); // + to skip prefix dot in current key
                config_map_.emplace(std::make_pair(new_key, std::move(cur_value_head_)));
            }

            // if value is not discarded it will be assigned to prev level in hierarchy
            // which is what we need for things like @locate
            cur_value_head_ = ConfigValueNode{};
            cur_value_last_ = nullptr;
        }

        // Means there is mismatch in (). This should be caught by the lexer.
        XYAssert(!cur_key_stack_.empty());

        cur_key_.resize(cur_key_stack_.back());
        cur_key_stack_.pop_back();
        is_nested_list_ = !cur_key_stack_.empty();
        return LexerSuccess{};
    }

    ConfigValue &AddValue() {
        // List not initialized yet -> initialize head.
        if (cur_value_last_ == nullptr) {
            XYAssert(cur_value_head_.value_.index() == 0); // should not have any values at this point.
            cur_value_last_ = &cur_value_head_;
            return cur_value_head_.value_;
        } else { // allocate new value and connect it to the last one;
            XYAssert(cur_value_last_->next_ == nullptr);

            ConfigValueNode *new_node = ConfigValueNode::CreateScratch(*allocator_);
            cur_value_last_->next_ = new_node;
            cur_value_last_ = new_node;
            return new_node->value_;
        }
    }

    Result LexerStrValue(StrSpan value) {
        if (is_nested_list_) {
            return StrSpan{"Nested lists are not allowed in config"};
        }
        ConfigValue &conf_value = AddValue();
        conf_value = MakeScratchCStrCopy(value, *allocator_);
        return LexerSuccess{};
    }

    Result LexerUnhandledValue(StrSpan value) {
        if (is_nested_list_) {
            return StrSpan{"Nested lists are not allowed in config"};
        }
        ConfigValue &conf_value = AddValue();
        if (value == "yes" || value == "Yes") {
            conf_value = true;
        } else if (value == "no" || value == "No") {
            conf_value = false;
        } else {
            return LexerStrValue(value);
        }

        return LexerSuccess{};
    }

    Result LexerIntValue(int64_t value) {
        if (is_nested_list_) {
            return StrSpan{"Nested lists are not allowed in config"};
        }
        ConfigValue &conf_value = AddValue();
        conf_value = value;
        return LexerSuccess{};
    }

    Result LexerDoubleValue(double value) {
        if (is_nested_list_) {
            return StrSpan{"Nested lists are not allowed in config"};
        }
        ConfigValue &conf_value = AddValue();
        conf_value = value;
        return LexerSuccess{};
    }

    Result LexerCustomData(uint32_t, StreamReader &) {
        return StrSpan{"No custom data is allowed in config"};
    }

private:
    ConfigMap &config_map_;
    ScratchAllocator *allocator_ = nullptr;

    StrSpan source_path_;

    ScratchStr cur_key_;
    ScratchVec<size_t> cur_key_stack_;
    ConfigValueNode cur_value_head_;
    ConfigValueNode *cur_value_last_ = nullptr; // last element of the list
    bool is_nested_list_ = false;
};
} // namespace xynq


////////////////////////////////////////////////////////////

// Config implementation.
Config::Config(ConfigMap &&values, ScratchAllocator &&allocator)
    : values_(std::forward<ConfigMap>(values))
    , allocator_(std::move(allocator))
{}

Config::~Config() {
    // Clean all before scratch allocator is destroyed.
    values_.clear();
}

Config::LoadResult Config::LoadFromFile(const char *config_path) {
    XYAssert(config_path != nullptr);

    InFileStream file_stream;
    if (!file_stream.Open(config_path)) {
        XYOutputError("Config: Failed to open config file %s. Error=%d (%s).",
                      config_path,
                      errno,
                      ::strerror(errno));
        return ConfigLoadError::FileNotFound;
    }

    char buffer[512];
    StreamReader file_reader{buffer, file_stream};

    return LoadFromStream(file_reader);
}

Config::LoadResult Config::LoadFromArgs(int argc, const char * const *argv) {
    Str temp_buf;

    int i = 0;
    while (i < argc) {
        const char *key = argv[i++];
        XYAssert(key != nullptr);

        // We are only interested in args starting with '/'
        if (*key != '/') {
            continue;
        }

        ++key;
        if (*key == '\0') {
            XYOutputError("Config: Invalid key in argv[%d]", i);
            return ConfigLoadError::InvalidArgs;
        }

        if (i >= argc) {
            XYOutputError("Config: No value for key '%s'", argv[i]);
            return ConfigLoadError::InvalidArgs;
        }

        const char *value = argv[i++];
        if (!MakeSlangExprFromArgs(key, value, temp_buf)) {
            XYOutputError("Config: Cannot parse arguments: %s %s", key, value);
            return ConfigLoadError::InvalidArgs;
        }
    }

    return LoadFromBuffer(MutStrSpan{temp_buf.data(), temp_buf.size()});
}

Config::LoadResult Config::LoadFromBuffer(MutStrSpan buffer) {
    DummyInStream in_stream;
    StreamReader reader{MutDataSpan{buffer.Data(), buffer.Size()}, in_stream, buffer.Size()};
    return LoadFromStream(reader);
}

Config::LoadResult Config::LoadFromStream(StreamReader &reader) {
    ConfigMap config_map;
    ScratchAllocator allocator;

    ConfigParser parser(config_map, reader.Stream().Name(), &allocator);
    slang::Lexer<ConfigParser&> lexer(parser);
    auto result = lexer.Run(reader, allocator);
    if (result.IsLeft()) {
        const LexerFailure &failure = result.Left();
        XYOutputError("Config: Failed to parse config: %.*s\n",
                      failure.err_msg_.Size(),
                      failure.err_msg_.Data());
        return ConfigLoadError::SyntaxError;
    }

    return Config{std::move(config_map), std::move(allocator)};
}

ConfigKeyResult<ConfigList> Config::GetList(CStrSpan key) const {
    auto it = values_.find(key.CStr());
    if (it == values_.end()) {
        return ConfigKeyError::DoesNotExist;
    }

    return ConfigList{&it->second, false};
}

void Config::MergeValues(ConfigMap &left, ConfigMap &&right, ScratchAllocator *allocator) {
    XYAssert(allocator != nullptr);

    for (const auto &it : right) {
        CStrSpan key = MakeScratchCStrCopy(it.first, *allocator);
        auto new_it = left.insert(std::make_pair(key, ConfigValueNode{}));
        ConfigValueNode::CloneNode(it.second, new_it.first->second, allocator);
    }
}

Config Config::Merge(Config &&left, Config &&right) {
    Config::MergeValues(left.values_, std::move(right.values_), &left.allocator_);
    return std::move(left);
}
