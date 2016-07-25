#ifndef flatntuple_options_hh
#define flatntuple_options_hh

#include <vector>
#include <string>

struct flatntuple_options {
  std::vector<std::string> input;
  std::string output, config;
  bool parse(int argc, const char** argv);
};

#endif
