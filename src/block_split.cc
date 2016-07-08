#include "block_split.hh"
#include <iterator>

std::vector<std::string>
block_split(const std::string& str, const std::string& delims) {
  std::vector<std::string> blocks;

  auto it=str.begin(), end=str.end();
  char d = '\0';

  while (it!=end) {
    std::string tmp;
    bool last_blank = false;
    for (auto begin = it; ; ) {
      const bool is_d = (d!='\0') ? *it==d
                      : delims.find(*it)!=std::string::npos;
      if (is_d) {
        if (*std::prev(it)=='\\') {
          int num_bs = 1;
          for (auto it1=std::prev(it,2); *it1=='\\'; --it1) ++num_bs;
          if (num_bs%2) { // escaped delimeter
            (tmp += { begin, std::prev(it,(num_bs+1)/2) }) += *it;
            begin = ++it;
          } else { // unescaped delimeter
            if (d=='\0') d = *it; // set delimeter
            tmp += { begin, std::prev(it,(num_bs+1)/2) };
            ++it;
            if (it==end) last_blank = true;
            break;
          }
        } else { // unescaped delimeter
          if (d=='\0') d = *it; // set delimeter
          tmp += { begin, it++ };
          if (it==end) last_blank = true;
          break;
        }
      } else if (it==end) {
        tmp += { begin, it };
        break;
      } else ++it;
    }
    blocks.emplace_back(std::move(tmp));
    if (last_blank) blocks.emplace_back();
  }
  return std::move(blocks);
}
