#include <iostream>
#include <cmath>

#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>

#include "exception.hh"

using std::cout;
using std::cerr;
using std::endl;

#define test(var) \
    std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

template <typename T> [[ gnu::const ]]
inline T sq(T x) noexcept { return x*x; }
template <typename T, typename... TT> [[ gnu::const ]]
inline T sq(T x, TT... xx) noexcept { return sq(x)+sq(xx...); }

template <class T>
inline T* get(TDirectory* dir, const char* name) {
  T *obj = nullptr;
  dir->GetObject(name,obj);
  if (obj) return obj;
  else throw ivanp::exception("No object ",name," in ",dir->GetName());
}

void dir_loop(TDirectory* dir1, TDirectory* dir2) {
  auto *cur_dir = gDirectory;
  TIter next(dir1->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(next()))) {
    auto *obj = key->ReadObj();
    if (obj->InheritsFrom(TDirectory::Class())) {
      auto *dir1_down = static_cast<TDirectory*>(obj);
      auto *dir1_down_name = dir1_down->GetName();
      cur_dir->mkdir(dir1_down_name);
      cur_dir->cd(dir1_down_name);
      cout << "\033[1;49;34m" << dir1_down->ClassName()
           << "\033[0m: " << dir1_down_name << endl;
      dir_loop(dir1_down, get<TDirectory>(dir2,dir1_down_name));
      cur_dir->cd();
    } else if (obj->InheritsFrom(TH1::Class())) {
      auto *h1 = static_cast<TH1*>(obj);
      auto *h2 = get<TH1>(dir2,h1->GetName());

      auto *rat = static_cast<TH1*>(h1->Clone());
      rat->Divide(h2);

      cout << "\033[34m" << rat->ClassName()
           << "\033[0m: " << rat->GetName() << endl;
    }
  }
}

int main(int argc, char** argv) {
  if (argc!=4) {
    cout << "usage: " << argv[0]
         << " top.root bottom.root ratio.root" << endl;
    return 1;
  }

  auto fin1 = std::make_unique<TFile>(argv[1],"read");
  if (fin1->IsZombie()) return 1;
  auto fin2 = std::make_unique<TFile>(argv[2],"read");
  if (fin2->IsZombie()) return 1;
  auto fout = std::make_unique<TFile>(argv[3],"recreate");
  if (fout->IsZombie()) return 1;

  dir_loop(fin1.get(),fin2.get());

  fout->Write();

  return 0;
}
