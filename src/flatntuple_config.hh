#ifndef flatntuple_config_hh
#define flatntuple_config_hh

#include <vector>
#include <array>
#include <string>

struct branch {
  enum type_char : char {
    I = 'I', i = 'i', F = 'F', D = 'D', L = 'L', l = 'l', O = 'O',
    B = 'B', b = 'b', S = 'S', s = 's'
  } type;
  bool is_array;
  std::string name;

  static type_char get_type(std::string str);
};

struct flatntuple_config {
  std::vector<std::array<branch,2>> branches;
  std::string itree, otree;

  void parse(const std::string& cfname);
};

#endif
