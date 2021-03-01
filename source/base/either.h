#pragma once

#include "base/assert.h"

#include <type_traits>
#include <utility>

namespace xynq {

// Helper that can either be of type LeftType or of type RightType.
template<class LeftType, class RightType>
class Either
{
private:
    union {
      LeftType  left_;
      RightType right_;
    };

    bool is_left_ = false;

public:

    static_assert(!std::is_same<LeftType, RightType>::value,
                  "Either types must be different");

    Either(const Either<LeftType, RightType> &rhs);
    Either(Either<LeftType, RightType> &&rhs);
    Either(const LeftType &value);
    Either(const RightType &value);
    Either(LeftType &&value);
    Either(RightType &&value);
    ~Either();

    Either<LeftType, RightType> &
    operator=(const Either<LeftType, RightType> &rhs);

    Either<LeftType, RightType> &
    operator=(Either<LeftType, RightType> &&rhs);

    // true if contains left type, false otherwise.
    bool IsLeft() const;
    // true if contains right type, false otherwise.
    bool IsRight() const;

    // Returns left node of the Either. Asserts if this either is Right.
    LeftType &Left();
    const LeftType &Left() const;

    // Returns left node of the Either or default_value if the Either is Right.
    const LeftType &LeftOrDefault(const LeftType &default_value) const;
    const LeftType &&LeftOrDefault(const LeftType &&default_value) const;
    LeftType &&LeftOrDefault(LeftType &&default_value);

    // Returns right node of the Either or default_value if the Either is Left.
    const RightType &RightOrDefault(const RightType &default_value) const;
    const RightType &&RightOrDefault(const RightType &&default_value) const;
    RightType &&RightOrDefault(RightType &&default_value);

    // Returns right node of the Either. Asserts if this either is Left.
    RightType &Right();
    const RightType &Right() const;

    // If this Either is of type Left calls left_func otherwise right_func.
    template<class LeftFunc, class RightFunc>
    auto Fold(LeftFunc left_func, RightFunc right_func) ->
        typename std::enable_if<std::is_invocable<LeftFunc()>::value, decltype(left_func())>::type;
    template<class LeftFunc, class RightFunc>
    auto Fold(LeftFunc left_func, RightFunc right_func) ->
        typename std::enable_if<std::is_invocable<LeftFunc, LeftType&&>::value, decltype(left_func(std::move(left_)))>::type;

    // If this Either is of type Left calls fold_func and returns result.
    // Otherwise, returns the Right value.
    // FoldFunc needs to be function(LeftType arg) -> RightType.
    template<class FoldFunc>
    RightType FoldLeft(FoldFunc fold_func);

    // If this Either is of type Right calls fold_func and returns result.
    // Otherwise, returns the Left value.
    // FoldFunc needs to be callable with return type of Left Type.
    template<class FoldFunc>
    LeftType FoldRight(FoldFunc fold_func);

    // Returns Either<Result of a function, CurrentRight>.
    // If this Either is left performs function and returns left sided new Either.
    // If it's right - returns right sided new Either with right value from this Either.
    template<class LeftFunc>
    auto MapLeft(LeftFunc left_func) -> Either<decltype(left_func(std::move(left_))), RightType>;

    // Returns Either<CurrentLeft, Result of a function>.
    // If this Either is right performs function and returns right sided new Either.
    // If it's left - returns left sided new Either with left value from this Either.
    template<class RightFunc>
    auto MapRight(RightFunc right_func) -> Either<LeftType, decltype(right_func(std::move(right_)))>;

    // Requires that the left part of this Either is also an Either. ie. Either<Either<A, B>, B>
    // If this either is left -> left value is returned as is, otherwise A is joined with the right B and returned.
    LeftType JoinLeft();

    // Requires that the right part of this Either is also an Either. ie. Either[A, Either[A, B]]
    // If this either is right -> right value is returned as is, otherwise the left A is joined with the right B and returned.
    RightType JoinRight();
};


namespace detail {

template<class FoldFunc, class ResultType, class ArgType>
inline typename std::enable_if<std::is_invocable_r<ResultType, FoldFunc, ArgType>::value, ResultType>::type
CallFold(FoldFunc fold_func, ArgType &&arg) {
    return fold_func(std::forward<ArgType>(arg));
}

template<class FoldFunc, class ResultType, class ArgType>
inline typename std::enable_if<std::is_invocable_r<ResultType, FoldFunc>::value, ResultType>::type
CallFold(FoldFunc fold_func, ArgType &&) {
    return fold_func();
}
} // namespace detail


// Implementation.
template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(const Either<LeftType, RightType> &rhs)
    : is_left_(rhs.is_left_) {

    if (is_left_) {
        new(&left_) LeftType(rhs.left_); // Forcing copy-ctor
    } else {
        new(&right_) RightType(rhs.right_);
    }
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(Either<LeftType, RightType> &&rhs)
    : is_left_(rhs.is_left_) {

    if (is_left_) {
        new(&left_) LeftType(std::move(rhs.left_)); // Forcing move-ctor
    } else {
        new(&right_) RightType(std::move(rhs.right_));
    }
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(const LeftType &value)
    : left_(value)
    , is_left_(true) {
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(const RightType &value)
    : right_(value)
    , is_left_(false) {
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(LeftType &&value)
    : left_(std::move(value))
    , is_left_(true) {
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::Either(RightType &&value)
    : right_(std::move(value))
    , is_left_(false) {
}

template<class LeftType, class RightType>
Either<LeftType, RightType>::~Either() {
    // Have to manually call d-tors for unioned elements.
    if (is_left_) {
        left_.~LeftType();
    } else {
        right_.~RightType();
    }
}

template<class LeftType, class RightType>
Either<LeftType, RightType> &
Either<LeftType, RightType>::operator=(const Either<LeftType, RightType> &rhs) {
    is_left_ = rhs.is_left_;
    if (is_left_) {
        left_ = rhs.left_;
    } else {
        right_ = rhs.right_;
    }
    return *this;
}

template<class LeftType, class RightType>
Either<LeftType, RightType> &
Either<LeftType, RightType>::operator=(Either<LeftType, RightType> &&rhs) {
    is_left_ = rhs.is_left_;
    if (is_left_) {
        left_ = std::move(rhs.left_);
    } else {
        right_ = std::move(rhs.right_);
    }
    return *this;
}

template<class LeftType, class RightType>
bool Either<LeftType, RightType>::IsLeft() const {
    return is_left_;
}

template<class LeftType, class RightType>
bool Either<LeftType, RightType>::IsRight() const {
    return !is_left_;
}

template<class LeftType, class RightType>
LeftType &Either<LeftType, RightType>::Left() {
    XYAssert(IsLeft());
    return left_;
}

template<class LeftType, class RightType>
const LeftType &Either<LeftType, RightType>::Left() const {
    XYAssert(IsLeft());
    return left_;
}

template<class LeftType, class RightType>
RightType &Either<LeftType, RightType>::Right() {
    XYAssert(IsRight());
    return right_;
}

template<class LeftType, class RightType>
const RightType &Either<LeftType, RightType>::Right() const {
    XYAssert(IsRight());
    return right_;
}

template<class LeftType, class RightType>
const LeftType &Either<LeftType, RightType>::LeftOrDefault(const LeftType &default_value) const {
    if (is_left_) {
        return left_;
    } else {
        return default_value;
    }
}

template<class LeftType, class RightType>
const LeftType &&Either<LeftType, RightType>::LeftOrDefault(const LeftType &&default_value) const {
    if (is_left_) {
        return std::move(left_);
    } else {
        return std::move(default_value);
    }
}

template<class LeftType, class RightType>
LeftType &&Either<LeftType, RightType>::LeftOrDefault(LeftType &&default_value) {
    if (is_left_) {
        return std::move(left_);
    } else {
        return std::move(default_value);
    }
}

template<class LeftType, class RightType>
const RightType &Either<LeftType, RightType>::RightOrDefault(const RightType &default_value) const {
    if (is_left_) {
        return default_value;
    } else {
        return right_;
    }
}

template<class LeftType, class RightType>
const RightType &&Either<LeftType, RightType>::RightOrDefault(const RightType &&default_value) const {
    if (is_left_) {
        return std::move(default_value);
    } else {
        return std::move(right_);
    }
}

template<class LeftType, class RightType>
RightType &&Either<LeftType, RightType>::RightOrDefault(RightType &&default_value) {
    if (is_left_) {
        return std::move(default_value);
    } else {
        return std::move(right_);
    }
}

template<class LeftType, class RightType>
template<class LeftFunc, class RightFunc>
auto Either<LeftType, RightType>::Fold(LeftFunc left_func, RightFunc right_func) ->
    typename std::enable_if<std::is_invocable<LeftFunc()>::value, decltype(left_func())>::type {

    using ResultType = decltype(left_func());
    if (is_left_) {
        return detail::CallFold<LeftFunc, ResultType, LeftType>(left_func, std::move(left_));
    } else {
        return detail::CallFold<RightFunc, ResultType, RightType>(right_func, std::move(right_));
    }
}

template<class LeftType, class RightType>
template<class LeftFunc, class RightFunc>
auto Either<LeftType, RightType>::Fold(LeftFunc left_func, RightFunc right_func) ->
    typename std::enable_if<std::is_invocable<LeftFunc, LeftType&&>::value, decltype(left_func(std::move(left_)))>::type {

    using ResultType = decltype(left_func(std::move(left_)));
    if (is_left_) {
        return detail::CallFold<LeftFunc, ResultType, LeftType>(left_func, std::move(left_));
    } else {
        return detail::CallFold<RightFunc, ResultType, RightType>(right_func, std::move(right_));
    }
}

template<class LeftType, class RightType>
template<class FoldFunc>
RightType Either<LeftType, RightType>::FoldLeft(FoldFunc fold_func) {
    if (is_left_) {
        return detail::CallFold<FoldFunc, RightType, LeftType>(fold_func, std::move(left_));
    } else {
        return std::move(right_);
    }
}

template<class LeftType, class RightType>
template<class FoldFunc>
LeftType Either<LeftType, RightType>::FoldRight(FoldFunc fold_func) {
    if (is_left_) {
        return std::move(left_);
    } else {
        return detail::CallFold<FoldFunc, LeftType, RightType>(fold_func, std::move(right_));
    }
}

template<class LeftType, class RightType>
template<class LeftFunc>
auto Either<LeftType, RightType>::MapLeft(LeftFunc left_func) -> Either<decltype(left_func(std::move(left_))), RightType> {
    if (is_left_) {
        return left_func(std::move(left_));
    } else {
        return std::move(right_);
    }
}

template<class LeftType, class RightType>
template<class RightFunc>
auto Either<LeftType, RightType>::MapRight(RightFunc right_func) -> Either<LeftType, decltype(right_func(std::move(right_)))> {
    if (is_left_) {
        return std::move(left_);
    } else {
        return right_func(std::move(right_));
    }
}

template<class LeftType, class RightType>
LeftType Either<LeftType, RightType>::JoinLeft() {
    if (is_left_) {
        return std::move(left_);
    } else {
        return std::move(right_);
    }
}

template<class LeftType, class RightType>
RightType Either<LeftType, RightType>::JoinRight() {
    if (is_left_) {
        return std::move(left_);
    } else {
        return std::move(right_);
    }
}

}
