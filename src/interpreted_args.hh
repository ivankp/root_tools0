#ifndef ivanp_interpreted_args_hh
#define ivanp_interpreted_args_hh

#include <type_traits>
#include <tuple>
#include <iterator>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

template <typename... Args>
class interpreted_args {
public:
  std::tuple<Args...> args;
  template <std::size_t N>
  using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
private:
  template <std::size_t I, typename InputIterator>
  void convert_impl(InputIterator it) {
    std::get<I>(args) = boost::lexical_cast<type<I>>(it->data(),it->size());
  }
  template <std::size_t I, typename InputIterator>
  inline typename std::enable_if<(I<sizeof...(Args)-1)>::type
  convert(InputIterator it) {
    convert_impl<I>(it);
    convert<I+1>(++it);
  }
  template <std::size_t I, typename InputIterator>
  inline typename std::enable_if<(I==sizeof...(Args)-1)>::type
  convert(InputIterator it) {
    convert_impl<I>(it);
  }
public:
  template <typename InputIterator>
  interpreted_args(InputIterator begin, InputIterator end) {
    if (std::distance(begin,end)!=sizeof...(Args)) throw std::runtime_error(
      "Interpreted arguments number mismatch"
    );
    convert<0>(begin);
  }
  virtual ~interpreted_args() { }
};

#endif
