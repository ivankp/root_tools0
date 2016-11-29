#include <iostream>
#include <cstring>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>

using std::cout;
using std::endl;

void prt(const char* type, const char* name, const char* title) {
  cout << "\033[35m" << type << "\033[0m "
       << name;
  if (title)
    if (std::strlen(title))
      if (std::strcmp(name,title))
        cout << ": \033[30m" << title << "\033[0m";
  cout << endl;
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    cout << "usage: " << argv[0] << " file.root" << endl;
    return 0;
  }

  TFile* file = new TFile(argv[1]);
  if (file->IsZombie()) return 1;

  TIter next(file->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(next()))) {
    const char * const key_class_name = key->GetClassName();
    TClass *key_class = TClass::GetClass(key_class_name);
    cout << "\033[34m"  << key_class_name << "\033[0m ";
    cout << key->GetName() << endl;

    if (key_class->InheritsFrom("TTree")) { // Found a tree

      TTree *tree = dynamic_cast<TTree*>(key->ReadObj());

      TObjArray *_b = tree->GetListOfBranches();
      auto * const lb = _b->Last();
      for ( auto bo : *_b ) {
        TBranch *b = static_cast<TBranch*>(bo);

        const char * const bcname = b->GetClassName();
        const auto nl = b->GetNleaves();

        cout << (b != lb ? "├" : "└") << "── ";
        if (nl>1) prt( bcname, b->GetName(), b->GetTitle() );

        TObjArray *_l = b->GetListOfLeaves();
        TLeaf * const ll = static_cast<TLeaf*>(_l->Last());

        if (nl==1) {
          const char * const ltname = ll->GetTypeName();
          prt( (ltname ? ltname : bcname), ll->GetName(), ll->GetTitle() );
        } else for ( auto lo : *_l ) {
          TLeaf *l = static_cast<TLeaf*>(lo);
          cout << "│   " << (l != ll ? "├" : "└") << "── ";
          prt( l->GetTypeName(), l->GetName(), l->GetTitle() );
        } // end leaf loop
      } // end branch loop

      cout << endl;
    } // end if tree
  }

  delete file;
  return 0;
}
