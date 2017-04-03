// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
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
  const auto f = std::make_unique<TFile>(argv[1]);
  if (f->IsZombie()) return 1;

  TH1 *h = dynamic_cast<TH1*>(f->Get(argv[2]));
  TAxis *xa = h->GetXaxis();
  if (xa->IsVariableBinSize()) {
    const TArrayD& bins = *xa->GetXbins();
    for (Int_t i=0, n=bins.GetSize(); i<n; ++i)
      cout << bins[i] << '\n';
    cout.flush();
  } else {
    cout << xa->GetNbins() << ": "
         << xa->GetXmin() << ' ' << xa->GetXmax() << endl;
  }

  return 0;
}
