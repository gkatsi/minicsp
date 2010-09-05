#include <iostream>
#include "solver.hpp"
#include "cons.hpp"

using namespace std;

int main(int argc, char *argv[])
{
  if( argc != 2 ) {
    cerr << "usage " << argv[0] << " <# marks>\n";
    return 1;
  }

  size_t m = atoi(argv[1]);
  int l = m*m;

  Solver s;
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

  bool sol = false, next = false;
  do {
    next = s.solve();
    sol = sol || next;
    if(next) {
      for(size_t i = 0; i != m; ++i)
        cout << s.cspModelValue(x[i]) << ' ';
      cout << "\n";
      int len = s.cspModelValue(x[m-1]);
      x[m-1].setmax(s, len-1, NO_REASON);
    }
  } while(next);

  cout << s.conflicts << " conflicts\n";
  if( !sol ) {
    cout << "unsat\n";
    return 0;
  }

  return 0;
}
