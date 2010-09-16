#include <iostream>
#include <fstream>
#include <list>
#include <string>

#include "flatzinc.hpp"
#include "solver.hpp"

using namespace std;

int main(int argc, char *argv[])
{
  list<string> args(argv+1, argv+argc);

  if(args.empty()) {
    cout << "usage: minicsp-fz input.fzn";
    return 1;
  }

  Solver s;
  FlatZinc::Printer p;
  FlatZinc::FlatZincModel *fm = parse(args.front(), s, p);
  s.solve();
  fm->print(cout, p);
  delete fm;
}
