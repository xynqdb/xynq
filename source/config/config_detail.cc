#include "config_detail.h"

#include "base/assert.h"
#include "containers/str.h"

using namespace xynq;
using namespace xynq::detail;

void xynq::detail::ConfigValueToString(const ConfigValue &value, StrBuilder<256> &builder) {
    switch (value.index()) {
        case ConfigValueTypeIndex::String:
            builder << std::get<ConfigValueTypeIndex::String>(value);
            break;
        case ConfigValueTypeIndex::Int:
            builder << std::get<ConfigValueTypeIndex::Int>(value);
            break;
        case ConfigValueTypeIndex::Double:
            builder << std::get<ConfigValueTypeIndex::Double>(value);
            break;
        case ConfigValueTypeIndex::Bool: {
            builder << (std::get<ConfigValueTypeIndex::Bool>(value) ? "Yes" : "No");
            break;
        }
        default:
            XYAssert(0);
            builder << "n/a";
    }
}

ConfigValueNode* ConfigValueNode::CreateScratch(ScratchAllocator &allocator) {
    return CreateObject<ConfigValueNode>(allocator);
}

ConfigValueNode *ConfigValueNode::Create() {
    return CreateObject<ConfigValueNode>(SystemAllocator::Shared());
}

void ConfigValueNode::Destroy(const ConfigValueNode *node) {
    const ConfigValueNode *cur = node;
    while (cur != nullptr) {
        ConfigValueNode *next = cur->next_;
        DestroyObject(SystemAllocator::Shared(), cur);
        cur = next;
    }
}

void ConfigValueNode::CloneNode(const ConfigValueNode &head, ConfigValueNode &cloned, ScratchAllocator *allocator) {
    XYAssert(allocator != nullptr);

    const ConfigValueNode *cur = &head;
    ConfigValueNode *cur_cloned = &cloned;
    while (cur != nullptr) {
        if (cur->value_.index() == ConfigValueTypeIndex::String) {
            CStrSpan str = std::get<ConfigValueTypeIndex::String>(cur->value_);
            cur_cloned->value_ = MakeScratchCStrCopy(str, *allocator);
        } else {
            cur_cloned->value_ = cur->value_;
        }

        cur = cur->next_;
        if (cur != nullptr) {
            cur_cloned->next_ = ConfigValueNode::CreateScratch(*allocator);
            cur_cloned = cur_cloned->next_;
        }
    }
}