#ifndef hist_fmt_re_hh
#define hist_fmt_re_hh

#include <iosfwd>
#include <string>
#include <vector>
#include <memory>

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
public:
  struct hist_wrap {
    typedef std::shared_ptr<std::string> shared_str;
    TH1 *h;
    shared_str group, legend;
    hist_wrap(TH1* h, const shared_str& group, const shared_str& legend);
    hist_wrap(TH1* h, shared_str&& group, shared_str&& legend);
    hist_wrap(TH1* h, const shared_str& group, shared_str&& legend);
    hist_wrap(TH1* h, shared_str&& group, const shared_str& legend);
    hist_wrap(TH1* h, const std::string& group, const std::string& legend);
    hist_wrap(TH1* h, std::string&& group, std::string&& legend);
    hist_wrap(TH1* h, const std::string& group, std::string&& legend);
    hist_wrap(TH1* h, std::string&& group, const std::string& legend);
    hist_wrap(hist_wrap&& o) noexcept
    : h(o.h), group(std::move(o.group)), legend(std::move(o.legend)) {
      o.h = nullptr;
    }
  };
  struct flags_t {
    enum fromto { g=0, t=1, x=2, y=3, l=4, n=5, f=6, d=7 };
    unsigned int s    : 1; // select
    unsigned int i    : 1; // invert selection and matching
    unsigned int mod  : 2; // mod
    unsigned int      : 0;
    fromto       from : 4; // string source
    fromto       to   : 4; // string being set
    signed int from_i : 8; // python index sign convention
    flags_t(): s(0), i(0), mod(0), from(g), to(g), from_i(-1) { }
  };
private:
  flags_t flags;
  boost::regex *re;
  std::string subst;
  std::vector<hist_fmt_fcn> fmt_fcns;
public:
  void init(const std::string& str);
  hist_fmt_re(): flags(), re(nullptr), subst(), fmt_fcns() { }
  hist_fmt_re(const std::string& str)
  : flags(), re(nullptr), subst(), fmt_fcns() { init(str); }
  ~hist_fmt_re();

  hist_fmt_re(const hist_fmt_re& o) noexcept;
  hist_fmt_re(hist_fmt_re&& o) noexcept;
  hist_fmt_re& operator=(const hist_fmt_re& o) noexcept;
  hist_fmt_re& operator=(hist_fmt_re&& o) noexcept;

  friend bool apply(const std::vector<hist_fmt_re>& expressions, hist_wrap& h);
};

std::istream& operator>>(std::istream& in, hist_fmt_re& ref);

#endif
