#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <iomanip>

#include "flatzinc.hpp"
#include "solver.hpp"
#include "cmdline.hpp"
#include "utils.hpp"

using namespace std;

int main(int argc, char *argv[])
{
  list<string> args(argv+1, argv+argc);

  if(args.empty()) {
    cout << "usage: minicsp-fz [options] input.fzn";
    return 1;
  }

  Solver s;

  cmdline::parse_solver_options(s, args);
  bool stat = cmdline::has_option(args, "--stat");
  bool maint = cmdline::has_option(args, "--maint");
  setup_signal_handlers(&s);

  double cpu_time = cpuTime();

  FlatZinc::Printer p;
  FlatZinc::FlatZincModel *fm = 0L;
  if( maint ) // do not catch exceptions
    fm = parse(args.back(), s, p);
  else try {
      fm = parse(args.back(), s, p);
    } catch (unsat& e) {
      cout << setw(5) << setfill('=') << '='
           << "UNSATISFIABLE" << setw(5) << '=' << "\n";
    }
  if( !fm ) return 0;

  double parse_time = cpuTime() - cpu_time;

  fm->findall = cmdline::has_option(args, "--all");

  fm->run(cout , p);
  delete fm;

  if( stat ) {
    printStats(s, "%% ");
    reportf("%sParse time            : %g s\n", "%% ", parse_time);
  }

  return 0;
}
