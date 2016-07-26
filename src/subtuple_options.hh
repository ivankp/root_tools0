#ifndef subtuple_options_hh
#define subtuple_options_hh

#include <vector>
#include <string>

struct subtuple_options {
  std::vector<std::string> input;
  std::string output, config;
  bool parse(int argc, const char** argv);
};

#endif
