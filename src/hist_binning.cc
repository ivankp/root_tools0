// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <TFile.h>
#include <TAxis.h>
#include <TH1.h>

#include <exception.hh>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char* argv[]) {
  if (argc!=3) {
    cout << "usage: " << argv[0] << " file.root dir/hist" << endl;
    return 1;
  }

  TFile f(argv[1]);
  if (f.IsZombie()) return 1;

  TH1 *h = dynamic_cast<TH1*>(f.Get(argv[2]));
  TAxis *xa = h->GetXaxis();
  if (xa->IsVariableBinSize()) {
    const TArrayD& bins = *xa->GetXbins();
    const auto n = bins.GetSize();
    const double diff = bins[1] - bins[0];
    bool uniform = true;
    for (Int_t i=1; i<n; ++i) {
      if (std::abs(1.-(bins[i]-bins[i-1])/diff) > 1e-8) {
        uniform = false;
        break;
      }
    }
    if (uniform)
      cout << (n-1) << ": " << bins[0] << ' ' << bins[n-1] << '\n';
    else
      for (Int_t i=0; i<n; ++i)
        cout << bins[i] << '\n';
    cout.flush();
  } else {
    cout << xa->GetNbins() << ": "
         << xa->GetXmin() << ' ' << xa->GetXmax() << endl;
  }

  return 0;
}
