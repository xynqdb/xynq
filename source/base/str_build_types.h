#pragma once

#include "base/span.h"

#include <cmath>
#include <cstdio>
#include <stddef.h>
#include <type_traits>
#include <algorithm>

namespace xynq {

namespace detail {
    // Numbers to string converstions.
    // All of these functions expect buffer to have enough capacity for the string
    // representation of a value.
    size_t StrBuildIntToStr(int64_t value, char *buf);
    size_t StrBuildUIntToStr(uint64_t value, char *buf);
    size_t StrBuildUIntToHex(uint64_t value, char *buf, bool add0x = true);
} // namespace detail


// Wrapper to print hex numbers, only works on unsigned integers.
// Can be used with StrBuilder like: builder << StrHex{16}; // Will produce "0x10"
template<class T>
struct StrHex {
    inline StrHex(T value, bool add0x = true)
        : value_(value)
        , add0x_(add0x)
    {}

    T value_;
    bool add0x_;
};

// Print doubles and floats with higher precision than simple writer.
// Much slower than default double/float writer.
struct StrHiPrecision {
    inline StrHiPrecision(double value)
        : value_(value)
    {}

    inline StrHiPrecision(float value)
        : value_(value)
    {}

    double value_;
};

// Wrapper to print floats and doubles with fixed precision.
// Can be used with StrBuilder like: builder << StrPrecision{2.1234, 2}; // Will produce "2.12"
template<class T>
struct StrPrecision {
    inline StrPrecision(T value, unsigned precision)
        : value_(value)
        , precision_(precision)
    {}

    T value_;
    unsigned precision_;
};


// List of types that supports being built into string with StrBuilder.
template<class T, class U = T>
struct StrBuild;

// Unsigned Integers.
template<class T>
struct StrBuild<T, std::enable_if_t<
       std::is_same<T, unsigned char>::value
    || std::is_same<T, unsigned short>::value
    || std::is_same<T, unsigned int>::value
    || std::is_same<T, unsigned long>::value
    || std::is_same<T, unsigned long long>::value
    || std::is_same<T, size_t>::value
    || std::is_same<T, uint8_t>::value
    || std::is_same<T, uint16_t>::value
    || std::is_same<T, uint32_t>::value
    || std::is_same<T, uint64_t>::value, T>> {

    template<class Writer>
    size_t operator()(T value, Writer &writer) const {
        writer.Write(24, [&](MutSpan<char> buf) {
            size_t sz = detail::StrBuildUIntToStr(value, buf.Data());
            XYAssert(sz <= 24);
            return sz;
        });
        return 0;
    }
};

// Signed Integers.
// char is handled separately.
template<class T>
struct StrBuild<T, std::enable_if_t<
       std::is_same<T, short>::value
    || std::is_same<T, int>::value
    || std::is_same<T, long>::value
    || std::is_same<T, long long>::value
    || std::is_same<T, int8_t>::value
    || std::is_same<T, int16_t>::value
    || std::is_same<T, int32_t>::value
    || std::is_same<T, int64_t>::value, T>> {

    template<class Writer>
    size_t operator()(T value, Writer &writer) const {
        writer.Write(24, [&](MutSpan<char> buf) {
            size_t sz = detail::StrBuildIntToStr(value, buf.Data());
            XYAssert(sz <= 24);
            return sz;
        });
        return 0;
    }
};

// bool
template<class T>
struct StrBuild<T, bool> {
    template<class Writer>
    void operator()(bool value, Writer &writer) const {
        writer.Write(1, [&](MutSpan<char> buf) {
            buf[0] = value ? 'Y' : 'N';
            return 1;
        });
    }
};

// char
template<class T>
struct StrBuild<T, char> {
    template<class Writer>
    void operator()(char value, Writer &writer) const {
        writer.Write(1, [&](MutSpan<char> buf) {
            buf[0] = value;
            return 1;
        });
    }
};


// CStrSpan
template<class T>
struct StrBuild<T, CStrSpan> {
    template<class Writer>
    void operator()(CStrSpan str, Writer &writer) const {
        writer.BestEffortWrite(str);
    }
};

// StrSpan
template<class T>
struct StrBuild<T, StrSpan> {
    template<class Writer>
    void operator()(StrSpan str, Writer &writer) const {
        writer.BestEffortWrite(str);
    }
};

template<class T>
struct StrBuild<T, MutStrSpan> {
    template<class Writer>
    void operator()(MutStrSpan str, Writer &writer) const {
        writer.BestEffortWrite(str);
    }
};

// char[]
template<class T, size_t Size>
struct StrBuild<T, char[Size]> {
    template<class Writer>
    void operator()(const char str[Size], Writer &writer) const {
        writer.BestEffortWrite(StrSpan{&str[0], Size - 1});
    }
};

// char*
template<>
struct StrBuild<char*> {
    template<class Writer>
    void operator()(const char *str, Writer &writer) const {
        StrBuild<CStrSpan>()({str}, writer);
    }
};

template<>
struct StrBuild<const char*> {
    template<class Writer>
    void operator()(const char *str, Writer &writer) const {
        StrBuild<CStrSpan>()({str}, writer);
    }
};

// Hex.
template<class IntType>
struct StrBuild<StrHex<IntType>> {
    template<class Writer>
    void operator()(StrHex<IntType> hex, Writer &writer) const {
        writer.Write(32, [&](MutSpan<char> buf) {
            return detail::StrBuildUIntToHex(hex.value_, buf.Data(), hex.add0x_);
        });
    }
};

// Pointers
template<class T>
struct StrBuild<T *> {
    template<class Writer>
    void operator()(T *ptr, Writer &writer) const {
        writer.Write(32, [&](MutSpan<char> buf) {
            return detail::StrBuildUIntToHex(reinterpret_cast<uint64_t>(ptr), buf.Data());
        });
    }
};
template<class T>
struct StrBuild<const T *> {
    template<class Writer>
    void operator()(const T *ptr, Writer &writer) const {
        writer.Write(32, [&](MutSpan<char> buf) {
            return detail::StrBuildUIntToHex(reinterpret_cast<uint64_t>(ptr), buf.Data());
        });
    }
};

// double
// This is building into human-readable string representing given double.
// Not precsie - should not be used for serialization.
namespace detail {

// num_fraction_digits - number of digits after the point.
template<class Writer>
void StrBuildWriteDouble(double value, size_t num_fraction_digits, Writer &writer) {
    if (std::isnan(value)) {
        writer.Write(3, [](MutStrSpan buf) {
            memcpy(buf.Data(), "nan", 3);
            return 3;
        });
    } else if (std::isinf(value)) {
        if (value > 0) {
            writer.Write(3, [](MutStrSpan buf) {
                memcpy(buf.Data(), "inf", 3);
                return 3;
            });
        } else {
            writer.Write(4, [](MutStrSpan buf) {
                memcpy(buf.Data(), "-inf", 4);
                return 4;
            });
        }
    } else {
        if (value < 0) {
            value = -value;
            StrBuild<char>()('-', writer);
        }

        if ((value < 1e6 && (value * 1e6 - std::floor(value * 1e6)) == 0.0)) { // can be represented with 6 digits?
            double hi;
            double lo = std::modf(value, &hi);
            StrBuild<uint64_t>()((uint64_t)hi, writer);
            StrBuild<char>()('.', writer);

            unsigned j = 0;
            for (unsigned i = 0; i < num_fraction_digits; ++i) {
                lo *= 10.0;
                double f = std::floor(lo);
                lo -= f;
                unsigned d = std::clamp((unsigned)f, 0u, 9u);
                StrBuild<char>()('0' + d, writer);
                if (d != 0) {
                    j = i;
                }
            }
            writer.Rollback(num_fraction_digits - 1 - j); // remove trailing zeros
        } else { // too long -> use scientific notation.
            double e = std::floor(std::log10(value));
            value /= std::pow(10, e); // getting heading digit

            double f = std::floor(value);
            unsigned d = std::clamp((unsigned)f, 0u, 9u);
            StrBuild<char>()('0' + d, writer);
            StrBuild<char>()('.', writer);
            value -= f;

            unsigned j = 0;
            for (unsigned i = 0; i < num_fraction_digits; ++i) {
                value *= 10.0;
                f = std::floor(value);
                value -= f;
                unsigned d = std::clamp((unsigned)f, 0u, 9u);
                StrBuild<char>()('0' + d, writer);

                if (d != 0) {
                    j = i;
                }
            }
            writer.Rollback(num_fraction_digits - 1 - j); // remove trailing zeros
            StrBuild<char>()('e', writer);
            StrBuild<int64_t>()((int64_t)e, writer);
        }
    }
}

} // detail

template<class T>
struct StrBuild<T, double> {
    template<class Writer>
    void operator()(double value, Writer &writer) const {
        detail::StrBuildWriteDouble(value, 6, writer);
    }
};

// Float
template<class T>
struct StrBuild<T, float> {
    template<class Writer>
    void operator()(float value, Writer &writer) const {
        StrBuild<double>()(value, writer);
    }
};

// Precision.
template<class FloatType>
struct StrBuild<StrPrecision<FloatType>> {
    template<class Writer>
    void operator()(StrPrecision<FloatType> float_val, Writer &writer) const {
        detail::StrBuildWriteDouble(float_val.value_, float_val.precision_, writer);
    }
};

// Hi Precision.
template<>
struct StrBuild<StrHiPrecision> {
    template<class Writer>
    void operator()(StrHiPrecision pr_val, Writer &writer) const {
        int sz = std::snprintf(0, 0, "%.24g", pr_val.value_);
        if (sz > 0) {
            writer.Write(size_t(sz) + 1, [&](MutSpan<char> buf) {
                sz = std::snprintf(buf.Data(), buf.Size(), "%.24g", pr_val.value_);
                return (sz < 0) ? 0 : sz;
            });
        }
    }
};

} // xynq
