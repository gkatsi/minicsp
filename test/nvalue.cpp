#include <vector>
#include <iostream>

#include "solver.hpp"
#include "cons.hpp"
#include "test.hpp"

using namespace std;

namespace {
  void nvalue01()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(3, 1, 5);
    cspvar N = s.newCSPVar(0, 3);

    post_atmostnvalue(s, x, N);
    assert( N.min(s) == 1 );

    s.newDecisionLevel();
    x[0].setmin(s, 3, NO_REASON);
    x[1].setmax(s, 2, NO_REASON);
    s.propagate();
    assert( N.min(s) == 2 );
    s.cancelUntil(0);
  }
  REGISTER_TEST(nvalue01);
}

void nvalue_test()
{
  cerr << "nvalue tests\n";
  the_test_container().run();
}
