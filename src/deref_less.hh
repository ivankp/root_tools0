#ifndef deref_less_hh
#define deref_less_hh

#include <utility>

template <typename T>
struct deref_less {
  inline bool operator()(const T& x, const T& y) const
  noexcept(noexcept(*x < *y)) {
    return *x < *y;
  }
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef bool result_type;
  typedef decltype(*std::declval<T>()) value_type;
};

#endif
