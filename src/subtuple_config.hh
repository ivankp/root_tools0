#ifndef subtuple_config_hh
#define subtuple_config_hh

#include <vector>
#include <array>
#include <string>

// #define ROOT_TYPE_CHAR_SEQ (I)(i)(F)(D)(L)(l)(O)(B)(b)(S)(s)
#define ROOT_TYPE_SEQ \
  ((I)(Int_t))((i)(UInt_t)) \
  ((F)(Float_t))((D)(Double_t)) \
  ((L)(Long64_t))((l)(ULong64_t)) \
  ((O)(Bool_t))((B)(Char_t))((b)(UChar_t)) \
  ((S)(Short_t))((s)(UShort_t))

struct branch {
  enum type_char : char {
    I = 'I', i = 'i', F = 'F', D = 'D', L = 'L', l = 'l', O = 'O',
    B = 'B', b = 'b', S = 'S', s = 's'
  } type;
  bool is_array;
  std::string name;

  static type_char get_type(std::string str);
  static std::string type_str(type_char t) noexcept;
};

struct cut {
  std::string op, val;
};

struct subtuple_config {
  std::string itree, otree;
  std::vector<std::array<branch,2>> branches;
  std::vector<std::pair<branch,cut>> cuts;

  void parse(const std::string& cfname);
};

#endif
