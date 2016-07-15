// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <tuple>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLine.h>
#include <TStyle.h>
#include <TPaveStats.h>

#include "ring.hh"
#include "hist_fmt_re.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;
namespace po = boost::program_options;

// using shared_str = std::shared_ptr<std::string>;
using hist_group_map = std::map<std::string,std::vector<TH1*>>;

namespace std {
  template <typename T1, typename T2>
  istream& operator>>(istream& in, pair<T1,T2>& p) {
    stringstream s1, s2;
    in.get(*s1.rdbuf(),':');
    s1 >> p.first;
    in.ignore(); // skip delim char
    in.get(*s2.rdbuf(),(char)EOF);
    s2 >> p.second;
    return in;
  }
}

struct file {
  TFile *f;
  ~file() { delete f; }
  friend std::istream& operator>>(std::istream& in, file& f) {
    string buff;
    getline(in,buff,(char)EOF);
    f.f = new TFile(buff.c_str(),"read");
    if (f.f->IsZombie()) throw runtime_error("unable to open file "+buff);
    cout << "Input file: " << f.f->GetName() << endl;
    return in;
  }
};

void get_hists(TDirectory* dir,
  const vector<hist_fmt_re>& re, hist_group_map& hmap
) noexcept {
  TIter nextkey(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(nextkey()))) {
    TObject *obj = key->ReadObj();
    if (obj->InheritsFrom(TH1::Class())) {

      TH1* h = static_cast<TH1*>(obj);
      // TODO: default group and legend strings
      hist_fmt_re::hist_wrap hist {h,"",""};
      if ( apply(re,hist) )
        hmap[hist.group].emplace_back(h);

    } else if (obj->InheritsFrom(TDirectory::Class())) {
      get_hists( static_cast<TDirectory*>(obj), re, hmap );
    }
    // TODO: allow TGraphs to be overlayed as well
  }
}

int main(int argc, char **argv)
{
  string ofname, cfname;
  vector<file> ifiles;
  vector<hist_fmt_re> hist_re;
  ring<Color_t> color;
  ring<Style_t> style;
  ring<Width_t> width;
  pair<double,double> yrange(0.,0.);
  int stats;
  bool legend,
       logx, morelogx, noexpx,
       logy, morelogy, noexpy;
  vector<double> hliney, vlinex;

  // TODO: allow options to be applied only to some groups
  // TODO: text boxes
  // TODO: lines' and textboxes' styles

  // options ---------------------------------------------------
  try {
    po::options_description desc("Options");
    desc.add_options()
      ("input,i", po::value(&ifiles)->multitoken()->required(),
       "*input root file names")
      ("output,o", po::value(&ofname),
       "output pdf file name")
      ("conf,c", po::value(&cfname),
       "configuration file name")

      ("regex,r", po::value(&hist_re)->multitoken(),
       "regex for organizing histograms")

      ("legend,l", po::bool_switch(&legend), "draw legend")
      ("stats", po::value(&stats)->default_value(0),
       "draw stats box: e.g. 111110")

      ("colors", po::value(color.v_ptr())->multitoken()->
        default_value({602,46,8}, "{602,46,8}"),
       "histograms colors")
      ("styles", po::value(style.v_ptr())->multitoken()->
        default_value({1}, "{1}"),
       "histograms line styles")
      ("widths", po::value(width.v_ptr())->multitoken()->
        default_value({2}, "{2}"),
       "histograms line widths")

      ("yrange,y", po::value(&yrange),
       "vertical axis range")

      ("logx", po::bool_switch(&logx), "logarithmic horizontal axis")
      ("logy", po::bool_switch(&logy), "logarithmic vertical axis")
      ("mlogx", po::bool_switch(&morelogx), "more X logarithmic labels")
      ("mlogy", po::bool_switch(&morelogy), "more Y logarithmic labels")
      ("noexpx", po::bool_switch(&noexpx), "")
      ("noexpy", po::bool_switch(&noexpy), "")

      ("hline", po::value(&hliney), "horizontal lines coordinates")
      ("vline", po::value(&vlinex), "vertical lines coordinates")
    ;

    po::positional_options_description pos;
    pos.add("input",-1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
      .options(desc).positional(pos).run(), vm);
    if (argc == 1) {
      cout << desc << endl;
      return 0;
    }
    if (vm.count("conf")) {
      po::store( po::parse_config_file<char>(
        vm["conf"].as<string>().c_str(), desc), vm);
    }
    if (vm.count("output")==0 && vm.count("input")>1) {
      throw runtime_error(
        "output filename must be provided with multiple input files");
    }
    po::notify(vm);
  } catch (exception& e) {
    cerr << "Error in overlay options: " <<  e.what() << endl;
    return 1;
  }
  // end options ---------------------------------------------------

  // Accumulate *******************************************
  hist_group_map hmap;
  for (const file& f : ifiles) get_hists(f.f, hist_re, hmap);

  // Draw *************************************************
  vector<TLine> hline(hliney.size()), vline(vlinex.size());

  if (stats) gStyle->SetOptStat(stats);

  TCanvas canv;
  canv.SetLogx(logx);
  canv.SetLogy(logy);
  canv.SaveAs((ofname+'[').c_str());

  // TODO: allow alphabetic & sequential ordering
  for (auto& hp : hmap) {
    cout << "group: " << hp.first << endl;

    TH1 *h = hp.second.front();
    h->SetStats(false);
    // TODO: let regex function specified attribute override default here
    h->SetLineWidth(width[0]);
    h->SetLineStyle(style[0]);
    h->SetLineColor(color[0]);
    h->SetMarkerColor(color[0]);

    TAxis *xa = h->GetXaxis();
    xa->SetTitleSize(0.045);
    xa->SetMoreLogLabels(morelogx);
    xa->SetNoExponent(noexpx);

    TAxis *ya = h->GetYaxis();
    ya->SetTitleSize(0.045);
    ya->SetTitleOffset(1.0);
    ya->SetMoreLogLabels(morelogy);
    ya->SetNoExponent(noexpy);
    if (yrange.first!=yrange.second) {
      ya->SetRangeUser(yrange.first,yrange.second);
    } else {

      // determine vertical canvas range
      double ymin = numeric_limits<double>::max(),
             ymax = (logy ? 0 : numeric_limits<double>::min());

      for (auto* h : hp.second) {
        for (auto n=h->GetNbinsX(), i=1; i<=n; ++i) {
          double y = h->GetBinContent(i);
          if (logy && y<=0.) continue;
          if (y<ymin) ymin = y;
          if (y>ymax) ymax = y;
        }
      }

      if (logy) {
        tie(ymin,ymax) = forward_as_tuple(
          pow(10.,1.05*log10(ymin) - 0.05*log10(ymax)),
          pow(10.,1.05*log10(ymax) - 0.05*log10(ymin)));
      } else {
        bool both = false;
        if (ymin > 0.) {
          if (ymin/ymax < 0.25) {
            ymin = 0.;
            ymax /= 0.95;
          } else both = true;
        } else if (ymax < 0.) {
          if (ymin/ymax < 0.25) {
            ymax = 0.;
            ymin /= 0.95;
          } else both = true;
        } else if (ymin==0.) {
          ymax /= 0.95;
        } else if (ymax==0.) {
          ymin /= 0.95;
        } else both = true;
        if (both) {
          tie(ymin,ymax) = forward_as_tuple(
            1.05556*ymin - 0.05556*ymax,
            1.05556*ymax - 0.05556*ymin);
        }
      }

      ya->SetRangeUser(ymin,ymax);
    }

    h->SetStats(stats && hp.second.size()==1);
    h->Draw();
    if (stats && hp.second.size()==1) {
      auto* stat_box = static_cast<TPaveStats*>(h->FindObject("stats"));
      if (stat_box) stat_box->SetFillColorAlpha(0,0.65);
    }

    for (unsigned i=1, n=hp.second.size(); i<n; ++i) {
      h = hp.second[i];
      h->SetLineWidth(width[i]);
      h->SetLineStyle(style[i]);
      h->SetLineColor(color[i]);
      h->SetMarkerColor(color[i]);
      h->Draw("same");
    }

    for (unsigned i=0; i<hliney.size(); ++i)
    	hline[i].DrawLine(xa->GetXmax(), hliney[i], xa->GetXmin(), hliney[i]);
    for (unsigned i=0; i<vlinex.size(); ++i)
    	vline[i].DrawLine(vlinex[i], ya->GetXmax(), vlinex[i], ya->GetXmin());

    TLegend *leg = nullptr;
    if (legend && hp.second.size()>1) {
      leg = new TLegend(0.72,0.9-hp.second.size()*0.04,0.9,0.9);
      leg->SetFillColorAlpha(0,0.65);
      for (TH1 *h : hp.second) {
        // TODO: print real legend string
        leg->AddEntry(h, h->GetName());
      }
      leg->Draw();
    }

    canv.SaveAs(ofname.c_str());
    delete leg;
  }
  canv.SaveAs((ofname+']').c_str());

  return 0;
}
