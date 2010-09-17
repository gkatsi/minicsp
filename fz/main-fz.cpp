#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <iomanip>

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
  FlatZinc::FlatZincModel *fm;
  try {
    fm = parse(args.front(), s, p);
  } catch (unsat& e) {
    cout << setw(5) << setfill('=') << '='
         << "UNSATISFIABLE" << setw(5) << '=' << "\n";
  }
  if( !fm ) return 0;

  fm->run(cout , p);
  delete fm;

  return 0;
}
