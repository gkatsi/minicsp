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

  // from nina's thesis
  void nvalue02()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(6, 1, 6);
    cspvar N = s.newCSPVar(1, 6);

    post_atmostnvalue(s, x, N);

    x[1].assign(s, 2, NO_REASON);
    x[2].assign(s, 2, NO_REASON);
    x[3].setmin(s, 2, NO_REASON);
    x[3].setmax(s, 4, NO_REASON);
    x[4].setmin(s, 4, NO_REASON);
    x[4].setmax(s, 5, NO_REASON);
    x[5].setmin(s, 4, NO_REASON);
    x[5].setmax(s, 5, NO_REASON);

    s.propagate();

    assert(N.min(s) == 2);

    s.newDecisionLevel();
    N.assign(s, 2, NO_REASON);
    s.propagate();
    assert(x[0].min(s) == 2);
    assert(x[0].max(s) == 5);
    s.cancelUntil(0);
  }
  REGISTER_TEST(nvalue02);
}

void nvalue_test()
{
  cerr << "nvalue tests\n";
  the_test_container().run();
}
