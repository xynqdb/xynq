#pragma once

#include "base/span.h"
#include "base/system_allocator.h"

#include <string_view>
#include <unordered_map>

namespace xynq {

template<typename K,
         typename V,
         typename H = std::hash<K>,
         typename E = std::equal_to<K>
>
using HashMap = std::unordered_map<K, V, H, E>;

} // xynq

namespace std {
template<> struct hash<xynq::StrSpan> {
    size_t operator()(xynq::StrSpan s) const {
        return std::hash<std::string_view>()({s.Data(), s.Size()});
    }
};
}
