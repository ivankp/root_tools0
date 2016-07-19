#ifndef array_istream_op_hh
#define array_istream_op_hh

#include <istream>
#include <sstream>
#include <array>
#include <type_traits>

template <size_t I, typename T, size_t N>
typename std::enable_if<(I==N-1)>::type
array_elem_from_istream(std::istream& in, std::array<T,N>& arr) {
  std::stringstream ss;
  in.get(*ss.rdbuf(),(char)EOF);
  ss >> std::get<I>(arr);
}

template <size_t I, typename T, size_t N>
typename std::enable_if<(I<N-1)>::type
array_elem_from_istream(std::istream& in, std::array<T,N>& arr) {
  std::stringstream ss;
  in.get(*ss.rdbuf(),':');
  ss >> std::get<I>(arr);
  in.ignore(); // skip delim char
  array_elem_from_istream<I+1>(in,arr);
}

namespace std {
  template <typename T, size_t N>
  istream& operator>>(istream& in, array<T,N>& arr) {
    array_elem_from_istream<0>(in,arr);
    return in;
  }
}

template <size_t I, typename T, size_t N>
typename std::enable_if<(I==N-1)>::type
array_elem_from_istream(std::istream& in, boost::array<T,N>& arr) {
  std::stringstream ss;
  in.get(*ss.rdbuf(),(char)EOF);
  ss >> arr[I];
}

template <size_t I, typename T, size_t N>
typename std::enable_if<(I<N-1)>::type
array_elem_from_istream(std::istream& in, boost::array<T,N>& arr) {
  std::stringstream ss;
  in.get(*ss.rdbuf(),':');
  ss >> arr[I];
  in.ignore(); // skip delim char
  array_elem_from_istream<I+1>(in,arr);
}

namespace boost {
  template <typename T, size_t N>
  std::istream& operator>>(std::istream& in, array<T,N>& arr) {
    array_elem_from_istream<0>(in,arr);
    return in;
  }
}

#endif
