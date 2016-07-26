// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include "timed_counter.hh"
#include "flatntuple_options.hh"
#include "flatntuple_config.hh"

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#define PP_SEQ_FOR_EACH_R_ID() BOOST_PP_SEQ_FOR_EACH_R
#define PP_DEFER(x) x BOOST_PP_EMPTY()

using namespace std;

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

struct value_wrap_base {
  virtual ~value_wrap_base() { }
};
template <typename T, bool Array=false>
struct value_wrap: public value_wrap_base {
  TTreeReaderValue<T> value;
  value_wrap(TTreeReader& reader, const string& name)
  : value(reader,name.c_str()) { }
  virtual ~value_wrap() { }
};
template <typename T>
struct value_wrap<T,true>: public value_wrap_base {
  TTreeReaderArray<T> value;
  value_wrap(TTreeReader& reader, const string& name)
  : value(reader,name.c_str()) { }
  virtual ~value_wrap() { }
};

using value_ptr = std::unique_ptr<value_wrap_base>;

value_ptr::pointer make_value(TTreeReader& reader, const branch& ibr) {
  #define CASE(r, data, elem) case branch:: BOOST_PP_SEQ_ELEM(0,elem): \
    return new value_wrap<BOOST_PP_SEQ_ELEM(1,elem),data> \
      (reader,ibr.name); break;
  if (!ibr.is_array) {
    switch (ibr.type) {
      BOOST_PP_SEQ_FOR_EACH(CASE, false, ROOT_TYPE_SEQ)
    }
  } else {
    switch (ibr.type) {
      BOOST_PP_SEQ_FOR_EACH(CASE, true, ROOT_TYPE_SEQ)
    }
  }
  #undef CASE
  return nullptr; // actually never happens
}

struct branch_pipe_base {
  virtual void copy() =0;
};

template <typename In, typename Out, bool Array=false>
struct branch_pipe: public branch_pipe_base {
  TTreeReaderValue<In> *in;
  Out out;
  branch_pipe(value_ptr::pointer val, TTree* tree, const branch& obr)
  : in( &static_cast<value_wrap<In>*>(val)->value ) {
    tree->Branch(obr.name.c_str(), &out,
                (obr.name + "/" + (char)obr.type).c_str());
  }
  virtual void copy() noexcept(noexcept(out = **in)) {
    out = **in;
  }
};
template <typename In, typename Out>
struct branch_pipe<In,Out,true>: public branch_pipe_base {
  TTreeReaderArray<In> *in;
  std::vector<Out> out;
  branch_pipe(value_ptr::pointer val, TTree* tree, const branch& obr)
  : in( &static_cast<value_wrap<In,true>*>(val)->value ) {
    tree->Branch(obr.name.c_str(), &out);
  }
  virtual void copy() {
    out.assign(in->begin(),in->end());
  }
};

branch_pipe_base*
make_branch_pipe(value_ptr::pointer val, TTree* tree,
                 const branch& ibr, const branch& obr)
{
  #define CASE_O(r, data, elem) \
    case branch:: BOOST_PP_SEQ_ELEM(0,elem): \
      return new branch_pipe< \
        BOOST_PP_SEQ_ELEM(1,data), \
        BOOST_PP_SEQ_ELEM(1,elem), \
        BOOST_PP_SEQ_ELEM(0,data) >(val,tree,obr); break;

  #define CASE_I(r, data, elem) \
    case branch:: BOOST_PP_SEQ_ELEM(0,elem): switch (obr.type) { \
      PP_DEFER(PP_SEQ_FOR_EACH_R_ID)()(r, CASE_O, \
        (data)(BOOST_PP_SEQ_ELEM(1,elem)), ROOT_TYPE_SEQ) \
    }; break;

  if (!ibr.is_array) {
    switch (ibr.type) {
      BOOST_PP_EXPAND(BOOST_PP_SEQ_FOR_EACH(CASE_I, false, ROOT_TYPE_SEQ))
    }
  } else {
    switch (ibr.type) {
      BOOST_PP_EXPAND(BOOST_PP_SEQ_FOR_EACH(CASE_I, true, ROOT_TYPE_SEQ))
    }
  }
  #undef CASE_O
  #undef CASE_I
  return nullptr; // actually never happens
}

int main(int argc, const char* argv[])
{
  flatntuple_options opt;
  flatntuple_config config;
  try {
    if (opt.parse(argc, argv)) return 1;
    config.parse(opt.config);
  } catch (exception& e) {
    cerr <<"\033[31mError in "<<argv[0]<<": "<<e.what()<<"\033[0m"<< endl;
    return 1;
  }

  cout << "Tree: " << config.itree << " -> " << config.otree << endl;
  for (const auto& b : config.branches) {
    cout << branch::type_str(b[0].type) << " " << b[0].name
         << " -> "
         << branch::type_str(b[1].type) << " " << b[1].name << endl;
  }

  TChain ch(config.itree.c_str());
  for (const auto& f : opt.input) ch.Add(f.c_str());

  TFile* fout = new TFile(opt.output.c_str(),"recreate");
  if (fout->IsZombie()) return 1;
  cout << "Output file: " << fout->GetName() << endl;
  TTree* tout = new TTree(config.otree.c_str(),"");

  TTreeReader reader(&ch);
  unordered_map<string,value_ptr> values;
  vector<unique_ptr<branch_pipe_base>> pipes;

  for (const auto& br : config.branches) {
    auto empl = values.emplace(std::piecewise_construct,
      forward_as_tuple(get<0>(br).name),
      forward_as_tuple(make_value(reader,get<0>(br)))
    );
    pipes.emplace_back( make_branch_pipe(
      empl.first->second.get(), tout, get<0>(br), get<1>(br) ) );
  }

  using tc = timed_counter<Long64_t>;
  for (tc ent(reader.GetEntries(true)); reader.Next(); ++ent) {
    for (const auto& p : pipes) p->copy();
    tout->Fill();
  }

  fout->Write(0,TObject::kOverwrite);
  fout->Close();
  delete fout;

  return 0;
}
