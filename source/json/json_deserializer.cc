#include "json_deserializer.h"

#include "jsmn/jsmn.h"

using namespace xynq;

namespace {

static constexpr unsigned int k_jsmn_max_tokens = 64;

// Need for delayed creation.
//static constexpr int JSMN_XYNQ_TYPE =

inline StrSpan GetErrorHintText(const jsmn_parser &parser, DataSpan buffer) {
    // Getting text in buffer: (pos - 20, pos + 20)
    long long pos_begin = std::max(0ll, (long long)parser.pos - 20);
    long long pos_end = std::min((long long)buffer.Size(), (long long)parser.pos + 20);

    return StrSpan{(const char *)buffer.Data() + pos_begin,
                   (const char *)buffer.Data() + pos_end};
}

inline void ResetTokens(jsmn_parser &parser) {
    parser.toknext = 0;
}

inline StrSpan TokenSpan(jsmntok_t token, DataSpan buffer) {
    return StrSpan{(const char *)buffer.Data() + token.start, token.size};
}

} // anon namespace

JsonDeserializer::JsonDeserializer(ScratchAllocator &allocator,
                                   JsonGetObjectWriter get_object_writer)
    : allocator_(allocator)
    , get_object_writer_(get_object_writer) {
    XYAssert(get_object_writer_);
}

Either<StrSpan, JsonDeserializerSuccess> JsonDeserializer::Deserialize(StreamReader &reader) {
    jsmn_parser parser;
    jsmntok_t tokens[k_jsmn_max_tokens];
    jsmn_init(&parser);

    // Prefill first data chunk from the stream if not prefilled already.
    if (reader.IsGood() && !reader.Available().IsEmpty()) {
        auto res_refill = reader.RefillAvailable();
        if (res_refill.IsLeft() && reader.Stream().LastError() != StreamError::Closed) {
            return StrSpan{"Stream error"};
        }
    }

    while (reader.IsGood() && !reader.Available().IsEmpty()) {
        int err = jsmn_parse(&parser,
                             (const char *)reader.Available().Data(),
                             reader.Available().Size(),
                             tokens, k_jsmn_max_tokens);

        if (err == JSMN_ERROR_INVAL) { // Malformed json.
            reader.Advance(parser.pos);
            str_builder_.Append("Invalid json at: ");
            str_builder_.Append(GetErrorHintText(parser, reader.Available()));
            return str_builder_.Buffer();
        }

        StrSpan tok_error = ProcessTokens(parser, reader.Available(), &tokens[0], parser.toknext);
        reader.Advance(parser.pos);
        if (!tok_error.IsEmpty()) {
            return tok_error;
        }

        if (err >= 0) { // No errors, finished processing.
            break;
        } else if (err == JSMN_ERROR_NOMEM) { // Starving for tokens.
            ResetTokens(parser);
        } else if (err == JSMN_ERROR_PART) { // Not enough data in the stream.
            // TODO: save current token or string etc.
            if (reader.RefillAvailable().IsLeft()) { // JSMN needs more data but there's no more available.
                return StrSpan{"Stream error"};
            }
        } else { // Some other jsmn error, should not really get here.
            XYAssert(0);
            return StrSpan{"Internal parser error"};
        }
    }

    return JsonDeserializerSuccess{};
}

StrSpan JsonDeserializer::ProcessTokens(jsmn_parser &parser, DataSpan buffer, jsmntok_t *tokens, size_t num_tokens) {
    if (num_tokens == 0) {
        return "";
    }

    if (is_deferred_object_ == 0) {
        tokens->type = JSMN_XYNQ_TYPE;
    }

    for (jsmntok_t *tok = tokens; tok != tokens + num_tokens; ++tok) {
        switch (tok->type) {
            case JSMN_XYNQ_TYPE:
                break;
            case JSMN_OBJECT:
                break;
            case
            default:

        }
    }

    return "";
}
