#include "subtuple_config.hh"

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

void subtuple_config::parse(const std::string& cfname) {
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
    if (boost::regex_match(line,sm,branch_ex)) {
      if (sm.str(4)!="tree") {
        branch::type_char tin  = branch::get_type(sm.str(4));
        branch::type_char tout = sm[2].length()==0 ? tin
                               : branch::get_type(sm.str(2));
        bool arr = sm.str(5)!=" ";
        branches.emplace_back(typename decltype(branches)::value_type{
          branch {tin ,arr,sm.str(6)},
          branch {tout,arr,sm.str(3)}
        });
      } else {
        if (itree.size()) throw runtime_error("tree redefined here: \'"+line+'\'');
        otree = sm.str(3);
        itree = sm.str(6);
        if (!otree.size()) otree = itree;
      }
    } else if (boost::regex_match(line,sm,cut_ex)) {
      cuts.emplace_back(
        branch {branch::get_type(sm.str(2)), false, sm.str(4)},
        cut {sm.str(5),sm.str(8)}
      );
    } else throw runtime_error("ambiguous config statement: \'"+line+'\'');
  }

  if (!itree.size()) throw runtime_error("no tree specified");
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

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>

std::string branch::type_str(branch::type_char t) noexcept {
  #define CASE(r, data, elem) case branch:: BOOST_PP_SEQ_ELEM(0,elem): \
    return BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(1,elem)); break;
  switch (t) {
    BOOST_PP_SEQ_FOR_EACH(CASE, _, ROOT_TYPE_SEQ)
  }
  #undef CASE
  return nullptr; // actually never happens
}
