CXX := g++
STD := -std=c++11
DF := $(STD)
CF := $(STD) -Wall -O3 -flto
LF := $(STD) -flto

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

C_rxplot := $(ROOT_CFLAGS)
L_rxplot := $(ROOT_LIBS) -lboost_program_options -lboost_regex

C_subtuple := $(ROOT_CFLAGS)
L_subtuple := $(ROOT_LIBS) -lTreePlayer -lboost_regex

C_br := $(ROOT_CFLAGS)
L_br := $(ROOT_LIBS)

C_hrat := $(ROOT_CFLAGS)
L_hrat := $(ROOT_LIBS)

C_tbrowser := $(ROOT_CFLAGS)
L_tbrowser := $(ROOT_LIBS)

C_hist_fmt_re := $(ROOT_CFLAGS)

C_envelope := $(ROOT_CFLAGS) -Isrc
L_envelope := $(ROOT_LIBS)

SRC := src
BIN := bin
BLD := .build

SRCS := $(shell find $(SRC) -type f -name '*.cc')
DEPS := $(patsubst $(SRC)%.cc,$(BLD)%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRC)
EXES := $(patsubst $(SRC)%.cc,$(BIN)%,$(shell $(GREP_EXES)))

NODEPS := clean
.PHONY: all clean

all: $(EXES)

bin/rxplot: $(BLD)/block_split.o $(BLD)/hist_fmt_re.o
bin/subtuple: $(BLD)/subtuple_options.o $(BLD)/subtuple_config.o

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
-include $(DEPS)
endif

$(DEPS): $(BLD)/%.d: $(SRC)/%.cc | $(BLD)
	$(CXX) $(DF) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CF) $(C_$*) -c $(filter %.cc,$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LF) $(filter %.o,$^) -o $@ $(L_$*)

$(BLD) $(BIN):
	mkdir $@

clean:
	@rm -rfv $(BLD) $(BIN)
