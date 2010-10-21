#include <vector>
#include <iostream>

#include "solver.hpp"
#include "cons.hpp"
#include "test.hpp"

using namespace std;

namespace {
  void alldiff01()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(3, 1, 2);
    MUST_BE_UNSAT(post_alldiff(s, x));
  }
  REGISTER_TEST(alldiff01);

  void alldiff02()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(3, 1, 3);
    post_alldiff(s, x);
  }
  REGISTER_TEST(alldiff02);

  // Hall interval
  void alldiff03()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(3, 1, 3);
    post_alldiff(s, x);

    vec<Lit> exp;
    exp.push(x[0].r_leq(s, 2));
    exp.push(x[1].r_leq(s, 2));
    exp.push(x[2].r_leq(s, 2));

    s.newDecisionLevel();
    x[0].remove(s, 3, NO_REASON);
    x[1].remove(s, 3, NO_REASON);
    x[2].remove(s, 3, NO_REASON);
    Clause *c = s.propagate();
    assert_clause_exact(c, exp);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff03);

  // Hall set
  void alldiff04()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(3, 1, 3);
    post_alldiff(s, x);

    vec<Lit> exp;
    exp.push(x[0].r_neq(s, 2));
    exp.push(x[1].r_neq(s, 2));
    exp.push(x[2].r_neq(s, 2));
    exp.push(x[0].r_max(s));
    exp.push(x[1].r_max(s));
    exp.push(x[2].r_max(s));

    s.newDecisionLevel();
    x[0].remove(s, 2, NO_REASON);
    x[1].remove(s, 2, NO_REASON);
    x[2].remove(s, 2, NO_REASON);
    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, exp);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff04);

  // Hall set and some unrelated stuff
  void alldiff05()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(8, 1, 10);
    post_alldiff(s, x);

    vec<Lit> exp;
    exp.push(x[2].r_neq(s, 5));
    exp.push(x[3].r_neq(s, 5));
    exp.push(x[4].r_neq(s, 5));
    exp.push(x[2].r_leq(s, 6));
    exp.push(x[3].r_leq(s, 6));
    exp.push(x[4].r_leq(s, 6));
    exp.push(x[2].r_geq(s, 4));
    exp.push(x[3].r_geq(s, 4));
    exp.push(x[4].r_geq(s, 4));

    s.newDecisionLevel();
    for(int i = 2; i != 5; ++i) {
      x[i].setmin(s, 4, NO_REASON);
      x[i].setmax(s, 6, NO_REASON);
      x[i].remove(s, 5, NO_REASON);
    }
    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, exp);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff05);
}

void alldiff_test()
{
  cerr << "alldifferent tests\n";
  the_test_container().run();
}
