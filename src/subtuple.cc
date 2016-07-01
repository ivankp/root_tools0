// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

#include <TFile.h>
#include <TTree.h>

#include "timed_counter2.hh"

using namespace std;

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

// https://root.cern.ch/doc/master/classTTree.html
class branch_pipe {
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
public:
  branch_pipe(TTree* in, TTree* out,
       const string& type_name,
       const string& old_branch_name,
       const string& new_branch_name)
  {
    char c;
    if      (type_name=="Int_t"     || type_name=="I") c = 'I';
    else if (type_name=="UInt_t"    || type_name=="i") c = 'i';
    else if (type_name=="Float_t"   || type_name=="F") c = 'F';
    else if (type_name=="Double_t"  || type_name=="D") c = 'D';
    else if (type_name=="Long64_t"  || type_name=="L") c = 'L';
    else if (type_name=="ULong64_t" || type_name=="l") c = 'l';
    else if (type_name=="Bool_t"    || type_name=="O") c = 'O';
    else if (type_name=="Char_t"    || type_name=="B") c = 'B';
    else if (type_name=="UChar_t"   || type_name=="b") c = 'b';
    else if (type_name=="Short_t"   || type_name=="S") c = 'S';
    else if (type_name=="UShort_t"  || type_name=="s") c = 's';
    else throw runtime_error("Unknown branch type "+type_name);

    cout << type_name << ' '
         << old_branch_name << " > " << new_branch_name << endl;

    in->SetBranchStatus(old_branch_name.c_str(),1);
    in->AddBranchToCache(old_branch_name.c_str(),kTRUE);
    if        (c == 'I') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.I);
      out->Branch(new_branch_name.c_str(),&x.I,(new_branch_name+'/'+c).c_str());
    } else if (c == 'i') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.i);
      out->Branch(new_branch_name.c_str(),&x.i,(new_branch_name+'/'+c).c_str());
    } else if (c == 'F') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.F);
      out->Branch(new_branch_name.c_str(),&x.F,(new_branch_name+'/'+c).c_str());
    } else if (c == 'D') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.D);
      out->Branch(new_branch_name.c_str(),&x.D,(new_branch_name+'/'+c).c_str());
    } else if (c == 'L') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.L);
      out->Branch(new_branch_name.c_str(),&x.L,(new_branch_name+'/'+c).c_str());
    } else if (c == 'l') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.l);
      out->Branch(new_branch_name.c_str(),&x.l,(new_branch_name+'/'+c).c_str());
    } else if (c == 'O') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.O);
      out->Branch(new_branch_name.c_str(),&x.O,(new_branch_name+'/'+c).c_str());
    } else if (c == 'B') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.B);
      out->Branch(new_branch_name.c_str(),&x.B,(new_branch_name+'/'+c).c_str());
    } else if (c == 'b') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.b);
      out->Branch(new_branch_name.c_str(),&x.b,(new_branch_name+'/'+c).c_str());
    } else if (c == 'S') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.S);
      out->Branch(new_branch_name.c_str(),&x.S,(new_branch_name+'/'+c).c_str());
    } else if (c == 's') {
      in->SetBranchAddress(old_branch_name.c_str(),&x.s);
      out->Branch(new_branch_name.c_str(),&x.s,(new_branch_name+'/'+c).c_str());
    }
  }
};

int main(int argc, char* argv[])
{
  if (argc!=6 && (argc-5)%3) {
    cout << "usage: " << argv[0]
         << "\n  input_file.root input_tree"
            "\n  output_file.root output_tree"
            "\n  branch_list_file"
            "\nor"
            "\n  branch_type old_name new_name ..." << endl;
    return 0;
  }

  TFile* fin = new TFile(argv[1],"read");
  if (fin->IsZombie()) return 1;
  cout << "Input file: " << fin->GetName() << endl;
  TTree* tin = static_cast<TTree*>(fin->Get(argv[2]));
  if (!tin) {
    cerr << "No TTree " << argv[2]
         << " in file " << argv[1] << endl;
    return 1;
  }
  tin->SetCacheSize(10000000);

  TFile* fout = new TFile(argv[3],"recreate");
  if (fout->IsZombie()) return 1;
  cout << "Output file: " << fout->GetName() << endl;
  TTree* tout = new TTree(argv[4],tin->GetTitle());

  tin->SetBranchStatus("*",0);

  vector<branch_pipe> bp;
  if (argc==6) {

  } else {
    bp.reserve((argc-5)/3);
    for (int i=5; i<argc; i+=3)
      bp.emplace_back(tin,tout,argv[i],argv[i+1],argv[i+2]);
  }

  // const auto nent = tin->GetEntries();
  // for (timed_counter<Long64_t> ent; ent<nent; ++ent) {
  for (timed_counter<Long64_t> ent(tin->GetEntries()); ent.ok(); ++ent) {
    tin->GetEntry(ent);
    tout->Fill();
  }

  fout->Write(0,TObject::kOverwrite);
  fout->Close();
  delete fout;

  fin->Close();
  delete fin;

  return 0;
}
