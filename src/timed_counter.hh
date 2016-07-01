#ifndef timed_counter_hh
#define timed_counter_hh

#include <iostream>
#include <iomanip>
#include <chrono>

template <typename I, typename Enable = void>
class timed_counter { };

template <typename I>
class timed_counter<I,typename std::enable_if<std::is_integral<I>::value>::type>
{
  using clock = std::chrono::system_clock;
  using time  = std::chrono::time_point<clock>;
  using dur   = std::chrono::duration<double>;

  I cnt;
  time start, last;

  void print() {
    using std::cout;
    using std::setw;
    using std::setfill;

    const auto dt = dur((last = clock::now()) - start).count();
    const int hours   = dt/3600;
    const int minutes = (dt-hours*3600)/60;
    const int seconds = (dt-hours*3600-minutes*60);

    cout << setw(12) << cnt << " | ";
    if (hours) {
      cout << setw(5) << hours << ':'
      << setfill('0') << setw(2) << minutes << ':'
      << setw(2) << seconds << setfill(' ');
    } else if (minutes) {
      cout << setw(2) << minutes << ':'
      << setfill('0') << setw(2) << seconds << setfill(' ');
    } else {
      cout << setw(2) << seconds <<'s';
    }

    cout.flush();
    if (hours)        for (char i=0;i<26;++i) cout << '\b';
    else if (minutes) for (char i=0;i<20;++i) cout << '\b';
    else              for (char i=0;i<18;++i) cout << '\b';
  }
  void print_check() {
    if ( dur(clock::now()-last).count() > 1 ) print();
  }

public:
  timed_counter(I i=0): cnt(i), start(clock::now()), last(start) { print(); }
  ~timed_counter() { print(); std::cout << std::endl; }

  // prefix
  I operator++() { print_check(); return ++cnt; }
  I operator--() { print_check(); return --cnt; }

  // postfix
  I operator++(int) { print_check(); return cnt++; }
  I operator--(int) { print_check(); return cnt--; }

  template <typename T> I operator+= (T i) { return cnt += i; }
  template <typename T> I operator-= (T i) { return cnt -= i; }

  template <typename T> inline bool operator<  (T i) { return cnt <  i; }
  template <typename T> inline bool operator<= (T i) { return cnt <= i; }
  template <typename T> inline bool operator>  (T i) { return cnt >  i; }
  template <typename T> inline bool operator>= (T i) { return cnt >= i; }

  // cast to integral type
  template <typename T> inline operator T () {
    static_assert( std::is_integral<T>::value,
      "Cannot cast timed_counter to a non-integral type" );
    return cnt;
  }
};

#endif
