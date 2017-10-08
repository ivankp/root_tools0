#ifndef HISTS_RANGE_HH
#define HISTS_RANGE_HH

template <typename HH, typename Pred>
std::pair<double,double>
hists_range(const HH& hh, Pred f, bool logy=false) {
  double ymin = std::numeric_limits<double>::max(),
         ymax = (logy ? 0 : std::numeric_limits<double>::min());

  for (const auto& h : hh) {
    for (int i=1, n=f(h)->GetNbinsX(); i<=n; ++i) {
      const auto y = f(h)->GetBinContent(i);
      if (logy && y<=0.) continue;
      const auto e = f(h)->GetBinError(i);
      if (y==0. && e==0.) continue; // ignore empty bins
      if (y-e<ymin) ymin = y-e;
      if (y+e>ymax) ymax = y+e;
    }
  }

  if (logy) {
    std::tie(ymin,ymax) = std::forward_as_tuple(
      std::pow(10.,1.05*std::log10(ymin) - 0.05*std::log10(ymax)),
      std::pow(10.,1.05*std::log10(ymax) - 0.05*std::log10(ymin)));
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
      std::tie(ymin,ymax) = std::forward_as_tuple(
        1.05556*ymin - 0.05556*ymax,
        1.05556*ymax - 0.05556*ymin);
    }
  }

  return { ymin, ymax };
}

#endif
