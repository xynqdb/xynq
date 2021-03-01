#include "lexer.h"
#include "base/assert.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace {

// Charaters allowed for operation names.
inline bool IsOpChar(char ch) {
    return (ch > 0x20)
        && !(ch >= '0' && ch <= '9')
        && (ch != '(' && ch != ')' && ch != '{' && ch != '}' && ch != '[' && ch != ']')
        && (ch != '"' && ch != '\'')
        && (ch != ':' && ch != ';');
}

}

namespace xynq {
namespace slang {
namespace detail {

bool LexerCheckOpName(const char *begin, const char *end) {
    XYAssert(begin != nullptr);
    XYAssert(end != nullptr);

    const char *cur = begin;
    if (cur != end && !IsOpChar(*cur++)) { // Check the first character -> must not be digit.
        return false;
    }

    while (cur != end) {
        char ch = *cur++;
        if (!IsOpChar(ch) && !(ch >= '0' && ch <= '9')) {
            return false;
        }
    }

    return true;
}

Maybe<int64_t> LexerParseInt64(const char *begin, const char *end) {
    XYAssert(begin != nullptr);
    XYAssert(end > begin);

    char temp_buf[64];
    size_t len = (size_t)std::min(end - begin, 63l);
    memcpy(temp_buf, begin, len);
    temp_buf[len] = 0;

    char *found_end = nullptr;
    errno = 0;
    int64_t value = ::strtoll(temp_buf, &found_end, 10);
    if (found_end != nullptr && ((found_end - &temp_buf[0]) == end - begin) && errno == 0) {
        return value;
    } else {
        return {};
    }
}

Maybe<double> LexerParseDouble(const char *begin, const char *end) {
    XYAssert(begin != nullptr);
    XYAssert(end > begin);

    char temp_buf[128];
    size_t len = (size_t)std::min(end - begin, 127l);
    memcpy(temp_buf, begin, len);
    temp_buf[len] = 0;

    char *found_end = nullptr;
    errno = 0;
    double value = ::strtod(temp_buf, &found_end);
    if (found_end != nullptr && ((found_end - &temp_buf[0]) == end - begin) && errno == 0) {
        return value;
    } else {
        return {};
    }
}

StrSpan LexerParseString(char *begin, char *end, bool was_escaped) {
    XYAssert(begin != nullptr);
    XYAssert(end != nullptr);

    // If not escaped - can just capture buffer.
    // If the string is escaped need to skip all the escape slashes.
    if (!was_escaped) {
        return StrSpan{begin, end};
    }

    bool escaped = false;
    char *out_str = begin;
    const char *cur = begin;
    while (cur != end) {
        char ch = *cur++;
        if (ch == '\\' && !escaped) {
            escaped = true;
            continue;
        }

        escaped = false;
        *out_str++ = ch;
    }

    return StrSpan{begin, out_str};
}

} // detail
} // slang
} // xynq
