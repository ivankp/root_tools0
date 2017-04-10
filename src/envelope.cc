// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

#include <TClass.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>

#include <exception.hh>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;

template <typename F, typename T, typename... TT>
inline void apply(F f, const T& x, const TT&... xx) {
  f(x);
  apply(f,xx...);
}
template <typename F, typename T>
inline void apply(F f, const T& x) { f(x); }

template <typename T>
inline T* read_key(TKey* key) { return static_cast<T*>(key->ReadObj()); }

template <typename T>
inline T* get(TDirectory* dir, const char* name) {
  auto *obj = dir->Get(name);
  if (!obj) throw ivanp::exception("cannot get object ",name);
  return dynamic_cast<T*>(obj);
}

TDirectory* enter(TDirectory* dir, const char* name) {
  TDirectory *child = dir->GetDirectory(name);
  if (!child) throw ivanp::exception(
    "directory ",name," does not exist inside ",dir->GetName());
  return child;
}

TClass* get_class(const char* name) {
  TClass* class_ptr = TClass::GetClass(name);
  if (!class_ptr) throw ivanp::exception("cannot find class ",name);
  return class_ptr;
}

template <typename T>
void cd_write(TDirectory* dir, T* obj) {
  dir->cd();
  obj->Clone();
}

template <typename... D>
void copy_dirs(TDirectory* dir, D*... dest) {
  TIter nextkey(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(nextkey()))) {
    const auto class_name = key->GetClassName();
    const auto class_ptr  = get_class(class_name);

    if (class_ptr->InheritsFrom(TDirectory::Class())) {
      TDirectory * const d = read_key<TDirectory>(key);
      const char* name = d->GetName();
      copy_dirs(d,dest->mkdir(name)...);
    }
  }
}

template <typename... D>
void copy_hists(TDirectory* dir, D*... dest) {
  TIter nextkey(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(nextkey()))) {
    const auto class_name = key->GetClassName();
    const auto class_ptr  = get_class(class_name);

    if (class_ptr->InheritsFrom(TH1::Class())) {
      TH1 *h = read_key<TH1>(key);
      h->SetName(key->GetName());
      apply([h](TDirectory* d){ cd_write(d,h); }, dest...);
    } else if (class_ptr->InheritsFrom(TDirectory::Class())) {
      TDirectory * const d = read_key<TDirectory>(key);
      const char* name = d->GetName();
      copy_hists(d,enter(dest,name)...);
    }
  }
}

void build_envelope(TDirectory* dir, TDirectory* lower, TDirectory* upper) {
  TIter nextkey(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(nextkey()))) {
    const auto class_name = key->GetClassName();
    const auto class_ptr  = get_class(class_name);

    if (class_ptr->InheritsFrom(TH1::Class())) {
      TH1 *h = read_key<TH1>(key);
      const char* key_name = key->GetName();
      const int nbins = h->GetNbinsX();
      TH1 *hl = get<TH1>(lower,key_name);
      TH1 *hu = get<TH1>(upper,key_name);
      if (nbins != hl->GetNbinsX()) throw ivanp::exception(
        "nbins don\'t match");
      // auto* hw2  = h ->GetSumw2();
      // auto* hlw2 = hl->GetSumw2();
      // auto* huw2 = hu->GetSumw2();
      // const bool has_sumw2 = ( hw2 && hlw2 );
      // test( (hw2->GetSize() == nbins) )

      for (int i=nbins+2; i; ) { --i;
        const auto x = h ->GetBinContent(i);
        const auto l = hl->GetBinContent(i);
        const auto u = hu->GetBinContent(i);
        if (x < l) {
          hl->SetBinContent(i,x);
          // if (has_sumw2) hlw2[i] = hw2[i];
        } else if (x > u) {
          hu->SetBinContent(i,x);
          // if (has_sumw2) huw2[i] = hw2[i];
        }
      }

    } else if (class_ptr->InheritsFrom(TDirectory::Class())) {
      TDirectory * const d = read_key<TDirectory>(key);
      const char* name = d->GetName();
      build_envelope(d,enter(lower,name),enter(upper,name));
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc<4) {
    cout << "usage: " << argv[0] <<
            " out.root central.root [other.root ...]" << endl;
    cout << "   or: " << argv[0] <<
            " out.root all.root central [directories ...]" << endl;
    return 1;
  }

  bool single_file = false;
  {
    std::vector<bool> root_exts(argc-1);
    for (int i=1; i<argc; ++i) {
      const auto n = strlen(argv[i]);
      if (n <= 5) return false;
      root_exts[i-1] = !strcmp(argv[i]+n-5,".root");
    };
    if (!root_exts[0]) {
      cerr << "\033[31mOutput file must have .root extension\033[0m" << endl;
      return 1;
    }
    if (!root_exts[1]) {
      cerr << "\033[31mInput file must have .root extension\033[0m" << endl;
      return 1;
    }
    single_file = !root_exts[2];
    for (int i=2; i<argc-1; ++i) {
      if (root_exts[i] == single_file) {
        cerr << "\033[31mAll or none of the subsequent arguments "
                "must be root files\033[0m" << endl;
        return 1;
      }
    }
  }

  const auto fout = std::make_unique<TFile>(argv[1],"recreate");
  cout << "\033[36mOutput\033[0m  : " << fout->GetName() << endl;
  if (fout->IsZombie()) return 1;
  TDirectory *central = fout->mkdir("central");
  TDirectory *lower   = fout->mkdir("lower");
  TDirectory *upper   = fout->mkdir("upper");

  if (single_file) {
    // ==============================================================

    const auto fin = std::make_unique<TFile>(argv[2]);
    cout << "\033[36mInput\033[0m   : " << fin->GetName() << endl;
    if (fin->IsZombie()) return 1;

    TDirectory* dir = enter(fin.get(),argv[3]);
    cout << "\033[36mCentral\033[0m : " << dir->GetName() << endl;
    copy_dirs(dir,central,lower,upper);
    copy_hists(dir,central);

    dir = enter(fin.get(),argv[4]);
    cout << "\033[36mEnvelope\033[0m: " << dir->GetName() << endl;
    copy_hists(dir,lower,upper);

    for (int i=5; i<argc; ++i) {
      dir = enter(fin.get(),argv[i]);
      cout << "\033[36mEnvelope\033[0m: " << dir->GetName() << endl;
      build_envelope(dir,lower,upper);
    }

    // ==============================================================
  } else { // multiple files
    // ==============================================================

    { const auto fin1 = std::make_unique<TFile>(argv[2]);
      cout << "\033[36mCentral\033[0m : " << fin1->GetName() << endl;
      if (fin1->IsZombie()) return 1;

      copy_dirs(fin1.get(),central,lower,upper);
      copy_hists(fin1.get(),central);
    }
    { const auto fin2 = std::make_unique<TFile>(argv[3]);
      cout << "\033[36mEnvelope\033[0m: " << fin2->GetName() << endl;
      if (fin2->IsZombie()) return 1;

      copy_hists(fin2.get(),lower,upper);
    }
    for (int i=4; i<argc; ++i) {
      const auto fin = std::make_unique<TFile>(argv[i]);
      cout << "\033[36mEnvelope\033[0m: " << fin->GetName() << endl;
      if (fin->IsZombie()) return 1;

      build_envelope(fin.get(),lower,upper);
    }

    // ==============================================================
  }

  fout->Write();

  return 0;
}
