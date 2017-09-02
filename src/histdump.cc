#include <iostream>
#include <iomanip>

#include <TClass.h>
#include <TFile.h>
#include <TH1.h>

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char* argv[]) {
  if (argc!=3) {
    cout << "usage: " << argv[0] << " in.root hist" << endl;
    return 1;
  }

  TFile f(argv[1]);
  if (f.IsZombie()) return 1;

  auto *ptr = f.Get(argv[2]);
  if (!ptr) {
    cerr << "Could not get " << argv[2] << " from file " << argv[1] << endl;
    return 1;
  }
  if (!ptr->InheritsFrom(TH1::Class())) {
    cerr << ptr->ClassName() <<' '<< ptr->GetName()
         << " does not inherit from TH1" << endl;
    return 1;
  }

  cout << std::fixed << std::setprecision(8) << std::scientific;

  TH1 *h = static_cast<TH1*>(ptr);
  const int nbins = h->GetNbinsX();
  cout << h->GetName() << endl;
  cout << "lower_bin_edge "
          "content        "
          "error" << endl;
  cout << "underflow      "
       << h->GetBinContent(0) << ' '
       << h->GetBinError(0) << endl;
  for (int i=1; i<=nbins+1; ++i)
    cout << h->GetBinLowEdge(i) << ' '
         << h->GetBinContent(i) << ' '
         << h->GetBinError(i) << endl;
}
