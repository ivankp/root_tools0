// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>
#include <set>
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
#include "deref_less.hh"
#include "array_istream_op.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;
namespace po = boost::program_options;

using shared_str = std::shared_ptr<std::string>;
class group_map {
  struct key {
    shared_str g;
    unsigned i;
    key(shared_str&& g, unsigned i): g(move(g)), i(i) { }
    inline const std::string& operator*() const noexcept(noexcept(*g)) {
      return *g;
    }
  };
  using set_t = std::set<key,deref_less<key>>;
  set_t s;
  using vec_t = std::vector<std::vector<hist_fmt_re::hist_wrap>>;
  vec_t v;

public:
  static bool unsorted;

  void emplace(hist_fmt_re::hist_wrap&& h) {
    auto g = s.emplace(std::move(h.group),v.size());
    if (g.second) {
      v.emplace_back();
      v.back().emplace_back(h.h,g.first->g,std::move(h.legend));
    } else v[g.first->i].emplace_back(h.h,g.first->g,std::move(h.legend));
  }

  struct iter_wrap {
    typename vec_t::iterator it_v;
    typename set_t::iterator it_s;

    inline typename vec_t::reference operator*() noexcept {
      return  *(unsorted ? it_v : it_v+it_s->i);
    }
    inline typename vec_t::pointer operator->() noexcept {
      return &*(unsorted ? it_v : it_v+it_s->i);
    }
    inline iter_wrap& operator++() noexcept {
      if (unsorted) ++it_v;
      else ++it_s;
      return *this;
    }
    inline bool operator!=(const iter_wrap& o) const noexcept {
      return unsorted ? it_v != o.it_v : it_s != o.it_s;
    }
  };

  inline iter_wrap begin() noexcept {
    return {v.begin(),s.begin()};
  }
  inline iter_wrap end() noexcept {
    return {v.end(),s.end()};
  }
};
bool group_map::unsorted = true;

bool multiple_files;
void get_hists(TDirectory* dir,
  const vector<hist_fmt_re>& re, group_map& gmap
) noexcept {
  TIter nextkey(dir->GetListOfKeys());
  TKey *key;
  while ((key = static_cast<TKey*>(nextkey()))) {
    TObject *obj = key->ReadObj();
    if (obj->InheritsFrom(TH1::Class())) {

      TH1* h = static_cast<TH1*>(obj);

      string group, legend = h->GetName();
      if (multiple_files) {
        group = get_hist_file_str(h);
      } else {
        group = get_hist_dirs_str(h);
        if (!group.size()) {
          // const auto sep = legend.find('_');
          // group  = legend.substr(0,sep);
          // legend = legend.substr(sep+1);
          group = legend;
        }
      }
      hist_fmt_re::hist_wrap hw(h,move(group),move(legend));

      if ( apply(re,hw) ) gmap.emplace(std::move(hw));

    } else if (obj->InheritsFrom(TDirectory::Class())) {
      get_hists( static_cast<TDirectory*>(obj), re, gmap );
    }
    // TODO: allow TGraphs to be drawn as well
  }
}

int main(int argc, char **argv)
{
  string ofname, cfname;
  vector<string> ifname, re_str;
  ring<Color_t> color, marker_color;
  ring<Style_t> style;
  ring<Width_t> width;
  ring<Size_t> marker_size;
  std::array<double,2> xrange {0.,0.}, yrange {0.,0.};
  std::array<float,4> margins {0.1,0.1,0.1,0.1};
  int stats;
  bool legend,
       logx, morelogx, noexpx, gridx,
       logy, morelogy, noexpy, gridy,
       ticks_left, ticks_top;
  Float_t label_size_x, title_size_x, title_offset_x,
          label_size_y, title_size_y, title_offset_y;
  string val_fmt;
  vector<double> hliney, vlinex;
  bool sort_groups;

  // TODO: allow options to be applied only to some groups
  // TODO: text boxes
  // TODO: lines' and textboxes' styles

  // options ---------------------------------------------------
  try {
    po::options_description desc("Options");
    desc.add_options()
      ("input,i", po::value(&ifname)->multitoken()->required(),
       "*input root file names")
      ("output,o", po::value(&ofname),
       "output pdf file name")
      ("conf,c", po::value(&cfname),
       "configuration file name")

      ("regex,r", po::value(&re_str)->multitoken(),
       "regex for organizing histograms")
      ("sort-groups", po::bool_switch(&sort_groups),
       "draw groups in alphabetic order\ninstead of sequential")

      ("legend,l", po::bool_switch(&legend), "draw legend")
      ("stats,s", po::value(&stats)->default_value(0),
       "draw stats box: e.g. 111110")
      ("val-fmt", po::value(&val_fmt),
       "gStyle->SetPaintTextFormat()\n+ TH1::Draw(\"TEXT\")")

      ("colors", po::value(color.v_ptr())->multitoken()->
        default_value({602,46,8,90,44,52}, "{602,46,8,90,44,52}"),
       "histograms colors")
      ("widths", po::value(width.v_ptr())->multitoken()->
        default_value({2}, "{2}"),
       "histograms line widths")
      ("styles", po::value(style.v_ptr())->multitoken(),
       "histograms line styles")
      ("marker-size", po::value(marker_size.v_ptr())->multitoken(),
       "histograms marker size")
      ("marker-color", po::value(marker_color.v_ptr())->multitoken(),
       "histograms marker color")

      ("xrange,x", po::value(&xrange), "horizontal axis range")
      ("yrange,y", po::value(&yrange), "vertical axis range")
      ("margins,m", po::value(&margins), "canvas margins")

      ("logx", po::bool_switch(&logx), "logarithmic horizontal axis")
      ("logy", po::bool_switch(&logy), "logarithmic vertical axis")
      ("mlogx", po::bool_switch(&morelogx), "more X logarithmic labels")
      ("mlogy", po::bool_switch(&morelogy), "more Y logarithmic labels")
      ("noexpx", po::bool_switch(&noexpx), "")
      ("noexpy", po::bool_switch(&noexpy), "")
      ("gridx", po::bool_switch(&gridx), "")
      ("gridy", po::bool_switch(&gridy), "")
      ("ticks-left", po::bool_switch(&ticks_left), "")
      ("ticks-top", po::bool_switch(&ticks_top), "")

      ("xlabel-size", po::value(&label_size_x)->default_value(1), "")
      ("ylabel-size", po::value(&label_size_y)->default_value(1), "")
      ("xtitle-size", po::value(&title_size_x)->default_value(1), "")
      ("ytitle-size", po::value(&title_size_y)->default_value(1), "")
      ("xtitle-offset", po::value(&title_offset_x)->default_value(1), "")
      ("ytitle-offset", po::value(&title_offset_y)->default_value(1), "")

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
    cerr <<"\033[31mError in rxplot options: "<<e.what()<<"\033[0m"<< endl;
    return 1;
  }
  if (ifname.size()==1) {
    if (ofname.size()==0) {
      auto slash = ifname.front().find('/');
      auto   dot = ifname.front().rfind('.');
      ofname = slash==string::npos
        ? ifname.front().substr(0,dot)
        : ifname.front().substr(slash+1,dot-slash-1);
      ofname += ".pdf";
    }
  } else {
    multiple_files = true;
  }
  cout << "Output file: " << ofname << endl;
  // end options ---------------------------------------------------

  // Parse regex ******************************************
  vector<hist_fmt_re> re;
  re.reserve(re_str.size());
  for (const auto& str : re_str) {
    try {
      re.emplace_back(str);
    } catch (exception& e) {
      cerr <<"\033[31mError in parsing regex: "<<e.what()<<"\033[0m"<< endl;
      return 1;
    }
  }

  // Accumulate *******************************************
  group_map gmap;
  vector<unique_ptr<TFile>> ff;
  ff.reserve(ifname.size());
  for (const auto& name : ifname) {
    static TFile *f;
    ff.emplace_back(f = new TFile(name.c_str(),"read"));
    if (f->IsZombie()) return 1;
    cout << "Input file: " << f->GetName() << endl;

    get_hists(f, re, gmap);
  }
  cout << endl;

  // Draw *************************************************
  vector<TLine> hline(hliney.size()), vline(vlinex.size());

  if (stats) gStyle->SetOptStat(stats);

  if (val_fmt.size()) gStyle->SetPaintTextFormat(val_fmt.c_str());

  TCanvas canv;
  if (logx) canv.SetLogx();
  if (logy) canv.SetLogy();
  canv.SetMargin(get<0>(margins),get<1>(margins),
                 get<2>(margins),get<3>(margins));
  if (ticks_left) gPad->SetTicky();
  if (ticks_top ) gPad->SetTickx();
  if (gridx) gPad->SetGridx();
  if (gridy) gPad->SetGridy();
  canv.SaveAs((ofname+'[').c_str());

  if (sort_groups) group_map::unsorted = false;
  for (auto&& hh : gmap) {
    cout << *hh.front().group << endl;

    TH1 *h = hh.front().h;
    h->SetStats(false);
    // TODO: let regex function specified attribute override default here
    h->SetLineColor(color[0]);
    h->SetLineWidth(width[0]);
    if (style.size()) h->SetLineStyle(style[0]);
    if (marker_color.size()) h->SetMarkerColor(marker_color[0]);
    else h->SetMarkerColor(color[0]);
    if (marker_size.size()) h->SetMarkerSize(marker_size[0]);

    TAxis *xa = h->GetXaxis();
    xa->SetLabelSize(label_size_x * xa->GetLabelSize());
    xa->SetTitleSize(title_size_x * xa->GetTitleSize());
    xa->SetTitleOffset(title_offset_x * xa->GetTitleOffset());
    xa->SetMoreLogLabels(morelogx);
    xa->SetNoExponent(noexpx);
    if (get<0>(xrange)!=get<1>(xrange)) {
      xa->SetRangeUser(get<0>(xrange),get<1>(xrange));
    }

    TAxis *ya = h->GetYaxis();
    ya->SetLabelSize(label_size_y * ya->GetLabelSize());
    ya->SetTitleSize(title_size_y * ya->GetTitleSize());
    ya->SetTitleOffset(title_offset_y * ya->GetTitleOffset());
    ya->SetMoreLogLabels(morelogy);
    ya->SetNoExponent(noexpy);
    if (get<0>(yrange)!=get<1>(yrange)) {
      ya->SetRangeUser(get<0>(yrange),get<1>(yrange));
    } else {

      // determine vertical canvas range
      double ymin = numeric_limits<double>::max(),
             ymax = (logy ? 0 : numeric_limits<double>::min());

      for (const auto& h : hh) {
        for (int i=1, n=h.h->GetNbinsX(); i<=n; ++i) {
          double y = h.h->GetBinContent(i);
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

    h->SetStats(stats && hh.size()==1);
    string draw_opt = h->GetOption();
    if (val_fmt.size()) draw_opt += "HIST TEXT0";
    h->Draw(draw_opt.c_str());
    if (stats && hh.size()==1) {
      auto* stat_box = static_cast<TPaveStats*>(h->FindObject("stats"));
      if (stat_box) stat_box->SetFillColorAlpha(0,0.65);
    }

    if (draw_opt.size()) draw_opt += " ";
    draw_opt += "SAME";
    for (unsigned i=1, n=hh.size(); i<n; ++i) {
      h = hh[i].h;
      h->SetLineColor(color[i]);
      h->SetLineWidth(width[i]);
      if (style.size()) h->SetLineStyle(style[i]);
      if (marker_color.size()) h->SetMarkerColor(marker_color[i]);
      else h->SetMarkerColor(color[i]);
      if (marker_size.size()) h->SetMarkerSize(marker_size[i]);
      h->Draw(draw_opt.c_str());
    }

    for (unsigned i=0; i<hliney.size(); ++i)
    	hline[i].DrawLine(xa->GetXmax(), hliney[i], xa->GetXmin(), hliney[i]);
    for (unsigned i=0; i<vlinex.size(); ++i)
    	vline[i].DrawLine(vlinex[i], ya->GetXmax(), vlinex[i], ya->GetXmin());

    TLegend *leg = nullptr;
    if (legend && hh.size()>1) {
      leg = new TLegend(0.72,0.9-hh.size()*0.04,0.9,0.9);
      leg->SetFillColorAlpha(0,0.65);
      for (const auto& h : hh)
        leg->AddEntry(h.h, h.legend->c_str());
      leg->Draw();
    }

    canv.SaveAs(ofname.c_str());
    delete leg;
  }
  canv.SaveAs((ofname+']').c_str());

  return 0;
}
