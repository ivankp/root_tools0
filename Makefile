EXE := overlay subtuple test

CXXFLAGS := -std=c++11 -Wall -O3

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

FLAGS_overlay := $(ROOT_CFLAGS)
LIBS_overlay := $(ROOT_LIBS) -lboost_program_options -lboost_regex

FLAGS_subtuple := $(ROOT_CFLAGS)
LIBS_subtuple := $(ROOT_LIBS) -lTreePlayer

# FLAGS_test := -DDEBUG -DUSE_STD_REGEX
# LIBS_test := -lboost_regex

.PHONY: all clean

all: $(EXE:%=bin/%)

bin/subtuple: src/timed_counter2.hh
bin/test: src/*.hh

bin/%: src/%.cc | bin
	g++ $(CXXFLAGS) $(FLAGS_$*) $(filter %.cc,$^) -o $@ $(LIBS_$*)

bin:
	mkdir $@

clean:
	@rm -rfv bin
