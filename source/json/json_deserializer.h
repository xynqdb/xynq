#pragma once

#include "base/either.h"
#include "base/span.h"
#include "base/stream.h"
#include "base/scratch_allocator.h"
#include "base/str_builder.h"

#include <functional>

struct jsmn_parser;
typedef jsmn_parser jsmn_parser;

struct jsmntok;
typedef jsmntok jsmntok_t;

namespace xynq {

// Returns stream writer for new object with the given type.
using JsonGetObjectWriter = std::function<Either<StrSpan, StreamWriter> (StrSpan)>;

struct JsonDeserializerSuccess{};

class JsonDeserializer {
public:
    JsonDeserializer(ScratchAllocator &allocator,
                     JsonGetObjectWriter get_object_writer);

    Either<StrSpan, JsonDeserializerSuccess> Deserialize(StreamReader &reader);
private:
    struct JsonNode {
    };

    ScratchAllocator &allocator_;
    JsonGetObjectWriter get_object_writer_;
    StrBuilder<100> str_builder_;

    JsonNode *tail_ = nullptr; // Json nodes is single linked list - this ptr is the tail of the.
    bool is_deferred_object_ = false;

    StrSpan ProcessTokens(jsmn_parser &parser, DataSpan buffer, jsmntok_t *tokens, size_t num_tokens);
};

} // xynq