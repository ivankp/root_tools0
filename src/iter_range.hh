#ifndef ivanp_iter_range_hh
#define ivanp_iter_range_hh

#include <iterator>
#include <algorithm>

template <typename T>
struct iter_range {
  using iter_t = typename T::iterator;
  using diff_t = typename std::iterator_traits<iter_t>::difference_type;
  iter_t begin, end;
  iter_range(const T& x): begin(x.begin()), end(x.end()) { }
  iter_range(const T& x, diff_t a): begin(std::next(x.begin(),a)), end(x.end()) { }
  iter_range(iter_t begin, iter_t end): begin(begin), end(end) { }
  diff_t dist() const { return std::distance(begin,end); }

  bool operator==(const T& x) {
    if (dist()!=std::distance(x.begin,x.end)) return false;
    return std::equal(begin, end, x.begin());
  }
  bool operator<(const T& x) {
    return std::lexicographical_compare(begin, end, x.begin(), x.end());
  }
};

#endif
