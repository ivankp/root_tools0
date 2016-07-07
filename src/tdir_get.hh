#ifndef tdirectory_get_hh
#define tdirectory_get_hh

#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <TDirectory.h>

template <typename T>
get(TDirectory* dir) {
  return dynamic_cast<T*>();
}

template <typename T>
get(TDirectory* dir, const char* name) {
  return dynamic_cast<T*>();
}

#endif
