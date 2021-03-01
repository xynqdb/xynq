#pragma once

#include "assert.h"

#include <utility>

#if defined(XYNQ_TRACK_DEP_LIFETIME)
    #include <atomic>
    #include <memory>
#endif

namespace xynq {
namespace detail {

#if defined(XYNQ_TRACK_DEP_LIFETIME)
class DepLifetime {
public:
    inline DepLifetime() { alive_ = true; }
    inline bool IsAlive() const { return alive_; }
    inline void Kill() { alive_ = false; }
private:
    std::atomic<bool> alive_;
};
#endif

} // detail

// Dependable by value.
template<class T>
class Dependable {
    template<class DepType>
    friend class Dep;
public:
    template<class...Args>
    inline Dependable(Args&&...args);
    inline Dependable(T &&object);
    inline Dependable(Dependable &&) = default;
    inline Dependable &operator=(Dependable &&) = default;
    inline ~Dependable();

    inline T *operator->() { return &object_; }
    inline const T *operator->() const { return &object_; }
    inline T &Get() { return object_; }
    inline const T &Get() const { return object_; }

    Dependable(const T&) = delete;
    Dependable &operator=(const Dependable &) = delete;
private:
    T object_;

#if defined(XYNQ_TRACK_DEP_LIFETIME)
    std::shared_ptr<detail::DepLifetime> lifetime_;
#endif
};

// Dependable by pointer.
template<class T>
class Dependable<T*> {
    template<class DepType>
    friend class Dep;
public:
    template<class...Args>
    inline Dependable(Args&&...args);
    inline Dependable(T *object);
    inline Dependable(Dependable &&);
    inline Dependable &operator=(Dependable &&);
    inline ~Dependable();

    inline T *operator->() { XYAssert(object_ != nullptr); return object_; }
    inline const T *operator->() const { XYAssert(object_ != nullptr); return object_; }
    inline T &Get() { XYAssert(object_ != nullptr); return *object_; }
    inline const T &Get() const { XYAssert(object_ != nullptr); return *object_; }

    inline bool IsValid() const { return object_ != nullptr; }

    inline Dependable(const T&) = delete;
    Dependable &operator=(const Dependable &) = delete;
private:
    T *object_ = nullptr;

#if defined(XYNQ_TRACK_DEP_LIFETIME)
    std::shared_ptr<detail::DepLifetime> lifetime_;
#endif
};
////////////////////////////////////////////////////////////


// Dependency.
template<class T>
class Dep {
    template<class U> friend class Dep;
public:
    Dep();
    Dep(std::nullptr_t);
    Dep(Dependable<T*> &dependable);
    Dep(Dependable<T> &dependable);

    template<class U>
    Dep(Dependable<U*> &dependable);

    template<class U>
    Dep(Dependable<U> &dependable);

    template<class U>
    Dep(const Dep<U> &other);
    Dep(const Dep<T> &other);

    template<class U>
    Dep(Dep<U> &&other);
    Dep(Dep<T> &&other);
    ~Dep();

    T *operator->() { return ptr_; }
    const T *operator->() const { return ptr_; }
    operator T*() { return ptr_; }
    operator const T*() const { return ptr_; }

    Dep<T> &operator=(const Dep<T> &other);
    Dep<T> &operator=(Dep<T> &&other);
private:
    T *ptr_;

#if defined(XYNQ_TRACK_DEP_LIFETIME)
    std::shared_ptr<detail::DepLifetime> lifetime_;
#endif
};
////////////////////////////////////////////////////////////


// Dependable (T).
template<class T>
template<class...Args>
Dependable<T>::Dependable(Args&&...args)
    : object_{std::forward<Args>(args)...} {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::make_shared<detail::DepLifetime>();
#endif
}

template<class T>
Dependable<T>::Dependable(T &&object)
    : object_(std::forward<T>(object)) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::make_shared<detail::DepLifetime>();
#endif
}

template<class T>
Dependable<T>::~Dependable() {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    XYAssert(lifetime_ != nullptr);
    XYAssert(lifetime_->IsAlive());
    XYAssert(lifetime_.use_count() == 1);
    lifetime_->Kill();
    lifetime_ = nullptr;
#endif
}

// Dependable (T*).
template<class T>
template<class...Args>
Dependable<T*>::Dependable(Args&&...args)
    : object_{std::forward<Args>(args)...} {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::make_shared<detail::DepLifetime>();
#endif
}

template<class T>
Dependable<T*>::Dependable(Dependable<T*> &&other) {
    std::swap(object_, other.object_);
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    std::swap(lifetime_, other.lifetime_);
#endif
}

template<class T>
Dependable<T*> &Dependable<T*>::operator=(Dependable<T*> &&other) {
    std::swap(object_, other.object_);
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    std::swap(lifetime_, other.lifetime_);
#endif
    return *this;
}

template<class T>
Dependable<T*>::Dependable(T *object)
    : object_{object} {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::make_shared<detail::DepLifetime>();
#endif
}

template<class T>
Dependable<T*>::~Dependable() {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    if (object_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive());
        XYAssert(lifetime_.use_count() == 1);
        lifetime_->Kill();
    }
    lifetime_ = nullptr;
#endif

    delete object_;
}
////////////////////////////////////////////////////////////


// Dep.
template<class T>
Dep<T>::Dep(Dependable<T*> &dependable)
    : ptr_(dependable.object_) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = dependable.lifetime_;
    XYAssert(lifetime_ != nullptr);
    XYAssert(lifetime_->IsAlive()); // dead on arrival
#endif
}

template<class T>
Dep<T>::Dep()
    : ptr_(nullptr) {
}

template<class T>
Dep<T>::Dep(std::nullptr_t)
    : ptr_(nullptr) {
}

template<class T>
Dep<T>::Dep(Dependable<T> &dependable)
    : ptr_(&dependable.object_)
{
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = dependable.lifetime_;
    XYAssert(lifetime_ != nullptr);
    XYAssert(lifetime_->IsAlive()); // dead on arrival
#endif
}

template<class T>
template<class U>
Dep<T>::Dep(Dependable<U*> &dependable)
    : ptr_(dependable.object_) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = dependable.lifetime_;
    XYAssert(lifetime_ != nullptr);
    XYAssert(lifetime_->IsAlive()); // dead on arrival
#endif
}

template<class T>
template<class U>
Dep<T>::Dep(Dependable<U> &dependable)
    : ptr_(&dependable.object_) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = dependable.lifetime_;
    XYAssert(lifetime_ != nullptr);
    XYAssert(lifetime_->IsAlive()); // dead on arrival
#endif
}

template<class T>
template<class U>
Dep<T>::Dep(const Dep<U> &other)
    : ptr_(other.ptr_) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = other.lifetime_;

    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive()); // dead on arrival
    }
#endif
}

template<class T>
Dep<T>::Dep(const Dep<T> &other)
    : ptr_(other.ptr_) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = other.lifetime_;
    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive()); // dead on arrival
    }
#endif
}

template<class T>
template<class U>
Dep<T>::Dep(Dep<U> &&other)
    : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::move(other.lifetime_);

    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive()); // dead on arrival
    }
#endif
}

template<class T>
Dep<T>::Dep(Dep<T> &&other)
    : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    lifetime_ = std::move(other.lifetime_);

    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive()); // dead on arrival
    }
#endif
}

template<class T>
Dep<T> &Dep<T>::operator=(const Dep<T> &other) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive());
    }
    lifetime_ = other.lifetime_;
#endif

    ptr_ = other.ptr_;
    return *this;
}

template<class T>
Dep<T> &Dep<T>::operator=(Dep<T> &&other) {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive()); // dead on arrival
    }

    lifetime_ = std::move(other.lifetime_);
#endif
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
}

template<class T>
Dep<T>::~Dep() {
#if defined(XYNQ_TRACK_DEP_LIFETIME)
    if (ptr_ != nullptr) {
        XYAssert(lifetime_ != nullptr);
        XYAssert(lifetime_->IsAlive());
    }
    lifetime_ = nullptr;
#endif
}

} // xynq
