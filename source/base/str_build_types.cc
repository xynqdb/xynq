#include "str_build_types.h"

using namespace xynq;
using namespace xynq::detail;

namespace {

const char hex_digits[] = "0123456789abcdef";

// Returns number of digits in the number (in base 10).
inline unsigned CountDigits(uint64_t value) {
    XYAssert(value != 0); // this function exepects only positive values.

    unsigned count = 0;
    while (value != 0) {
        value /= 10;
        ++count;
    }
    return count;
}

inline unsigned CountHexDigits(uint64_t value) {
    XYAssert(value != 0);

    unsigned count = 0;
    while (value != 0) {
        value >>= 4;
        ++count;
    }
    return count;
}
} // anon namespace
////////////////////////////////////////////////////////////


size_t xynq::detail::StrBuildUIntToStr(uint64_t value, char *buf) {
    if (value == 0) {
        *buf++ = '0';
        return 1;
    }

    // Get number of digits.
    unsigned num_digits = CountDigits(value);
    size_t i = num_digits;

    // Filling buffer with digits from the highest byte to lowest.
    while (value > 0) {
        uint64_t next_val = value / 10;
        buf[--i] = '0' + (value - 10 * next_val);
        value = next_val;
    }

    return num_digits;
}

size_t xynq::detail::StrBuildIntToStr(int64_t value, char *buf) {
    if (value < 0) {
        *buf++ = '-';
        return 1 + StrBuildUIntToStr((uint64_t)-value, buf);
    } else {
        return StrBuildUIntToStr((uint64_t)value, buf);
    }
}
#include <stdio.h>

size_t xynq::detail::StrBuildUIntToHex(uint64_t value, char *buf, bool add0x) {
    unsigned extra_len = 0;
    if (add0x) {
        *buf++ = '0';
        *buf++ = 'x';
        extra_len = 2;
    }

    if (value == 0) {
        *buf++ = '0';
        return extra_len + 1;
    }

    unsigned num_digits = CountHexDigits(value);
    size_t i = num_digits;

    while (value != 0) {
        buf[--i] = hex_digits[value & 0xf];
        value >>= 4;
    }
    return extra_len + num_digits; // 2 for 0x
}