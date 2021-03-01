#pragma once

#include "base/assert.h"

#include <utility>

namespace xynq {

// Wrapper for value that might exist or not.
// Equivalent of std::optional but can be folded and mapped with lambdas
// and piped with other Maybes and Eithers.
template<class T>
class Maybe
{
private:
    union {
      T  value_;
    };

    const bool has_value_ = false;

public:
    Maybe();
    Maybe(const Maybe<T> &rhs);
    Maybe(Maybe<T> &&rhs);
    Maybe(const T &rhs);
    Maybe(T &&rhs);
    ~Maybe();

    // Maybe<T> &operator=(const Maybe<T> &rhs) = default;
    // Maybe<T> &operator=(Maybe<T> &&rhs) = default;

    // true if value exists.
    bool HasValue() const;
    operator bool();

    // Returns underlying value.
    // Asserts if vallue does not exist.
    const T &Value() const;
    T &Value();

    // Returns underlying value. Asserts if value does not exist.
    T &Get();
    const T &Get() const;

    // Returns value if exists or default value.
    const T &GetOrDefault(const T &default_value) const;
    T &&GetOrDefault(T &&default_value);
    const T &&GetOrDefault(const T &&default_value) const;

    // If value exists will return value, otherwise will return a function result.
    // Function must be: T func();
    template<class FoldFunc>
    T Fold(FoldFunc fold_func);

    // Returns Maybe<Result of a function>.
    // If this Maybe has value - performs function and returns new Maybe.
    // If value does not exist will return Maybe<ReturnType> with no value.
    template<class MapFunc>
    auto Map(MapFunc map_func) -> Maybe<decltype(map_func(std::move(value_)))>;
};
////////////////////////////////////////////////////////////


// Implementation.
template<class T>
Maybe<T>::Maybe()
    : has_value_(false) {
}

template<class T>
Maybe<T>::Maybe(const Maybe<T> &rhs)
    : has_value_(rhs.has_value_) {

    if (has_value_) {
        new(&value_) T(rhs.value_); // Forcing copy-ctor
    }
}

template<class T>
Maybe<T>::Maybe(Maybe<T> &&rhs)
    : has_value_(rhs.has_value_) {

    if (rhs.has_value_) {
        new(&value_) T(std::move(rhs.value_)); // Forcing move-ctor
    }
}

template<class T>
Maybe<T>::Maybe(const T &value)
    : value_(value)
    , has_value_(true) {
}

template<class T>
Maybe<T>::Maybe(T &&value)
    : value_(std::forward<T>(value))
    , has_value_(true) {
}

template<class T>
Maybe<T>::~Maybe() {
    if (has_value_) {
        value_.~T();
    }
}

// template<class T>
// Maybe<T> &operator=(const Maybe<T> &rhs) {
//     return *this;
// }

// template<class T>
// Maybe<T> &operator=(Maybe<T> &&rhs) {
//     has_value = rhs.
//     return *this;
// }

template<class T>
bool Maybe<T>::HasValue() const {
    return has_value_;
}

template<class T>
Maybe<T>::operator bool() {
    return has_value_;
}

template<class T>
const T &Maybe<T>::Value() const {
    XYAssert(HasValue());
    return value_;
}

template<class T>
T &Maybe<T>::Value() {
    XYAssert(HasValue());
    return value_;
}

template<class T>
T &Maybe<T>::Get() {
    XYAssert(has_value_);
    return value_;
}

template<class T>
const T &Maybe<T>::Get() const {
    XYAssert(has_value_);
    return value_;
}

template<class T>
T &&Maybe<T>::GetOrDefault(T &&default_value) {
    if (has_value_) {
        return std::move(value_);
    } else {
        return std::move(default_value);
    }
}

template<class T>
const T &&Maybe<T>::GetOrDefault(const T &&default_value) const {
    if (has_value_) {
        return std::move(value_);
    } else {
        return std::move(default_value);
    }
}

template<class T>
const T &Maybe<T>::GetOrDefault(const T &default_value) const {
    if (has_value_) {
        return value_;
    } else {
        return default_value;
    };
}

template<class T>
template<class FoldFunc>
T Maybe<T>::Fold(FoldFunc fold_func) {
    if (has_value_) {
        return std::move(value_);
    } else {
        return fold_func();
    }
}

template<class T>
template<class MapFunc>
auto Maybe<T>::Map(MapFunc map_func) -> Maybe<decltype(map_func(std::move(value_)))> {
    if (has_value_) {
        return map_func(value_);
    } else {
        using ResultType = decltype(map_func(std::move(value_)));
        return Maybe<ResultType>();
    }
}

} // xynq