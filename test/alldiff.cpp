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
    assert_clause_exact(s, c, exp);
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
    assert_clause_exact(s, c, exp);
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
    assert_clause_exact(s, c, exp);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff05);

  // a couple of hall sets working together to produce unsat
  void alldiff06()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(8, 1, 10);
    post_alldiff(s, x);

    vec<Lit> exp;
    for(int i = 2; i != 4; ++i) {
      exp.push(x[i].r_neq(s, 5));
      exp.push(x[i].r_leq(s, 6));
      exp.push(x[i].r_geq(s, 4));
    }
    for(int i = 5; i != 8; ++i) {
      exp.push(x[i].r_geq(s, 3));
      exp.push(x[i].r_leq(s, 8));
      // unfortunate that we have to have 4 and 6 as well, since they
      // are unnecessary
      exp.push(x[i].r_neq(s, 4));
      exp.push(x[i].r_neq(s, 5));
      exp.push(x[i].r_neq(s, 6));
    }
    exp.push(x[1].r_geq(s, 3));
    exp.push(x[1].r_leq(s, 8));
    exp.push(x[1].r_neq(s, 5));

    s.newDecisionLevel();
    for(int i = 2; i != 4; ++i) {
      x[i].setmin(s, 4, NO_REASON);
      x[i].setmax(s, 6, NO_REASON);
      x[i].remove(s, 5, NO_REASON);
    }
    for(int i = 5; i != 8; ++i) {
      x[i].setmin(s, 3, NO_REASON);
      x[i].setmax(s, 8, NO_REASON);
      x[i].remove(s, 4, NO_REASON);
      x[i].remove(s, 5, NO_REASON);
      x[i].remove(s, 6, NO_REASON);
    }
    x[1].setmin(s, 3, NO_REASON);
    x[1].setmax(s, 8, NO_REASON);
    x[1].remove(s, 5, NO_REASON);
    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(s, c, exp);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff06);

  // pruning tests
  void alldiff07()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 1, 10);
    post_alldiff(s, x);

    s.newDecisionLevel();
    for(int i = 2; i != 4; ++i) {
      x[i].setmin(s, 4, NO_REASON);
      x[i].setmax(s, 6, NO_REASON);
      x[i].remove(s, 5, NO_REASON);
    }
    assert(!s.propagate());
    for(int i = 0; i != 5; ++i) {
      assert( i == 2 || i == 3 ||
              (!x[i].indomain(s, 4) &&
               !x[i].indomain(s, 6)));
    }
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff07);

  void alldiff08()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 1, 10);
    post_alldiff(s, x);

    s.newDecisionLevel();
    x[0].setmax(s, 2, NO_REASON);
    x[1].setmax(s, 2, NO_REASON);
    assert(!s.propagate());
    assert(x[2].min(s) == 3);
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff08);

  // an edge that crosses SCCs but is safe
  void alldiff09()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 1, 10);
    post_alldiff(s, x);

    s.newDecisionLevel();
    x[0].setmax(s, 3, NO_REASON);
    x[1].setmax(s, 2, NO_REASON);
    x[2].setmin(s, 3, NO_REASON);
    x[3].setmin(s, 3, NO_REASON);
    x[4].setmin(s, 3, NO_REASON);
    assert(!s.propagate());
    assert(x[0].indomain(s, 3));
    s.cancelUntil(0);
  }
  REGISTER_TEST(alldiff09);
}

void alldiff_test()
{
  cerr << "alldifferent tests\n";
  the_test_container().run();
}
