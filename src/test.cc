#include <iostream>

#include <sstream>
// #include "block_split.hh"

// #include <vector>
// #include <string>
// #include "interpreted_args.hh"
// #include "substr.hh"

#include "hist_fmt_re.hh"

#include <TH1.h>
#include <TAxis.h>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;

int main(int argc, char *argv[])
{
  // cout << endl;
  // for (int i=1; i<argc; ++i) {
  //   cout << "str "<<i<<": " << argv[i] << endl;
  //   int j=0;
  //   for (const auto& s : block_split(argv[i]))
  //     cout << (j++) << ": " << s << endl;
  //   cout << endl;
  // }

  // vector<string> v1 { "55", "3.14", "hello" };
  // vector<substr> v2 { {v1[0],1}, {v1[1].c_str()+2,2}, {v1[2].c_str()+1,4} };
  //
  // interpreted_args<int,double,string> args1(v1.begin(),v1.end());
  // test(get<0>(args1.args))
  // test(get<1>(args1.args))
  // test(get<2>(args1.args))
  //
  // interpreted_args<int,double,string> args2(v2.begin(),v2.end());
  // test(get<0>(args2.args))
  // test(get<1>(args2.args))
  // test(get<2>(args2.args))

  vector<hist_fmt_re> re; re.reserve(argc-1);
  for (int i=1; i<argc; ++i) {
    stringstream ss(argv[i]);
    re.emplace_back();
    ss >> re.back();
  }

  TH1 *h = new TH1D("hist_test","Histogram example",10,0,10);
  hist_fmt_re::hist_wrap hw {h,"group","leg"};
  apply(hw,re);

  test(hw.group)
  test(h->GetTitle());
  test(h->GetXaxis()->GetTitle());
  test(h->GetYaxis()->GetTitle());
  test(hw.legend)
  test(h->GetName());
  test(h->GetLineColor());

  delete h;
}
