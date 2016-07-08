#include "re_mod_base.hh"

#include <iterator>
#include "in.hh"

#ifdef DEBUG
#include <iostream>
#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;
#else
#define test(var)
#endif

void re_mod_base::init(const std::string& str) {
  auto it=str.begin(), end=str.end();
  if (it==end) return;
  test(str)

  for (; it!=end; ++it)
    if (in(*it,'/','|',':')) break;
  flags = { str.begin(), it };
  test(flags)

  if (it==end) return;
  const char d = *(it++);

  bool get_re = true;
  if (it==end) return; // single delim after flags
  else if (*it==d) { // two delims after flags
    get_re = false;
    ++it;
    // continue because subst string may be useful even without re
  }

  if (get_re) {
    std::string re_str;
    for (auto begin = it; ; ) {
      if (*it==d) {
        // ??/ T
        // ?\/ F
        // \\/ T
        // \?/ T
        if (*std::prev(it)=='\\') {
          int num_bs = 1;
          for (auto it1=std::prev(it,2); *it1=='\\'; --it1) ++num_bs;
          if (num_bs%2) {
            (re_str += { begin, std::prev(it,(num_bs+1)/2) }) += d;
            begin = ++it;
          } else {
            re_str += { begin, std::prev(it,(num_bs+1)/2) };
            ++it;
            break;
          }
        } else {
          re_str += { begin, it++ };
          break;
        }
      } else if (it==end) {
        re_str += { begin, it };
        break;
      } else ++it;
    }
    test(re_str)
    re = new std::decay<decltype(*re)>::type(re_str);
  } else if (re) {
    delete re;
    re = nullptr;
  }

  if (it != end) subst = new std::string(it,end);
  else if (subst) {
    delete subst;
    subst = nullptr;
  }
  if (subst) test(*subst)
}

re_mod_base::~re_mod_base() {
  if (re) delete re;
  if (subst) delete subst;
}
