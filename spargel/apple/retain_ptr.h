#pragma once

#include <CoreFoundation/CoreFoundation.h>

namespace spargel::apple {

// a scoped wrapper for CoreFoundation objects
template <typename T>
class RetainPtr {
public:
  RetainPtr() = default;
  explicit RetainPtr(T object) : object_{object} {}

  RetainPtr(RetainPtr const& that) : object_{that.get()} {
    if (object_) {
      CFRetain(object_);
    }
  }
  RetainPtr& operator=(RetainPtr const& that) {
    reset(that.get());
    if (object_) {
      CFRetain(object_);
    }
    return *this;
  }

  RetainPtr(RetainPtr&& that) : object_{that.release()} {}
  RetainPtr& operator=(RetainPtr&& that) {
    reset(that.get());
    if (object_) {
      CFRetain(object_);
    }
    return *this;
  }

  ~RetainPtr() {
    if (object_) {
      CFRelease(object_);
    }
  }

  void reset(T object = nullptr) {
    if (object_) {
      CFRelease(object_);
    }
    object_ = object;
  }

  [[nodiscard]] T release() {
    T tmp = object_;
    object_ = nullptr;
    return tmp;
  }

  T get() const { return object_; }

  friend bool operator==(RetainPtr lhs, RetainPtr rhs) {
    return lhs.object_ == rhs.object_;
  }

private:
  T object_ = nullptr;
};

}  // namespace spargel::apple
