#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unordered_set>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TH1.h>

#include "catstr.hh"

using std::cout;
using std::endl;
using ivanp::cat;

class comma_numpunct: public std::numpunct<char> {
protected:
  virtual char do_thousands_sep() const { return '\''; }
  virtual std::string do_grouping() const { return "\03"; }
};
std::locale comma_locale(std::locale(), new comma_numpunct());

auto file_size(const char* name) {
  std::ifstream in(name, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}
std::string file_size_str(const char* name) {
  double size = file_size(name);
  unsigned i = 0;
  for ( ; size > 1024; ++i) size /= 1024;
  return cat(std::setprecision(2),std::fixed,size,' '," kMGT"[i],'B');
}

std::vector<bool> last;
inline void operator++(decltype(last)& v) noexcept { v.push_back(false); }
inline void operator--(decltype(last)& v) noexcept { return v.pop_back(); }

bool indent() {
  const auto n = last.size();
  if (n) {
    for (size_t i=0; i<n-1; ++i)
      cout << (last[i] ? " " : "│") << "   ";
  }
  return n;
}
void indent(bool is_last) {
  if (indent()) {
    cout << ((last.back() = is_last) ? "└" : "├") << "── ";
  }
}

void prt_key(TKey* key, const char* color) {
  cout << color << key->GetClassName() << "\033[0m "
       << key->GetName();
  const auto cycle = key->GetCycle();
  if (cycle!=1)
    cout << "\033[2;49;37m;" << key->GetCycle() << "\033[0m";
}

void prt_branch(const char* type, const char* name, const char* title) {
  if (type && type[0])
    cout << "\033[35m" << type << "\033[0m ";
  cout << name;
  if (title)
    if (std::strcmp(name,title))
      cout << ": \033[2;49;37m" << title << "\033[0m";
  cout << endl;
}

void prt_tree(TTree* tree) {
  std::stringstream ss;
  ss.imbue(comma_locale);
  ss << tree->GetEntries();
  cout << " [" << ss.rdbuf() << ']' << endl;
  
  std::unordered_set<std::string> branch_names;

  ++last;
  TObjArray *_b = tree->GetListOfBranches();
  auto * const lb = _b->Last();
  for ( auto bo : *_b ) {
    TBranch *b = static_cast<TBranch*>(bo);

    const char * const bcname = b->GetClassName();
    const char * const bname = b->GetName();
    const auto nl = b->GetNleaves();

    const bool dup = !branch_names.emplace(bname).second;

    TObjArray *_l = b->GetListOfLeaves();
    TLeaf * const ll = static_cast<TLeaf*>(_l->Last());

    indent(b==lb);
    if (nl==1 && !strcmp(bname,ll->GetName())) {
      std::string lname(ll->GetName());
      if (dup) lname = "\033[31m" + lname + "\033[0m";
      prt_branch( ll->GetTypeName(), lname.c_str(), ll->GetTitle() );
    } else {
      std::string lname(ll->GetName());
      if (dup) lname = "\033[31m" + lname + "\033[0m";
      prt_branch( bcname, bname, b->GetTitle() );

      ++last;
      for ( auto lo : *_l ) {
        TLeaf *l = static_cast<TLeaf*>(lo);
        indent(l==ll);
        prt_branch( l->GetTypeName(), l->GetName(), l->GetTitle() );
      } // end leaf loop
      --last;
    }

  } // end branch loop
  --last;
}

bool integrals = false;

bool read_dir(TDirectory* dir, bool first=true) {
  bool skip = false;
  TIter next(dir->GetListOfKeys());
  TKey *key, *next_key = static_cast<TKey*>(next());
  if (!first) ++last;
  while ((key = next_key)) {
    TClass *key_class = TClass::GetClass(key->GetClassName());
    indent(!(next_key = static_cast<TKey*>(next())));

    if (!key_class) {
      prt_key(key,"\033[33m");
      cout << endl;
    } else if (key_class->InheritsFrom(TTree::Class())) { // Found a tree
      prt_key(key,"\033[1;49;92m");
      prt_tree(dynamic_cast<TTree*>(key->ReadObj()));
      indent();
      skip = true;
      cout << endl;
    } else if (key_class->InheritsFrom(TDirectory::Class())) {
      prt_key(key,"\033[1;49;34m");
      cout << endl;
      const auto dir = dynamic_cast<TDirectory*>(key->ReadObj());
      skip = read_dir(dir, false);
      if (!skip && dir->GetListOfKeys()->GetSize()>0 && next_key) {
        indent();
        cout << (first ? "" : "│") << endl;
        skip = true;
      }
    } else if (integrals && key_class->InheritsFrom(TH1::Class())) {
      prt_key(key,"\033[34m");
      if (integrals) {
        TH1 *h = static_cast<TH1*>(key->ReadObj());
        cout << ' ' << h->Integral(0,-1);
      }
      cout << endl;
    } else {
      prt_key(key,"\033[34m");
      cout << endl;
      const char* title = key->GetTitle();
      if (title && title[0]!='\0') cout << '\t' << title << endl;
    }
  }
  if (!first) --last;
  return skip;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "usage: " << argv[0] << " file.root" << endl;
    return 0;
  }
  for (int i=2; i<argc; ++i) {
    if (!strcmp(argv[i],"--integrals"))
      integrals = true;
  }

  cout << "File size: " << file_size_str(argv[1]) <<'\n'<< endl;

  TFile file(argv[1]);
  if (file.IsZombie()) return 1;

  read_dir(&file);

  return 0;
}
