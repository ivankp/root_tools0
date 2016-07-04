// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include "timed_counter2.hh"

using namespace std;

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

struct br_var {
  union {
    Int_t     I;
    UInt_t    i;
    Float_t   F;
    Double_t  D;
    Long64_t  L;
    ULong64_t l;
    Bool_t    O;
    Char_t    B;
    UChar_t   b;
    Short_t   S;
    UShort_t  s;
  } x;
  enum type_char : char {
    I = 'I', i = 'i', F = 'F', D = 'D', L = 'L', l = 'l', O = 'O',
    B = 'B', b = 'b', S = 'S', s = 's'
  } type;

  void set_type(const string& str) {
    if      (str=="Int_t"    ) type = I;
    else if (str=="UInt_t"   ) type = i;
    else if (str=="Float_t"  ) type = F;
    else if (str=="Double_t" ) type = D;
    else if (str=="Long64_t" ) type = L;
    else if (str=="ULong64_t") type = l;
    else if (str=="Bool_t"   ) type = O;
    else if (str=="Char_t"   ) type = B;
    else if (str=="UChar_t"  ) type = b;
    else if (str=="Short_t"  ) type = S;
    else if (str=="UShort_t" ) type = s;
    else throw runtime_error("Unexpected branch type "+str);
  }

  const char* get_type() const noexcept {
    #define var_type_case(c,T) case c: return #T;
    switch (type) {
      var_type_case(I,Int_t)
      var_type_case(i,UInt_t)
      var_type_case(F,Float_t)
      var_type_case(D,Double_t)
      var_type_case(L,Long64_t)
      var_type_case(l,ULong64_t)
      var_type_case(O,Bool_t)
      var_type_case(B,Char_t)
      var_type_case(b,UChar_t)
      var_type_case(S,Short_t)
      var_type_case(s,UShort_t)
      default: return "";
    }
    #undef var_type_case
  }

  void set_val(void* ptr, bool is_ptr) noexcept {
    #define var_type_case(c,T) \
      case c: x.c = *(is_ptr ? *(T**)ptr : (T*)ptr); break;
    switch (type) {
      var_type_case(I,Int_t)
      var_type_case(i,UInt_t)
      var_type_case(F,Float_t)
      var_type_case(D,Double_t)
      var_type_case(L,Long64_t)
      var_type_case(l,ULong64_t)
      var_type_case(O,Bool_t)
      var_type_case(B,Char_t)
      var_type_case(b,UChar_t)
      var_type_case(S,Short_t)
      var_type_case(s,UShort_t)
    }
    #undef var_type_case
  }

  void branch(TTree* tree, const string& br) noexcept {
    #define var_type_case(c) \
      case c: tree->Branch(br.c_str(), &x.c, (br + "/" #c).c_str()); break;
    switch (type) {
      var_type_case(I)
      var_type_case(i)
      var_type_case(F)
      var_type_case(D)
      var_type_case(L)
      var_type_case(l)
      var_type_case(O)
      var_type_case(B)
      var_type_case(b)
      var_type_case(S)
      var_type_case(s)
    }
    #undef var_type_case
  }
};

class TTreeReaderValuePipe: public ROOT::Internal::TTreeReaderValueBase {
  br_var val;
public:
  TTreeReaderValuePipe(
    TTreeReader& tr, const char* type, const char* ibr,
    TTree* tree, const string& obr
  ): TTreeReaderValueBase(&tr, ibr, TDictionary::GetDictionary(type))
  {
    val.set_type(type);
    val.branch(tree,obr);
  }

  void Get() { val.set_val(GetAddress(),fProxy->IsaPointer()); }

  virtual const char* GetDerivedTypeName() const {
    return val.get_type();
  }
};

int main(int argc, char* argv[])
{
  if (argc!=6 && (argc-5)%3) {
    cout << "usage: " << argv[0]
         << "\n  input_file.root [input_tree] [event_range]"
            "\n  output_file.root output_tree"
            "\n  branch_list_file"
            "\nor"
            "\n  branch_type old_name new_name ..." << endl;
    return 0;
  }

  TFile* fin = new TFile(argv[1],"read");
  if (fin->IsZombie()) return 1;
  cout << "Input file: " << fin->GetName() << endl;
  TTreeReader reader(argv[2], fin);

  TFile* fout = new TFile(argv[3],"recreate");
  if (fout->IsZombie()) return 1;
  cout << "Output file: " << fout->GetName() << endl;
  TTree* tout = new TTree(argv[4],"");

  vector<TTreeReaderValuePipe> pipes;
  if (argc==6) {

  } else {
    pipes.reserve((argc-5)/3);
    for (int i=5; i<argc; i+=3)
      pipes.emplace_back(reader,argv[i],argv[i+1],tout,argv[i+2]);
  }

  // const auto nent = tin->GetEntries();
  // for (timed_counter<Long64_t> ent; ent<nent; ++ent) {
  for (timed_counter<Long64_t> ent(reader.GetEntries(true)); reader.Next(); ++ent)
  {
    for (auto& p : pipes) p.Get();
    tout->Fill();
  }

  fout->Write(0,TObject::kOverwrite);
  fout->Close();
  delete fout;

  fin->Close();
  delete fin;

  return 0;
}
