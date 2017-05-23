#include <iostream>
#include <vector>
#include <memory>

#include <TApplication.h>
#include <TBrowser.h>
#include <TFile.h>
#include <TStyle.h>

int main(int argc, char* argv[])
{
  int app_argc = 1;
  char app_argv1[] = "l";
  char *app_argv[] = { app_argv1 };
  TApplication app("App",&app_argc,app_argv);

  gStyle->SetOptStat(110010);
  gStyle->SetPaintTextFormat(".2f");

  std::vector<std::unique_ptr<TFile>> files;
  files.reserve(argc-1);
  // std::cout << std::endl;
  for (int i=1; i<argc; ++i) {
    files.emplace_back(new TFile(argv[i]));
    // std::cout << files.back()->GetName();
  }
  // std::cout << std::endl;

  TBrowser b;

  app.Run();
  return 0;
}
