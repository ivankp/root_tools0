#ifndef re_mod_base_hh
#define re_mod_base_hh

#include <string>
#include <vector>
// #include <memory>
// #include <bitset>

#ifdef USE_STD_REGEX
#include <regex>
#else
#include <boost/regex.hpp>
#endif

class re_mod_base {
  std::string flags;
  #ifdef USE_STD_REGEX
    std::regex *re;
  #else
    boost::regex *re;
  #endif
  std::string *subst;
  // d = delete
  // D = delete all but
  // s = select
  // S = inverse select
public:
  void init(const std::string& str);
  re_mod_base(): flags(), re(nullptr), subst(nullptr) { }
  re_mod_base(const std::string& str)
  : flags(), re(nullptr), subst(nullptr) { init(str); }
  ~re_mod_base();
  friend std::istream& operator>>(std::istream& in, re_mod_base& ref) {
    std::string buff;
    std::getline(in,buff,(char)EOF);
    ref.init(buff);
    return in;
  }
};

#endif
