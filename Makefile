CXX := g++
STD := -std=c++11
DF := $(STD)
CF := $(STD) -Wall -g #-O3 -flto
LF := $(STD) #-flto

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

C_overlay := $(ROOT_CFLAGS)
L_overlay := $(ROOT_LIBS) -lboost_program_options -lboost_regex

C_overlay2 := $(ROOT_CFLAGS)
L_overlay2 := $(ROOT_LIBS) -lboost_program_options -lboost_regex

C_subtuple := $(ROOT_CFLAGS)
L_subtuple := $(ROOT_LIBS) -lTreePlayer

C_test := $(ROOT_CFLAGS)
L_test := $(ROOT_LIBS) -lboost_regex

C_hist_fmt_re := $(ROOT_CFLAGS)

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

bin/test: $(BLD)/block_split.o $(BLD)/hist_fmt_re.o
bin/overlay2: $(BLD)/block_split.o $(BLD)/hist_fmt_re.o

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
