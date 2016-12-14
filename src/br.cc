#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>

using std::cout;
using std::endl;

class comma_numpunct: public std::numpunct<char> {
protected:
  virtual char do_thousands_sep() const { return '\''; }
  virtual std::string do_grouping() const { return "\03"; }
};
std::locale comma_locale(std::locale(), new comma_numpunct());

void prt_branch(const char* type, const char* name, const char* title) {
  cout << "\033[35m" << type << "\033[0m " << name;
  if (title)
    if (std::strlen(title))
      if (std::strcmp(name,title))
        cout << ": \033[2;49;37m" << title << "\033[0m";
  cout << endl;
}

void prt_tree(TTree* tree) {
  std::stringstream ss;
  ss.imbue(comma_locale);
  ss << tree->GetEntries();
  cout << " [" << ss.rdbuf() << ']' << endl;

  TObjArray *_b = tree->GetListOfBranches();
  auto * const lb = _b->Last();
  for ( auto bo : *_b ) {
    TBranch *b = static_cast<TBranch*>(bo);

    const char * const bcname = b->GetClassName();
    const auto nl = b->GetNleaves();

    cout << (b != lb ? "├" : "└") << "── ";
    if (nl>1) prt_branch( bcname, b->GetName(), b->GetTitle() );

    TObjArray *_l = b->GetListOfLeaves();
    TLeaf * const ll = static_cast<TLeaf*>(_l->Last());

    if (nl==1) {
      const char * const ltname = ll->GetTypeName();
      prt_branch( (ltname ? ltname : bcname), ll->GetName(), ll->GetTitle() );
    } else for ( auto lo : *_l ) {
      TLeaf *l = static_cast<TLeaf*>(lo);
      cout << "│   " << (l != ll ? "├" : "└") << "── ";
      prt_branch( l->GetTypeName(), l->GetName(), l->GetTitle() );
    } // end leaf loop
  } // end branch loop
}

void prt_key(TKey* key, const char* color) {
  cout << color << key->GetClassName() << "\033[0m " << key->GetName();
}

void read_dir(TDirectory* dir) {
  TIter next(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(next()))) {
    TClass *key_class = TClass::GetClass(key->GetClassName());

    if (key_class->InheritsFrom(TTree::Class())) { // Found a tree
      prt_key(key,"\033[1;49;92m");
      prt_tree(dynamic_cast<TTree*>(key->ReadObj()));
    } else if (key_class->InheritsFrom(TDirectory::Class())) {
      prt_key(key,"\033[1;49;34m");
      cout << endl;
      read_dir(dynamic_cast<TDirectory*>(key->ReadObj()));
    } else prt_key(key,"\033[34m");
    cout << endl;
  }
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    cout << "usage: " << argv[0] << " file.root" << endl;
    return 0;
  }

  TFile* file = new TFile(argv[1]);
  if (file->IsZombie()) return 1;

  read_dir(file);

  delete file;
  return 0;
}
