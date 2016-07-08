#include <iostream>
#include <sstream>

#include "block_split.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace std;

int main(int argc, char *argv[])
{
  cout << endl;
  for (int i=1; i<argc; ++i) {
    cout << "str "<<i<<": " << argv[i] << endl;
    int j=0;
    for (const auto& s : block_split(argv[i]))
      cout << (j++) << ": " << s << endl;
    cout << endl;
  }
}
