EXE := overlay subtuple

CXXFLAGS := -std=c++11 -Wall -O3
LIBFLAGS := -Wl,--as-needed

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

LIBS_overlay := $(ROOT_LIBS) -lboost_program_options -lboost_regex

bin/subtuple: src/timed_counter.hh

.PHONY: all clean

all: $(EXE:%=bin/%)

bin/%: src/%.cc | bin
	g++ $(CXXFLAGS) $(ROOT_CFLAGS) $^ -o $@ $(LIBFLAGS) $(ROOT_LIBS) $(LIBS_$*)

bin:
	mkdir $@

clean:
	@rm -rfv bin
