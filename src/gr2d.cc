#include <iostream>
#include <fstream>
#include <ctime>

#include <TCanvas.h>
#include <TGraph2D.h>

#include "catstr.hh"

using ivanp::cat;

int main(int argc, char* argv[]) {

  std::istream *is;
  if (argc==2) {
    is = &std::cin;
  } else if (argc==3) {
    is = new std::ifstream(argv[2]);
  }

  TCanvas canv(cat("canvas_",std::time(0)).c_str(),"");
  TGraph2D gr;

  for (double x, y, z; (*is) >> x >> y >> z;)
    gr.SetPoint(gr.GetN(),x,y,z);

  gr.Draw("surf2");
  canv.SaveAs(argv[1]);

  if (argc==3) delete is;
}
