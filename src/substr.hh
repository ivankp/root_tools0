#ifndef ivanp_substr_hh
#define ivanp_substr_hh

#include <string>
#include <algorithm>

struct substr {
  const char* ptr;
  size_t n;
  substr(const std::string& str): ptr(str.c_str()), n(str.size()) { }
  substr(const std::string& str, size_t n): ptr(str.c_str()), n(n) { }
  substr(const char* ptr, size_t n): ptr(ptr), n(n) { }
  inline const char* data() const noexcept { return ptr; }
  inline size_t size() const noexcept { return n; }

  std::string str() const { return std::move(std::string(ptr,n)); }

  bool operator==(const std::string& str) {
    if (str.size()!=n) return false;
    return std::equal(ptr, ptr+n, str.data());
  }
  bool operator<(const std::string& str) {
    return std::lexicographical_compare(
      ptr, ptr+n, str.data(), str.data()+str.size());
  }
};

#endif
