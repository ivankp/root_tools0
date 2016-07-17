#ifndef ivanp_iter_range_hh
#define ivanp_iter_range_hh

#include <iterator>
#include <type_traits>

template <typename Iter, bool Deref,
  typename std::iterator_traits<iterator>::difference_type Increment>
class iter_range {
public:
  using iterator = Iter;
  using deref_t = typename std::conditional< Deref,
    decltype(*std::declval<iterator>()),
    iterator >::type;
  using diff_t = typename std::iterator_traits<iterator>::difference_type;

  struct iter_wrap {
    iterator i;
    inline std::enable_if<!Deref,deref_t> operator*() const noexcept {
      return i;
    }
    inline std::enable_if< Deref,deref_t> operator*() const noexcept {
      return *i;
    }
    inline iterator operator++() noexcept(noexcept(std::advance(i,Increment))) {
      std::advance(i,Increment);
      return i;
    }
    inline bool operator!=(const iter_wrap& o) const noexcept {
      return i != o.i;
    }
  };

private:
  iter_wrap _begin, _end;

public:
  iter_range(iterator begin, iterator end): _begin{begin}, _end{end} { }
  inline diff_t dist() const noexcept(noexcept(std::distance(begin,end))) {
    return std::distance(begin,end);
  }

  inline iter_wrap begin() const noexcept { return _begin; }
  inline iter_wrap   end() const noexcept { return   _end; }
};

// template <typename Iter, bool Deref,
//   typename std::iterator_traits<iterator>::difference_type Increment>
// iter_range<Iter,Deref,Increment> make_iter_range(Iter begin, Iter end) {
//   return {begin,end};
// }

#endif
