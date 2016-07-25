#include "flatntuple_options.hh"

#include <iostream>
#include <cstring>
#include <stdexcept>

using namespace std;

enum current_opt { opt_input, opt_output, opt_config };

bool flatntuple_options::parse(int argc, const char** argv) {
  if (argc==1 || (argc==2 && !strcmp(argv[1],"-h"))) {
    cout << argv[0] << " options:\n"
         << "  -i : input file(s)\n"
         << "  -o : output file\n"
         << "  -c : config file\n" << endl;
    return true;
  } else {
    current_opt opt = opt_input;
    bool prev_is_opt = false;
    for (int i=1; i<argc; ++i) {
      if (strlen(argv[i])==2 && argv[i][0]=='-') {
        switch (argv[i][1]) {
          case 'i': opt = opt_input; break;
          case 'o': opt = opt_output;
            if (output.size()) throw runtime_error("multiple -o options");
            break;
          case 'c': opt = opt_config;
            if (config.size()) throw runtime_error("multiple -c options");
            break;
          default: throw runtime_error(string("unrecognized option: ")+argv[i]);
        }
        if (prev_is_opt) throw runtime_error(
          string("blank option before: ")+argv[i]);
        if (argc-i==1) throw runtime_error(string("blank option: ")+argv[i]);
        prev_is_opt = true;
      } else {
        switch (opt) {
          case opt_input : input.emplace_back(argv[i]); break;
          case opt_output: output = argv[i]; break;
          case opt_config: config = argv[i]; break;
        }
        prev_is_opt = false;
      }
    }
    if (!output.size()) throw runtime_error("-o options is required");
    if (!config.size()) throw runtime_error("-c options is required");

    return false;
  }
}
