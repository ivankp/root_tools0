// Developed by Ivan Pogrebnyak, MSU

#ifndef IVANP_CATSTR_HH
#define IVANP_CATSTR_HH

#include <string>
#include <sstream>
#include <utility>

namespace ivanp {

namespace detail {

template <typename T>
inline void cat_impl(std::stringstream& ss, const T& t) { ss << t; }

template <typename T, typename... TT>
inline void cat_impl(std::stringstream& ss, const T& t, const TT&... tt) {
  ss << t;
  cat_impl(ss,tt...);
}

}

template <typename... TT>
inline std::string cat(const TT&... tt) {
  std::stringstream ss;
  ivanp::detail::cat_impl(ss,tt...);
  return ss.str();
}

}

#endif
