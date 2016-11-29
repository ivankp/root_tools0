#include <iostream>
#include <memory>
#include <cstring>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>

using std::cout;
using std::endl;

int main(int argc, char** argv)
{
  if (argc != 2) {
    cout << "usage: " << argv[0] << " file.root" << endl;
    return 0;
  }

  auto file = std::make_unique<TFile>(argv[1]);
  if (file->IsZombie()) return 1;

  TIter next(file->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(next()))) {
    TClass *key_class = TClass::GetClass(key->GetClassName());
    if (key_class->InheritsFrom("TTree")) { // Found a tree

      TTree *tree = dynamic_cast<TTree*>(key->ReadObj());
      cout << "\033[34m"  << tree->ClassName()
           << "\033[0m " << tree->GetName() << endl;

      TObjArray *lb = tree->GetListOfBranches();
      auto * const lb_last = lb->Last();
      for ( auto bo : *lb ) {
        TBranch *b = static_cast<TBranch*>(bo);

        const auto nl = b->GetNleaves();

        cout << (b == lb_last ? "└" : "├");
        if (nl!=1) {
          cout << "── ";
          if (std::strlen(b->GetClassName())>1)
            cout << "\033[35m" << b->GetClassName() << "\033[0m ";
          cout << "\033[32m" << b->GetName()
               << "\033[0m: " << b->GetTitle();
        }

        TObjArray *ll = b->GetListOfLeaves();
        auto * const ll_last = ll->Last();
        for ( auto lo : *ll ) {
          TLeaf *l = static_cast<TLeaf*>(lo);
          if (nl!=1) cout << "    " << (l == ll_last ? "└" : "├");
          cout << "── \033[35m" << l->GetTypeName()
               << "\033[0m \033[32m" << l->GetName()
               << "\033[0m: " << l->GetTitle()
               << endl;
        } // end leaf loop
      } // end branch loop

    } // end if tree
    cout << endl;
  }

  return 0;
}
