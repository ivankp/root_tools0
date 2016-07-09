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
  bool from = false, to = false;
  for (const char *c = it->c_str(); *c!='\0'; ++c) {
    bool fromto = false;
    switch (*c) {
      case 'r': flags.r = 1; continue;
      case 's': flags.s = 1; continue;
      case 'i': flags.i = 1; continue;
      case 'g': flags.to = 0; fromto = true; break; // group
      case 't': flags.to = 1; fromto = true; break; // title
      case 'x': flags.to = 2; fromto = true; break; // x title
      case 'y': flags.to = 3; fromto = true; break; // y title
      case 'l': flags.to = 4; fromto = true; break; // legend
      case 'n': flags.to = 5; fromto = true; break; // name
      case 'f': flags.to = 6; fromto = true; break; // file
      case 'd': flags.to = 7; fromto = true; break; // directories
      default: break;
    }
    if (fromto) {
      if (!from) {
        from = true;
        flags.from = flags.to;
      } else if (!to) to = true;
      else throw std::runtime_error("too many from/to flags in "+str);
    } else if (isdigit(*c)) {
      if (fromto) {
        std::string num_str;
        num_str.reserve(4);
        for (; isdigit(*c); ++c) num_str += *c;
        unsigned num = stoi(num_str);
        if (num>254) throw std::runtime_error(
          "overly large number "+num_str+" in "+str);
        if (to) flags.to_i = num;
        else flags.from_i = num;
      } else throw std::runtime_error(
        "number not preceded by from/to flags in "+str);
    } else throw std::runtime_error("unknown flag "+(*c+(" in "+str)));
  }
  if (!from) throw std::runtime_error("no from/to flags specified in "+str);
  if ((++it)==end) return;

  // set re
  if (it->size()) re = new boost::regex(*it);
  if ((++it)==end) return;

  // set subst
  if (it->size()) subst = std::move(*it);
  if ((++it)==end) return;

  // set fmt_fcns
  fmt_fcns.reserve(end-it);
  for (; it!=end; ++it) fmt_fcns.emplace_back(*it);
}

hist_fmt_re::~hist_fmt_re() { if (re) delete re; }
hist_fmt_re::hist_fmt_re(const hist_fmt_re& o)
: flags(o.flags), re(o.re), subst(o.subst), fmt_fcns(o.fmt_fcns) { }
hist_fmt_re::hist_fmt_re(hist_fmt_re&& o)
: flags(o.flags), re(o.re),
  subst(std::move(o.subst)), fmt_fcns(std::move(o.fmt_fcns))
{
  o.re = nullptr;
  o.subst = nullptr;
}
hist_fmt_re& hist_fmt_re::operator=(const hist_fmt_re& o) {
  flags = o.flags;
  re = o.re;
  subst = o.subst;
  fmt_fcns = o.fmt_fcns;
  return *this;
}
hist_fmt_re& hist_fmt_re::operator=(hist_fmt_re&& o) {
  flags = o.flags;
  re = o.re; o.re = nullptr;
  subst = o.subst; o.subst = nullptr;
  fmt_fcns = std::move(o.fmt_fcns);
  return *this;
}

std::istream& operator>>(std::istream& in, hist_fmt_re& ref) {
  std::string buff;
  std::getline(in,buff,(char)EOF);
  ref.init(buff);
  return in;
}

bool hist_fmt_re::apply(TH1* h) const {
  // TODO: apply regex matching
  // TODO: containers for intermediate strings
  for (const auto& fcn : fmt_fcns) fcn.apply(h);
  return true;
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
    while (*c!=d && *c!='\0') ++c;
    std::pair<const char*,const char*> cc(begin,c-1);
    while (isspace(*cc.first )) ++cc.first;
    while (isspace(*cc.second)) --cc.second;
    tokens.emplace_back(cc.first,cc.second-cc.first+1);

    if (*c=='\0') break;
    else { begin = ++c; if (d=='=') d = ','; }
  }

  // find the right function
  try {
    if (tokens.front() == "LineColor") {
      impl = new hist_fmt_fcn_SetLineColor(++tokens.begin(),tokens.end());
    } else if (tokens.front() == "LineStyle") {
      impl = new hist_fmt_fcn_SetLineStyle(++tokens.begin(),tokens.end());
    } else if (tokens.front() == "LineWidth") {
      impl = new hist_fmt_fcn_SetLineWidth(++tokens.begin(),tokens.end());
    } else throw std::runtime_error("unknown function "+tokens.front().str());
  } catch (const std::exception& e) {
    throw std::runtime_error("in "+str+": "+e.what());
  }
}
