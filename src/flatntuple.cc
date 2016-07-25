// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

#include <TTree.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include "timed_counter.hh"
#include "flatntuple_options.hh"
#include "flatntuple_config.hh"

using namespace std;

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

int main(int argc, const char* argv[])
{
  flatntuple_options opt;
  flatntuple_config config;
  try {
    if (opt.parse(argc, argv)) return 1;
    config.parse(opt.config);
  } catch (exception& e) {
    cerr <<"\033[31mError in "<<argv[0]<<": "<<e.what()<<"\033[0m"<< endl;
    return 1;
  }

  test(config.itree)
  test(config.otree)
  cout << endl;
  for (const auto& b : config.branches) {
    test((char)b[0].type)
    test(b[0].name)
    test((char)b[1].type)
    test(b[1].name)
    cout << endl;
  }

  // TChain ch("T");
  // for (const auto& f : opt.input) ch.Add(f.c_str());
  //
  // TFile* fout = new TFile(opt.output.c_str(),"recreate");
  // if (fout->IsZombie()) return 1;
  // cout << "Output file: " << fout->GetName() << endl;
  // TTree* tout = new TTree(argv[4],"");
  //
  // TTreeReader reader(&ch);

  // using tc = timed_counter<Long64_t>;
  // for (tc ent(reader.GetEntries(true)); reader.Next(); ++ent) {
  //   for (auto& p : pipes) p.Get();
  //   tout->Fill();
  // }
  //
  // fout->Write(0,TObject::kOverwrite);
  // fout->Close();
  // delete fout;
  //
  // fin->Close();
  // delete fin;

  return 0;
}
