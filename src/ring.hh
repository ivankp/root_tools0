#ifndef ring_hh
#define ring_hh

#include <vector>

template <typename T>
struct ring: public std::vector<T> {
  typedef std::vector<T> v_type;
  template <typename I> inline const T& operator[](I i) const noexcept {
    return v_type::operator[](i%size());
  }
  template <typename I> inline T& operator[](I i) noexcept {
    return v_type::operator[](i%size());
  }
  inline v_type* v_ptr() noexcept { return this; }
};

#endif
