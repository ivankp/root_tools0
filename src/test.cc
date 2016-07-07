#include <iostream>
#include <sstream>

#include "re_mod_base.hh"

using namespace std;

int main()
{
  re_mod_base re;
  stringstream("abc/(.*)_$/\\1") >> re;
  cout << endl;
  stringstream("abc/(.*)_$\\/\\1") >> re;
  cout << endl;
  stringstream("abc/(.*)_$\\\\/\\1") >> re;
  cout << endl;
  stringstream("abc/(.*)_$\\\\\\/\\1") >> re;
  cout << endl;
  stringstream("abc/(.*)_$\\\\\\\\/\\1") >> re;
  cout << endl;
  stringstream("abc/(.*)_$\\\\\\\\\\/\\1") >> re;
  cout << endl;
  stringstream("abc\\/(.*)_$/\\1") >> re;
  cout << endl;
  stringstream("abc:(.*)_$/\\1") >> re;
  cout << endl;
  stringstream("abc:(.*)_$:\\1") >> re;
  cout << endl;
  stringstream("abc:(.*)_$\\:\\1") >> re;
  cout << endl;
  stringstream("abc:(.*)_$\\\\:\\1") >> re;
  cout << endl;
  stringstream("abc::\\1") >> re;
  cout << endl;
  stringstream("::\\1") >> re;
}
