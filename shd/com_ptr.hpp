#define _CRT_SECURE_NO_WARNINGS

// #include "impl/platform.h"

#include <type_traits>

/* COM pointer wrapper. */
template<class T> class RXCComPtr {
  public:
  RXCComPtr() = default;

  explicit RXCComPtr(T* ptr) : ptr_(ptr) {
    static_assert(std::is_base_of<IUnknown, T>::value);
  }

  template<class F> explicit RXCComPtr(F create_fn) {
    static_assert(std::is_invocable_r<HRESULT, F, T**>::value);
    const HRESULT create_result = create_fn(&ptr_);
    if (create_result != S_OK) ptr_ = nullptr;
  }

  RXCComPtr(RXCComPtr&& other) {
    *this = std::move(other);
  }

  ~RXCComPtr() {
    release();
  }

  RXCComPtr(const RXCComPtr&) = delete;
  RXCComPtr& operator=(const RXCComPtr&) = delete;

  RXCComPtr& operator=(RXCComPtr&& other) noexcept {
    release();
    ptr_       = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }

  T* operator->() {
    return ptr_;
  }

  const T* operator->() const {
    return ptr_;
  }

  T* get() {
    return ptr_;
  }

  const T* get() const {
    return ptr_;
  }

  T** PtrToContent() {
    return &ptr_;
  }

  private:
  void release() {
    if (ptr_) { ptr_->Release(); }
  }

  T* ptr_ = nullptr;
};
