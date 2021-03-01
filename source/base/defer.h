#pragma once

#include <utility>

namespace xynq {
// Defers execution of some lambda(or any callable) until the Defer object will
// leave its scope. ie.:
//  void foo() {
//    fp = fopen(...);
//    buf = new char[100];
//    Defer cleanup([&] { .     // Will defer some cleanup until function returns
//          fclose(some_file);
//          delete[] some_buffer;
//    });
//    ...
//  }
template<class T>
class Defer {
public:
    explicit Defer(T &&func)
        : func_(std::forward<T>(func))
    {}

    ~Defer() {
        func_();
    }

    Defer(const Defer &) = delete;
    Defer(Defer &&) = delete;
    Defer &operator=(const Defer &) = delete;
    Defer &operator=(Defer &&) = delete;
private:
    T func_;
};

} // xynq