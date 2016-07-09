#ifndef hist_fmt_re_hh
#define hist_fmt_re_hh

#include <iofwd>
#include <string>
#include <vector>

#include <boost/regex_fwd.hpp>

class TH1;

struct hist_fmt_fcn_impl {
  virtual void apply(TH1* h) const =0;
  virtual ~hist_fmt_fcn_impl() { }
};

class hist_fmt_fcn {
  hist_fmt_fcn_impl* impl;
public:
  hist_fmt_fcn(const std::string& str);
  ~hist_fmt_fcn() { delete impl; }
  inline void apply(TH1* h) const { impl->apply(h); }
};

class hist_fmt_re {
  struct flags_t {
    unsigned int r : 1; // replace
    unsigned int s : 1; // select
    unsigned int i : 1; // invert selection
    unsigned int : 0;
    unsigned int from : 4; // string source
    unsigned int to   : 4; // string being set
    unsigned int from_i : 8;
    unsigned int to_i   : 8;
    flags_t(): r(0), s(0), i(0), from(0), to(0), from_i(255), to_i(255) { }
  } flags;
  boost::regex *re;
  std::string subst;
  std::vector<hist_fmt_fcn> fmt_fcns;
public:
  void init(const std::string& str);
  hist_fmt_re(): flags(), re(nullptr), subst(nullptr), fmt_fcns() { }
  hist_fmt_re(const std::string& str)
  : flags(), re(nullptr), subst(nullptr), fmt_fcns() { init(str); }
  ~hist_fmt_re() { if (re) delete re; }

  hist_fmt_re(const hist_fmt_re& o)
  : flags(o.flags), re(o.re), subst(o.subst), fmt_fcns(o.fmt_fcns) { }
  hist_fmt_re(hist_fmt_re&& o)
  : flags(o.flags), re(o.re), subst(o.subst), fmt_fcns(std::move(o.fmt_fcns))
  {
    o.re = nullptr;
    o.subst = nullptr;
  }
  hist_fmt_re& operator=(const hist_fmt_re& o) {
    flags = o.flags;
    re = o.re;
    subst = o.subst;
    fmt_fcns = o.fmt_fcns;
  }
  hist_fmt_re& operator=(hist_fmt_re&& o) {
    flags = o.flags;
    re = o.re; o.re = nullptr;
    subst = o.subst; o.subst = nullptr;
    fmt_fcns = std::move(o.fmt_fcns);
  }

  bool apply(TH1* h) const;
};

std::istream& operator>>(std::istream& in, hist_fmt_re& ref);

#endif
