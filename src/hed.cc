#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <TClass.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>

#include "catstr.hh"
#include "type.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using ivanp::cat;

template <typename Str, unsigned N>                                             
bool substrcmp(const Str& str, unsigned n, const char(&prefix)[N]) {                      
  if (n!=N-1) return false;
  for (unsigned i=0; i<N-1; ++i)                                                
    if (str[i]=='\0' || str[i]!=prefix[i]) return false;                        
  return true;                                                                  
}

template <typename T>
inline T lexical_cast(const char* str, unsigned n) {
  try {
    return boost::lexical_cast<T>(str,n);
  } catch (const boost::bad_lexical_cast &e) {
    throw std::runtime_error(cat(
      '\"', std::string(str,n), "\" cannot be cast to ", type_str<T>() ));
  }
}

template <typename T>
std::string full_path(T* x) {
  std::string path;
  for ( auto* dir = x->GetDirectory(); !dir->InheritsFrom(TFile::Class());
        dir = dir->GetMotherDir() ) {
    if (path.size()) path += '/';
    path += dir->GetName();
  }
  return path;
}

// histogram functions ==============================================
void scale_fcn(const char* arg, const std::vector<unsigned>& ds,
  std::vector<std::function<void(TH1*)>>& fs
) {
  const auto n = ds.size();
  if (n<2 || 3<n) throw std::runtime_error(
    "function syntax: scale,double,string=\"\"");
  fs.emplace_back(
    [ c1 = lexical_cast<double>(arg+ds[0]+1,ds[1]-ds[0]-1),
      opt = (n==2 ? "" : arg+ds[1]+1)
    ](TH1* h){
      h->Scale(c1,opt);
    }
  );
}
// ==================================================================

struct {
  struct expr {
    boost::regex re;
    std::string sub;
    std::vector< std::function<void(TH1*)> > fs;
    bool select;

    template <typename Reg, typename Sub>
    expr(Reg&& re, Sub&& sub, bool s)
    : re(std::forward<Reg>(re)), sub(std::forward<Sub>(sub)), fs(),
      select(s) { }
  };
  std::vector<std::unique_ptr<expr>> es;

  void operator()(TH1* h) {
    std::string name1(full_path(h));
    if (name1.size()) name1 += '/';
    name1 += h->GetName();
    std::string name(name1);
    boost::smatch matches;
    for (const auto& e : es) {
      if (boost::regex_match(name, matches, e->re)) {
        if (e->sub.size()) {
          auto name2 = boost::regex_replace(name, e->re, e->sub,
            boost::match_default | boost::format_sed);
          if (!name2.size()) throw std::runtime_error(cat(
            "histogram name \"",name2,"\" is blank after substitution"));
          name = std::move(name2);
        }
        for (auto& f : e->fs) f(h);
      } else if (e->select) return;
    }
    cout << name1 << " => " << name << endl;
    TDirectory *cur = gDirectory, *dir = cur;
    for (unsigned i=0; i<name.size(); ++i) {
      if (name[i]!='/') continue;
      const auto dir_name = name.substr(0,i);
      dir = cur->GetDirectory(dir_name.c_str());
      if (!dir) dir = cur->mkdir(dir_name.c_str());
      name = name.substr(i+1);
      i = 0;
    }
    if (!name.size()) throw std::runtime_error(cat(
      "histogram name \"",name,"\" is only directories"));

    h->SetDirectory(dir);
    h->SetName(name.c_str());
  }

  bool add_expr(const char* arg) {
    char c = arg[0];
    const bool s = (c=='s');
    if (s) {
      ++arg;
      c = arg[0];
    }
    if (!(c=='/' || c=='|' || c==':')) return false;
    const char d = c;
    std::string re;
    const char* a = arg;
    while ((c = *++a)!='\0') {
      if (c==d) { ++a; break; }
    }
    re.assign(arg+1, c==d ? a-1 : a);
    if (re.size()==0) throw std::runtime_error("blank regex");
    es.emplace_back(new expr(std::move(re),a,s));
    return true;
  }

  void add_fcn(const char* arg) {
    if (!es.size()) throw std::runtime_error(cat(
      "at least one expression must be given before function argument"));
    std::vector<unsigned> ds;
    unsigned i = 0;
    for (; arg[i]!='\0'; ++i)
      if (arg[i]==',') ds.push_back(i);
    ds.push_back(i);

#define ADDFCN(NAME) \
    if (substrcmp(arg,ds.front(),#NAME)) { \
      NAME##_fcn(arg,ds,es.back()->fs); return; }

    ADDFCN(scale)
#undef ADDFCN
    throw std::runtime_error(cat("unrecognized function argument: ",arg));
  }
} action;

void dir_loop(TDirectory* din) {
  for (auto* _key : *din->GetListOfKeys()) {
    TKey *key = static_cast<TKey*>(_key);
    TClass *key_class = TClass::GetClass(key->GetClassName());
    if (key_class->InheritsFrom(TDirectory::Class())) {
      dir_loop(static_cast<TDirectory*>(key->ReadObj()));
    } else if (key_class->InheritsFrom(TH1::Class())) {
      action(static_cast<TH1*>(key->ReadObj()));
    }
  }
}

int main(int argc, char* argv[]) {
  try {
    for (int a=3; a<argc; ++a) {
      if (action.add_expr(argv[a])) continue;
      action.add_fcn(argv[a]);
    }
  } catch (const std::exception& e) {
    cerr << "\033[31m" << e.what() << "\033[0m" << endl;
    return 1;
  }

  TFile fin(argv[1],"read");
  if (fin.IsZombie()) return 1;
  TFile fout(argv[2],"recreate");
  if (fout.IsZombie()) return 1;

  dir_loop(&fin);

  fout.Write();
}
