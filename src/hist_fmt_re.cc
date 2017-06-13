#include "hist_fmt_re.hh"

#include <cctype>
#include <tuple>
#include <array>
#include <stdexcept>

#include <boost/utility/string_ref.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <TH1.h>
#include <TAxis.h>
#include <TFile.h>
#include <TDirectory.h>

#include "block_split.hh"
#include "interpreted_args.hh"

#include <iostream>
#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

void hist_fmt_re::init(const std::string& str) {
  auto blocks = block_split(str,"/|:");
  auto it = blocks.begin(), end = blocks.end();
  if (it==end) throw std::runtime_error(
    "Cannot make hist_fmt_re from empty string");

  // set flag bits
  bool from = false, to = false, prev_fromto = false;
  for (const char *c = it->c_str(); *c!='\0'; ++c) {
    bool fromto = false;
    switch (*c) {
      case 's': flags.s = 1; prev_fromto = false; continue;
      case 'i': flags.i = 1; prev_fromto = false; continue;
      case 'm': flags.m = 1; prev_fromto = false; continue;
      case 'g': flags.to = flags_t::g; fromto = true; break; // group
      case 't': flags.to = flags_t::t; fromto = true; break; // title
      case 'x': flags.to = flags_t::x; fromto = true; break; // x title
      case 'y': flags.to = flags_t::y; fromto = true; break; // y title
      case 'z': flags.to = flags_t::z; fromto = true; break; // z title
      case 'l': flags.to = flags_t::l; fromto = true; break; // legend
      case 'n': flags.to = flags_t::n; fromto = true; break; // name
      case 'f': flags.to = flags_t::f; fromto = true; break; // file
      case 'd': flags.to = flags_t::d; fromto = true; break; // directories
      case '+': { // concatenate
        // mod 0 = none
        // mod 1 = replace
        // mod 2 = append
        // mod 3 = prepend
        if (flags.mod) throw std::runtime_error("too many \'+\' in "+str);
        if (to) flags.mod = 2;
        else if (from) flags.mod = 3;
        else throw std::runtime_error("\'+\' without \'from/to\' in "+str);
        prev_fromto = false;
        continue;
      }
      default: break;
    }
    if (fromto) {
      if (!from) {
        from = true;
        flags.from = flags.to;
      } else if (!to) to = true;
      else throw std::runtime_error("too many \'from/to\' flags in "+str);
      prev_fromto = true;
    } else if (isdigit(*c) || *c=='-') {
      if (prev_fromto) {
        if (to) throw std::runtime_error("number after \'to\' in "+str);
        std::string num_str;
        num_str.reserve(4);
        for (; isdigit(*c) || *c=='-'; ++c) num_str += *c;
        --c; // to prevent incrementing past the first char after digits
        signed int num = stoi(num_str);
        if (num>127 || num<-128) throw std::runtime_error(
          "number "+num_str+" is too large in "+str);
        flags.from_i = num;
      } else throw std::runtime_error("number not preceded by \'from\' in "+str);
    } else throw std::runtime_error("unknown flag "+(*c+(" in "+str)));
  }
  if (flags.mod && !to) throw std::runtime_error(
    "expected \'to\' after \'+\' in "+str);
  if (from && to && !flags.mod) flags.mod = 1;
  // test(from)
  // test(to)
  // test(flags.from)
  // test(flags.from_i)
  // test(flags.to)
  // test(flags.s)
  // test(flags.i)
  // test(flags.mod)
  if ((++it)==end) return;

  // set re
  if (it->size()) re = new boost::regex(*it);
  if ((++it)==end) return;

  // set subst
  if (it->size()) // subst = std::move(*it);
    subst = boost::replace_all_copy(*it,"**","#");
  if (re && !flags.mod && subst.size()) flags.mod = 1;
  if ((++it)==end) return;

  // set fmt_fcns
  fmt_fcns.reserve(end-it);
  for (; it!=end; ++it) fmt_fcns.emplace_back(*it);
}

using shared_str = std::shared_ptr<std::string>;

shared_str get_hist_str(
  hist_fmt_re::hist_wrap& h, hist_fmt_re::flags_t::fromto flag)
{
  std::string str;
  using flags_t = hist_fmt_re::flags_t;
  switch (flag) {
    case flags_t::g: return h.group; break;
    case flags_t::t: str = h.h->GetTitle(); break;
    case flags_t::x: str = h.h->GetXaxis()->GetTitle(); break;
    case flags_t::y: str = h.h->GetYaxis()->GetTitle(); break;
    case flags_t::z: str = h.h->GetZaxis()->GetTitle(); break;
    case flags_t::l: return h.legend; break;
    case flags_t::n: str = h.h->GetName(); break;
    case flags_t::f: str = std::move(get_hist_file_str(h.h)); break;
    case flags_t::d: str = std::move(get_hist_dirs_str(h.h)); break;
    // TODO: get file name
  }
  return std::move(std::make_shared<std::string>(std::move(str)));
}

bool apply(
  const std::vector<hist_fmt_re>& expressions,
  hist_fmt_re::hist_wrap& h)
{
  // array of temporary strings
  std::array<std::vector<shared_str>,9> share;
  for (auto& v : share) { /*v.reserve(2);*/ v.emplace_back(); }

  // loop over expressions
  for (const auto& re : expressions) {
    // get the string
    auto& vec = share[re.flags.from];
    int index = re.flags.from_i;
    if (index<0) index += vec.size();
    if (index<0 || (unsigned(index))>vec.size())
      throw std::runtime_error("out of range string version index");
    auto& str = vec[index];
    if (!str) str = get_hist_str(h,re.flags.from);

    bool matched = true;
    if (re.re) { // applying regex
      // match
      boost::smatch matches;
      matched = boost::regex_match(*str, matches, *re.re);
      if (re.flags.i) matched = !matched;
      if (re.flags.s && !matched) return false;
      // replace
      if (re.flags.mod && (!re.flags.m || matched)) {
        share[re.flags.to].emplace_back(std::make_shared<std::string>(
          boost::regex_replace(*str, *re.re, re.subst,
            boost::match_default | boost::format_sed)
        ));
      }
    } else if (re.flags.mod) { // no regex, modify
      if (re.flags.mod==1) // no copy, share
        share[re.flags.to].emplace_back(str);
      else // need a copy
        share[re.flags.to].emplace_back(std::make_shared<std::string>(*str));
    }

    // concatenate if necessary
    if (re.flags.mod>1) {
      auto& v_to = share[re.flags.to];
      auto& str_to = *(v_to.end()-2);
      if (!str_to) str_to = get_hist_str(h,re.flags.to);

      switch (re.flags.mod) {
        case 2: *v_to.back() = *str_to + *v_to.back(); break;
        case 3:  v_to.back()->append(*str_to); break;
        default: break;
      }
    }

    // apply functions
    if (matched)
      for (const auto& fcn : re.fmt_fcns) fcn.apply(h.h);
  } // end expression loop

  // substitute strings
  if (std::get<0>(share).size()>1)
    h.group = std::get<0>(share).back();
  if (std::get<1>(share).size()>1)
    h.h->SetTitle(std::get<1>(share).back()->c_str());
  if (std::get<2>(share).size()>1)
    h.h->GetXaxis()->SetTitle(std::get<2>(share).back()->c_str());
  if (std::get<3>(share).size()>1)
    h.h->GetYaxis()->SetTitle(std::get<3>(share).back()->c_str());
  if (std::get<4>(share).size()>1)
    h.h->GetZaxis()->SetTitle(std::get<4>(share).back()->c_str());
  if (std::get<5>(share).size()>1)
    h.legend = std::get<5>(share).back();
  if (std::get<6>(share).size()>1)
    h.h->SetName(std::get<6>(share).back()->c_str());

  return true;
}

hist_fmt_re::~hist_fmt_re() { if (re) delete re; }
hist_fmt_re::hist_fmt_re(const hist_fmt_re& o) noexcept
: flags(o.flags), re(o.re), subst(o.subst), fmt_fcns(o.fmt_fcns) { }
hist_fmt_re::hist_fmt_re(hist_fmt_re&& o) noexcept
: flags(o.flags), re(o.re),
  subst(std::move(o.subst)), fmt_fcns(std::move(o.fmt_fcns))
{
  o.re = nullptr;
}
hist_fmt_re& hist_fmt_re::operator=(const hist_fmt_re& o) noexcept {
  flags = o.flags;
  re = o.re;
  subst = o.subst;
  fmt_fcns = o.fmt_fcns;
  return *this;
}
hist_fmt_re& hist_fmt_re::operator=(hist_fmt_re&& o) noexcept {
  flags = o.flags;
  re = o.re; o.re = nullptr;
  subst = o.subst;
  fmt_fcns = std::move(o.fmt_fcns);
  return *this;
}

// ******************************************************************

struct hist_fmt_fcn_SetLineColor
: public hist_fmt_fcn_impl, private interpreted_args<Color_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineColor(std::get<0>(args)); }
};

struct hist_fmt_fcn_SetLineStyle
: public hist_fmt_fcn_impl, private interpreted_args<Style_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineStyle(std::get<0>(args)); }
};

struct hist_fmt_fcn_SetLineWidth
: public hist_fmt_fcn_impl, private interpreted_args<Width_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetLineWidth(std::get<0>(args)); }
};

struct hist_fmt_fcn_Scale
: public hist_fmt_fcn_impl, private interpreted_args<Double_t,std::string> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->Scale(
    std::get<0>(args), std::get<1>(args).c_str() ); }
};

struct hist_fmt_fcn_Normalize
: public hist_fmt_fcn_impl, private interpreted_args<double> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const {
    h->Scale(std::get<0>(args)/h->Integral("width")); }
};

struct hist_fmt_fcn_SetOption
: public hist_fmt_fcn_impl, private interpreted_args<std::string> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->SetOption(std::get<0>(args).c_str()); }
};

struct hist_fmt_fcn_Rebin
: public hist_fmt_fcn_impl, private interpreted_args<Int_t> {
  using interpreted_args::interpreted_args;
  virtual void apply(TH1* h) const { h->Rebin(std::get<0>(args)); }
};

// ******************************************************************

bool all_lower_strcmp(const boost::string_ref& s1, const char* s2) {
  for (unsigned i=0, n=s1.size(); i<n; ++i)
    if (std::tolower(s1[i])!=s2[i]) return false;
  return true;
}

// syntax: function=arg1,arg2,arg3
// spaces don't matter
hist_fmt_fcn::hist_fmt_fcn(const std::string& str) {
  if (str.size()==0) throw std::runtime_error(
    "blank string in hist_fmt_fcn function field");

  std::vector<boost::string_ref> tokens;
  const char *c = str.c_str();
  const char *begin = c;
  char d = '=';
  for (;;) {
    while (*c!=d && *c!='\0') ++c;
    if (*c!='\0' && d==',') {
      int esc = 0;
      for (const char *a=c-1; *a=='\\' && a!=begin; --a) ++esc;
      if (esc%2) { ++c; continue; }
    }
    std::pair<const char*,const char*> cc(begin,c-1);
    while (isspace(*cc.first )) ++cc.first;
    while (isspace(*cc.second)) --cc.second;
    tokens.emplace_back(cc.first,cc.second-cc.first+1);

    if (*c=='\0') break;
    else { begin = ++c; if (d=='=') d = ','; }
  }

  // find the right function
  try {
    if (all_lower_strcmp(tokens.front(),"linecolor")) {
      impl = new hist_fmt_fcn_SetLineColor(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"linestyle")) {
      impl = new hist_fmt_fcn_SetLineStyle(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"linewidth")) {
      impl = new hist_fmt_fcn_SetLineWidth(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"scale")) {
      impl = new hist_fmt_fcn_Scale(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"norm")) {
      impl = new hist_fmt_fcn_Normalize(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"draw")) {
      impl = new hist_fmt_fcn_SetOption(++tokens.begin(),tokens.end());
    } else if (all_lower_strcmp(tokens.front(),"rebin")) {
      impl = new hist_fmt_fcn_Rebin(++tokens.begin(),tokens.end());
    } else throw std::runtime_error("unknown function "+tokens.front().to_string());
  } catch (const std::exception& e) {
    throw std::runtime_error("in "+str+": "+e.what());
  }
}

std::string get_hist_file_str(TH1* h) {
  auto* dir = h->GetDirectory();
  while (!dir->InheritsFrom(TFile::Class())) dir = dir->GetMotherDir();
  return dir->GetName();
}
std::string get_hist_dirs_str(TH1* h) {
  std::string dirs;
  for ( auto* dir = h->GetDirectory(); !dir->InheritsFrom(TFile::Class());
        dir = dir->GetMotherDir() ) {
    if (dirs.size()) dirs += '/';
    dirs += dir->GetName();
  }
  return std::move(dirs);
}

hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  const std::string& group, const std::string& legend)
: h(h), group(std::make_shared<std::string>(group)),
  legend(std::make_shared<std::string>(legend))
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  std::string&& group, std::string&& legend)
: h(h), group(std::make_shared<std::string>(std::move(group))),
  legend(std::make_shared<std::string>(std::move(legend)))
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  const std::string& group, std::string&& legend)
: h(h), group(std::make_shared<std::string>(group)),
  legend(std::make_shared<std::string>(std::move(legend)))
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  std::string&& group, const std::string& legend)
: h(h), group(std::make_shared<std::string>(std::move(group))),
  legend(std::make_shared<std::string>(legend))
{ }

hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  const shared_str& group, const shared_str& legend)
: h(h), group(group), legend(legend)
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  shared_str&& group, shared_str&& legend)
: h(h), group(std::move(group)), legend(std::move(legend))
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  const shared_str& group, shared_str&& legend)
: h(h), group(group), legend(std::move(legend))
{ }
hist_fmt_re::hist_wrap::hist_wrap(TH1* h,
  shared_str&& group, const shared_str& legend)
: h(h), group(std::move(group)), legend(legend)
{ }
