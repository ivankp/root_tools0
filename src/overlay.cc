// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <tuple>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <TFile.h>
#include <TKey.h>
#include <TH1.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLine.h>
#include <TStyle.h>
#include <TPaveStats.h>

#include "ring.hh"
// #include "hist_fmt_re.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;
namespace po = boost::program_options;

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

class regsub {
  vector<pair<boost::regex,string>> re;
public:
  inline operator bool() const noexcept { return re.size(); }
  inline string operator()(const string& str) const {
    string s;
    if (*this) s = boost::regex_replace(str, re.front().first, re.front().second,
      boost::match_default | boost::format_sed | boost::format_no_copy );
    else return s;
    for (auto it=++re.begin(); it!=re.end(); ++it)
      s = boost::regex_replace(s, it->first, it->second,
        boost::match_default | boost::format_sed );
    return s;
  }
  friend istream& operator>>(istream& in, regsub& rs) {
    char delim;
    string buff;
    in.get(delim);
    for (;;) {
      if ( !getline(in,buff,delim) ) break;
      rs.re.emplace_back(piecewise_construct,
        forward_as_tuple(move(buff)),
        forward_as_tuple(string{}));
      if ( !getline(in,buff,delim) ) break;
      rs.re.back().second = move(buff);
      if (in.eof()) break;
    }
    return in;
  }
};

int main(int argc, char **argv)
{
  string ofname, cfname;
  vector<string> ifname, iflbl;
  ring<Color_t> color;
  ring<Style_t> style;
  ring<Width_t> width;
  pair<double,double> yrange(0.,0.);
  int stats;
  bool legend,
       logx, morelogx, noexpx,
       logy, morelogy, noexpy;
  vector<double> hliney, vlinex;
  regsub hist_select, hist_title, hist_leg, xtitle, ytitle;
  // options ---------------------------------------------------
  try {
    po::options_description desc("Options");
    desc.add_options()
      ("input,i", po::value(&ifname)->multitoken()->required(),
       "*input root file names")
      ("output,o", po::value(&ofname)->required(),
       "*output pdf file name")
      ("conf,c", po::value(&cfname),
       "configuration file name")

      ("regex.select,r", po::value(&hist_select),
       "regex for selecting histogram")
      ("regex.title", po::value(&hist_title),
       "regex for making titles from regex.select output")
      ("regex.legend", po::value(&hist_leg),
       "regex for making legend entries from hist names")

      ("regex.xtitle", po::value(&xtitle), "horizontal axis title")
      ("regex.ytitle", po::value(&ytitle), "vertical axis title")

      ("legend,l", po::bool_switch(&legend), "draw legend")
      ("stats", po::value(&stats)->default_value(0),
       "draw stats box: e.g. 111110")

      ("file-label", po::value(&iflbl)->multitoken(),
       "labels associated with input files")
      ("colors", po::value(color.v_ptr())->multitoken()->
        default_value({602,46}, "{602,46}"),
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

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    if (argc == 1) {
      cout << desc << endl;
      return 0;
    }
    if (vm.count("conf")) {
      po::store( po::parse_config_file<char>(
        vm["conf"].as<string>().c_str(), desc), vm);
    }
    po::notify(vm);

    if (iflbl.size() && (iflbl.size() != ifname.size()))
      throw runtime_error("All or none file labels must be specified");

  } catch (exception& e) {
    cerr << "Error in overlay options: " <<  e.what() << endl;
    return 1;
  }
  // end options ---------------------------------------------------

  vector<unique_ptr<TFile>> ff;
  vector<pair<string,vector<TH1*>>> hh;
  unordered_map<TH1*,const string*> hflbl;

  TH1* h = nullptr;

  // Accumulate *******************************************
  for (size_t i=0, n=ifname.size(); i<n; ++i) {
    static TFile *f;
    ff.emplace_back(f = new TFile(ifname[i].c_str(),"read"));
    if (f->IsZombie()) return 1;
    cout << "Input file: " << f->GetName() << endl;

    TIter nextkey(f->GetListOfKeys());
    while (TKey *key = static_cast<TKey*>(nextkey())) {
      TObject *obj = key->ReadObj();
      if (obj->InheritsFrom(TH1::Class())) {
        string name(obj->GetName());

        const string hlbl = hist_select(obj->GetName());
        if (!hlbl.size()) continue;

        auto ithh = find_if(hh.begin(), hh.end(),
          [hlbl](const pair<string,vector<TH1*>>& v){ return v.first==hlbl; });
        if (ithh == hh.end()) {
          hh.emplace_back(hlbl,vector<TH1*>());
          ithh = --hh.end();
        }
        ithh->second.push_back(h = static_cast<TH1*>(obj));
        //h->SetTitle(hlbl.c_str());
        if (iflbl.size()) hflbl[h] = &iflbl[i];
      }
    }
  }
  cout << endl;

  // Draw *************************************************
  vector<TLine> hline(hliney.size()), vline(vlinex.size());

  if (stats) gStyle->SetOptStat(stats);

  TCanvas canv;
  canv.SetLogx(logx);
  canv.SetLogy(logy);
  canv.SaveAs((ofname+'[').c_str());

  for (auto& hp : hh) {
    cout << hp.first << endl;

    h = hp.second.front();
    h->SetStats(false);
    h->SetLineWidth(width[0]);
    h->SetLineStyle(style[0]);
    h->SetLineColor(color[0]);
    h->SetMarkerColor(color[0]);
    if (hist_title)
      h->SetTitle(hist_title(h->GetName()).c_str());

    TAxis *xa = h->GetXaxis();
    xa->SetTitleSize(0.045);
    xa->SetMoreLogLabels(morelogx);
    xa->SetNoExponent(noexpx);
    if (xtitle)
      xa->SetTitle(xtitle(h->GetName()).c_str());

    TAxis *ya = h->GetYaxis();
    ya->SetTitleSize(0.045);
    ya->SetTitleOffset(1.0);
    ya->SetMoreLogLabels(morelogy);
    ya->SetNoExponent(noexpy);
    if (ytitle)
      ya->SetTitle(ytitle(ya->GetTitle()).c_str());
    if (yrange.first!=yrange.second) {
      ya->SetRangeUser(yrange.first,yrange.second);
    } else {

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
        tie(ymin,ymax) =
        forward_as_tuple(pow(10.,1.05*log10(ymin) - 0.05*log10(ymax)),
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
          tie(ymin,ymax) =
          forward_as_tuple(1.05556*ymin - 0.05556*ymax, 1.05556*ymax - 0.05556*ymin);
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

    for (size_t i=1, n=hp.second.size(); i<n; ++i) {
      h = hp.second[i];
      h->SetLineWidth(width[i]);
      h->SetLineStyle(style[i]);
      h->SetLineColor(color[i]);
      h->SetMarkerColor(color[i]);
      h->Draw("same");
    }

    for (size_t i=0; i<hliney.size(); ++i)
    	hline[i].DrawLine(xa->GetXmax(), hliney[i], xa->GetXmin(), hliney[i]);
    for (size_t i=0; i<vlinex.size(); ++i)
    	vline[i].DrawLine(vlinex[i], ya->GetXmax(), vlinex[i], ya->GetXmin());

    TLegend *leg = nullptr;
    if (legend && hp.second.size()>1) {
      leg = new TLegend(0.72,0.9-hp.second.size()*0.04,0.9,0.9);
      leg->SetFillColorAlpha(0,0.65);
      for (TH1 *h : hp.second) {
        string ent = hist_leg(h->GetName());
        if (iflbl.size()) ent = *hflbl[h]+' '+ent;
        leg->AddEntry(h, ent.c_str());
      }
      leg->Draw();
    }

    canv.SaveAs(ofname.c_str());
    delete leg;
  }
  canv.SaveAs((ofname+']').c_str());

  return 0;
}
