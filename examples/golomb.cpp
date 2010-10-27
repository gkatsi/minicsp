#include <iostream>
#include "solver.hpp"
#include "cons.hpp"
#include "cmdline.hpp"
#include "utils.hpp"

using namespace std;

int main(int argc, char *argv[])
{
  cmdline::arglist args(argv+1, argv+argc);
  Solver s;
  cmdline::parse_solver_options(s, args);
  bool stat = cmdline::has_option(args, "--stat");
  setup_signal_handlers(&s);

  if( args.empty() ) {
    cerr << "usage " << argv[0] << " [options] <# marks>\n";
    return 1;
  }

  size_t m = atoi(args.back().c_str());
  int l = m*m;

  vector<cspvar> x = s.newCSPVarArray(m, 0, l);

  for(size_t i = 1; i != m; ++i)
    for(size_t j = i+1; j != m; ++j) {
      cspvar diff = s.newCSPVar(0, l);
      x.push_back(diff);

      vector<cspvar> v(3);
      vector<int> c(3);
      v[0] = x[i]; c[0] = -1;
      v[1] = x[j]; c[1] = 1;
      v[2] = diff; c[2] = -1;
      post_lin_eq(s, v, c, 0);
    }

  for(size_t i = 0; i != m-1; ++i)
    post_less(s, x[i], x[i+1], 0);

  post_alldiff(s, x);
  x[0].assign(s, 0, NO_REASON);
  post_less(s, x[1], x.back(), 0);

  bool sol = false, next = false;
  int opt = l;
  do {
    next = s.solve();
    sol = sol || next;
    if(next) {
      cout << "solution ";
      for(size_t i = 0; i != m; ++i)
        cout << s.cspModelValue(x[i]) << ' ';
      cout << "\n";
      int len = s.cspModelValue(x[m-1]);
      opt = len;
      x[m-1].setmax(s, len-1, NO_REASON);
    }
  } while(next);

  cout << "optimal length " << opt << "\n";
  if( !sol ) {
    cout << "unsat\n";
    return 0;
  }

  if( stat )
    printStats(s);

  return 0;
}
