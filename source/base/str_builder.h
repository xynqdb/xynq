#pragma once

#include <stddef.h>

#include "base/span.h"
#include "base/str_build_types.h"

namespace xynq {

template<size_t Size>
class StrBuilder {
    static_assert(Size > 0, "Invalid size");
public:
    StrBuilder() = default;

    // Clear all currently built data.
    void Clear();

    // Creates builder with stringified args.
    template<class Head, class...Tail>
    explicit StrBuilder(Head &&head, Tail&&...tail);

    template<class Head, class...Tail>
    inline void Append(Head &&head, Tail&&...tail);

    // Erases num characters from the back of the buffer.
    inline void Rollback(size_t num);

    // Std-streams like operator<<.
    // Appends single argument to a string.
    template<class T>
    inline StrBuilder &operator<<(T &&arg);

    // Writes data into Builder's buffer.
    template<class Handler>
    inline void Write(size_t needed_size, Handler handler);

    // Writes as much data as it can fit in the internal buffer.
    // Returns number of characters actually written.
    inline size_t BestEffortWrite(StrSpan str);

    // Current raw buffer built.
    // Not zero terminated.
    inline StrSpan Buffer() const;

    // Creates C-string and returns it.
    inline CStrSpan MakeCStr();
private:
    // Usable size is -1 to always have space for zero termination.
    static constexpr size_t UseSize = Size - 1;
    char buf_[Size];
    size_t offset_ = 0;

    inline void Append(){}
};
////////////////////////////////////////////////////////////


template<size_t Size>
void StrBuilder<Size>::Clear() {
    offset_ = 0;
}

template<size_t Size>
template<class Handler>
void StrBuilder<Size>::Write(size_t needed_size, Handler handler) {
    if (UseSize - offset_ < needed_size) {
        return;
    }

    offset_ += handler(MutSpan<char>{&buf_[0] + offset_, needed_size});
    XYAssert(offset_ <= UseSize);
}

template<size_t Size>
template<class T>
StrBuilder<Size> &StrBuilder<Size>::operator<<(T &&arg) {
    Append(arg);
    return *this;
}

template<size_t Size>
template<class Head, class...Tail>
StrBuilder<Size>::StrBuilder(Head &&head, Tail&&...tail) {
    Append(std::forward<Head>(head), std::forward<Tail>(tail)...);
}

template<size_t Size>
template<class Head, class...Tail>
void StrBuilder<Size>::Append(Head &&head, Tail&&...tail) {
    using T = typename std::remove_cv<typename std::remove_reference<Head>::type>::type;
    StrBuild<T>()(std::forward<Head>(head), *this);
    Append(std::forward<Tail>(tail)...);
}

template<size_t Size>
void StrBuilder<Size>::Rollback(size_t num) {
    XYAssert(num <= offset_);
    offset_ -= num;
}


template<size_t Size>
StrSpan StrBuilder<Size>::Buffer() const {
    return {&buf_[0], offset_};
}

template<size_t Size>
CStrSpan StrBuilder<Size>::MakeCStr() {
    XYAssert(offset_ < Size);
    buf_[offset_] = '\0';
    return {&buf_[0], offset_};
}

template<size_t Size>
size_t StrBuilder<Size>::BestEffortWrite(StrSpan str) {
    size_t sz = std::min(UseSize - offset_, str.Size());
    memcpy(&buf_[0] + offset_, str.Data(), sz);
    offset_ += sz;
    return sz;
}

} // xynq
