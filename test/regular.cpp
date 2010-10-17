#include <vector>
#include <iostream>

#include "solver.hpp"
#include "cons.hpp"
#include "test.hpp"

using namespace std;
using namespace regular;

namespace {
  void buildd(int da[][3], vector<transition>& d)
  {
    for(size_t i = 0; da[i][0] >= 0; ++i)
      d.push_back( transition(da[i][0], da[i][1], da[i][2]) );
  }

  // non-minimal DFA
  void regular01()
  {
    Solver s;
    int da[][3] = {
      {1, 0, 1},
      {1, 1, 2},
      {1, 2, 3},
      {2, 0, 2},
      {2, 1, 2},
      {2, 2, 2},
      {3, 0, 3},
      {3, 1, 3},
      {3, 2, 3},
      {-1, -1, -1}
    };
    vector<transition> d;
    set<int> f;
    buildd(da, d);
    f.insert(2);
    automaton a(d, 1, f);

    vector<cspvar> X = s.newCSPVarArray(5, 0, 3);
    post_regular(s, X, a, false);

    // symbol not in the language, fail
    s.newDecisionLevel();
    X[3].assign(s, 3, NO_REASON);
    assert( s.propagate() );
    s.cancelUntil(0);

    // bad assignments, fail
    s.newDecisionLevel();
    X[0].assign(s, 0, NO_REASON);
    X[1].assign(s, 2, NO_REASON);
    assert( s.propagate() );
    s.cancelUntil(0);

    // non-failing assignments, don't fail
    s.newDecisionLevel();
    X[0].assign(s, 0, NO_REASON);
    X[2].assign(s, 2, NO_REASON);
    assert( !s.propagate() );
    s.cancelUntil(0);

    // non-accepting complete assignment, fail
    s.newDecisionLevel();
    X[0].assign(s, 0, NO_REASON);
    X[1].assign(s, 0, NO_REASON);
    X[2].assign(s, 0, NO_REASON);
    X[3].assign(s, 0, NO_REASON);
    X[4].assign(s, 0, NO_REASON);
    assert( s.propagate() );
    s.cancelUntil(0);

    // accepting partial assignment, don't fail
    s.newDecisionLevel();
    X[0].assign(s, 0, NO_REASON);
    X[1].assign(s, 0, NO_REASON);
    X[2].assign(s, 1, NO_REASON);
    X[3].assign(s, 2, NO_REASON);
    assert( !s.propagate() );
    s.cancelUntil(0);

    // accepting complete assignment, don't fail
    s.newDecisionLevel();
    X[0].assign(s, 0, NO_REASON);
    X[1].assign(s, 0, NO_REASON);
    X[2].assign(s, 1, NO_REASON);
    X[3].assign(s, 2, NO_REASON);
    X[4].assign(s, 2, NO_REASON);
    assert( !s.propagate() );
    s.cancelUntil(0);
  }
  REGISTER_TEST(regular01);
}

void regular_test()
{
  cerr << "regular tests\n";
  the_test_container().run();
}
