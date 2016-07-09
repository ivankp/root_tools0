#include "hist_fmt_re.hh"

#include <cctype>
#include <tuple>
#include <stdexcept>
#include <boost/regex.hpp>
#include <TH1.h>

#include "block_split.hh"
#include "substr.hh"
#include "interpreted_args.hh"

void hist_fmt_re::init(const std::string& str) {
  auto blocks = block_split(str,"/|:");
  auto it = blocks.begin(), end = blocks.end();
  if (it==end) throw std::runtime_error(
    "Cannot make hist_fmt_re from empty string");

  // set flag bits
  // 0: group
  // 1: name
  // 2: title
  // 3: x title
  // 4: y title
  // 5: legend
  // 6: file
  // 7: directories
  // TODO: implement
  if ((++it)==end) return;

  // set re
  if (it->size()) re = new boost::regex(*it);
  if ((++it)==end) return;

  // set fmt_fcns
  for (; it!=end; ++it) fmt_fcns.emplace_back(*it);
}

std::istream& operator>>(std::istream& in, hist_fmt_re& ref) {
  std::string buff;
  std::getline(in,buff,(char)EOF);
  ref.init(buff);
  return in;
}

bool hist_fmt_re::apply(TH1* h) const {
  // TODO: apply regex matching
  for (const auto& fcn : fmt_fcns) fcn.apply(h);
}

// ******************************************************************

struct hist_fmt_fcn_SetLineColor
: public hist_fmt_fcn_impl, public interpreted_args<Color_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineColor(std::get<0>(args)); }
};

struct hist_fmt_fcn_SetLineStyle
: public hist_fmt_fcn_impl, public interpreted_args<Style_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineStyle(std::get<0>(args)); }
};

struct hist_fmt_fcn_SetLineWidth
: public hist_fmt_fcn_impl, public interpreted_args<Width_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineWidth(std::get<0>(args)); }
};

// ******************************************************************

// syntax: function=arg1,arg2,arg3
// spaces don't matter
hist_fmt_fcn::hist_fmt_fcn(const std::string& str) {
  std::vector<substr> tokens;
  const char* c = str.c_str();
  const char* begin = c;
  char d = '=';
  for (;;) {
    while (*c!=d && *c!='\0') ++it;
    substr token(begin,c-begin);
    while (isspace(*token.ptr)) ++token.begin;
    while (isspace(*token.end  )) --token.end;
    tokens.emplace_back(token);

    if (it==end) break;
    else { begin = ++it; if (d=='=') d = ','; }
  }

  // find the right function
  if (tokens.front() == "LineColor") {
    impl = new hist_fmt_fcn_SetLineColor(tokens.begin(),tokens.end());
  } else if (tokens.front() == "LineStyle") {
    impl = new hist_fmt_fcn_SetLineStyle(tokens.begin(),tokens.end());
  } else if (tokens.front() == "LineWidth") {
    impl = new hist_fmt_fcn_SetLineWidth(tokens.begin(),tokens.end());
  }
}
