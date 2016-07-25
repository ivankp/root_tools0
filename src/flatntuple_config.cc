#include "flatntuple_config.hh"

#include <iostream>
#include <fstream>
#include <cctype>
#include <stdexcept>

#include <boost/regex.hpp>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;
using std::string;
using std::runtime_error;

template <typename T>
inline T&& pop_back(T&& x) noexcept(noexcept(x.pop_back())) {
  test(x)
  x.pop_back();
  return std::forward<T>(x);
}

void flatntuple_config::parse(const std::string& cfname) {
  std::ifstream f(cfname);
  string line;

  while (std::getline(f,line)) { // skip blank and commented out lines
    bool skip = false;
    for (size_t i=0; ; ++i) {
      if (i==line.size()) {
        skip = true;
        break;
      }
      char c = line[i];
      if (std::isblank(c)) continue;
      if (c=='#') skip = true;
      break;
    }
    if (skip) continue;

    // parse --------------------------------------------------------
    static boost::regex
      branch_ex("( *([^ \\[\\]=]+ |) *([^ \\[\\]=]+) *=|)"
                " *([^ \\[\\]=]+) *(\\[ *\\]| ) *([^ \\[\\]=]+) *"),
      cut_ex("( *([^ \\[\\]=]+) *(\\[.*\\]| ) *([^ \\[\\]=]+) *([=><!]=?)|)"
             " *([^ \\[\\]=]*) *(\\[.*\\]| ) *([^ \\[\\]=]+) *");

    boost::smatch sm;
    test(line)
    if (boost::regex_match(line,sm,branch_ex)) {
      if (sm.str(4)!="tree") {
        cout << "branch" << endl;
        branch::type_char tin  = branch::get_type(sm.str(4));
        branch::type_char tout = sm[2].length()==0 ? tin
                               : branch::get_type(sm.str(2));
        bool arr = sm.str(5)!=" ";
        branches.emplace_back(typename decltype(branches)::value_type{
          branch {tin ,arr,sm.str(6)},
          branch {tout,arr,sm.str(3)}
        });
      } else {
        cout << "tree" << endl;
        if (itree.size()) throw runtime_error("tree redefined here: \'"+line+'\'');
        otree = sm.str(3);
        itree = sm.str(6);
        if (!otree.size()) otree = itree;
      }
    } else if (boost::regex_match(line,sm,cut_ex)) {
      cout << "cut\n";
      for (const auto& m : sm) test(m)
    } else throw runtime_error("ambiguous config statement: \'"+line+'\'');
    cout << endl;
  }
}

branch::type_char branch::get_type(std::string str) {
  if (str.back()==' ') str.pop_back();
  for (char& c : str)
    if (std::isupper(c)) c = std::tolower(c);

  if      (str=="int"    ) return I;
  else if (str=="uint"   ) return i;
  else if (str=="float"  ) return F;
  else if (str=="double" ) return D;
  else if (str=="long64" ) return L;
  else if (str=="ulong64") return l;
  else if (str=="bool"   ) return O;
  else if (str=="char"   ) return B;
  else if (str=="uchar"  ) return b;
  else if (str=="short"  ) return S;
  else if (str=="ushort" ) return s;
  else throw runtime_error("Unexpected branch type: \'"+str+'\'');
}
