#include <iostream>
#include <sstream>

// #include "hist_fmt_re.hh"
//
// #include <TH1.h>
// #include <TAxis.h>
#include <boost/program_options.hpp>

#include "array_istream_op.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  // vector<hist_fmt_re> re; re.reserve(argc-1);
  // for (int i=1; i<argc; ++i) {
  //   re.emplace_back(argv[i]);
  // }
  //
  // TH1 *h = new TH1D("hist_test","Histogram example",10,0,10);
  // hist_fmt_re::hist_wrap hw {h,"group","leg"};
  // apply(re,hw);
  //
  // test(*hw.group)
  // test(h->GetTitle());
  // test(h->GetXaxis()->GetTitle());
  // test(h->GetYaxis()->GetTitle());
  // test(*hw.legend)
  // test(h->GetName());
  // test(h->GetLineColor());
  //
  // delete h;

  std::array<double,3> arr;

  try {
    po::options_description desc("Options");
    desc.add_options()
      ("arr,a", po::value(&arr), "array test");
    po::positional_options_description pos;
    pos.add("arr",-1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
      .options(desc).positional(pos).run(), vm);
    po::notify(vm);
  } catch (exception& e) {
    cerr <<"\033[31mError in overlay options: "<<e.what()<<"\033[0m"<< endl;
    return 1;
  }

  // stringstream(argv[1]) >> arr;
  for (auto x : arr) test(x)

  return 0;
}
