#ifndef _MY_BASE_DEFER_HPP
#define _MY_BASE_DEFER_HPP
#pragma once
#include <type_traits> // for std::move


namespace my {

  /*
namespace glslang {
// An object that, when destroyed, executes a given function.
// Use this to perform work along all exit paths from a function.
template <typename F>
class Defer {
 public:
  explicit Defer(F&& f) : f_(std::move(f)) { }
  Defer(Defer&&) = default;
  ~Defer() { f_(); } // Run the given function.
 private:
  Defer(const Defer&) = delete;
  Defer& operator=(const Defer&) = delete;
  F f_;
};

// Template argument deduction guide for Defer.
template <typename T>
Defer(T) -> Defer<T>;

} // namespace glslang
  */

  template<typename F>
  class Defer final {
  public:
    explicit Defer(F&& fn) : fn_(std::move(fn)) {}
    Defer(Defer&&) = default;
    ~Defer() { fn_(); }

    //void dismiss();
  private:
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;

    F fn_;
    //bool active_;
  };
  // Template argument deduction guide for Defer.
  template <typename T>
  Defer(T) -> Defer<T>;

  namespace detail {
#if 0
    // Usage1
    template <typename F>
    inline Defer<F> make_defer(F&& func) {
      return Defer<F>(std::move(func));
    }
#else
    // Usage2
    struct DeferTag {
      explicit DeferTag() = default;
    };

    template<typename F>
    inline Defer<F> operator+(DeferTag, F&& fn) {
      return Defer<F>(std::move(fn));
    }
#endif
  }

} // namespcae my



// =======heleper macro======
#define MY_DEFER_CONCAT(s1,s2) s1##s2
#define MY_DEFER_CONCAT_NAME(s1,s2) MY_DEFER_CONCAT(s1,s2)

#if 0
// Usage1: DEFER([...] {...;});
#define DEFER(lambda_code)\
  auto MY_DEFER_CONCAT_NAME(MY_DEFER_,__COUNTER__)  = my::detail::make_defer(lambda_code)

#else

// Usage2:  DEFER[...] {...;};
#define DEFER \
  auto MY_DEFER_CONCAT_NAME(MY_DEFER_,__COUNTER__) = my::detail::DeferTag{} +
#endif



#endif // !_MY_BASE_DEFER_HPP


