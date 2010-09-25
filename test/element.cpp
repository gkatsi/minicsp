#include <vector>
#include <iostream>

#include "solver.hpp"
#include "cons.hpp"
#include "test.hpp"

using namespace std;

namespace {

  // prune in I
  void element01()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(0, 9);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X);
    s.propagate();

    s.newDecisionLevel();
    X[5].setmax(s, 4, NO_REASON);
    s.propagate();
    assert( !I.indomain(s, 5) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(element01);

  // prune in R because I is assigned
  void element02()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(0, 9);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X);
    s.propagate();

    for(int i = 0; i != 10; ++i)
      X[i].remove(s, i+1, NO_REASON);
    s.propagate();
    for(int i = 4; i != 10; ++i) {
      s.newDecisionLevel();
      I.assign(s, i, NO_REASON);
      s.propagate();
      assert( !R.indomain(s, i+1) );
      s.cancelUntil(0);
    }
  }
  REGISTER_TEST(element02);

  // prune in R because the value has been removed from all X[i]
  // s.t. i in D(I)
  void element03()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(0, 9);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X);
    s.propagate();

    s.newDecisionLevel();
    for(int i = 0; i != 10; ++i)
      X[i].remove(s, 8, NO_REASON);
    s.propagate();
    assert(!R.indomain(s, 8));
    s.cancelUntil(0);
  }
  REGISTER_TEST(element03);

  // Prune in X after assigning I
  void element04()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(0, 9);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X);
    s.propagate();

    s.newDecisionLevel();
    I.assign(s, 4, NO_REASON);
    R.remove(s, 8, NO_REASON);
    s.propagate();
    assert( !X[4].indomain(s, 8) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(element04);

  // unsat
  void element05()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(0, 9);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X);
    s.propagate();

    s.newDecisionLevel();
    for(int i = 0; i != 9; ++i)
      X[i].remove(s, 8, NO_REASON);
    I.remove(s, 9, NO_REASON);
    R.assign(s, 8, NO_REASON);
    assert(s.propagate());
    s.cancelUntil(0);
  }
  REGISTER_TEST(element05);

  // I is a unary var
  void element06()
  {
    Solver s;
    cspvar R = s.newCSPVar(3, 7);
    cspvar I = s.newCSPVar(2, 2);
    vector<cspvar> X = s.newCSPVarArray(4, 1, 8);
    for(int i = 0; i != 4; ++i) {
      X[i].setmin(s, 1+2*i, NO_REASON);
      X[i].setmax(s, 2+2*i, NO_REASON);
    }
    post_element(s, R, I, X);

    assert( !s.propagate() );
    assert( R.min(s) == 5 );
    assert( R.max(s) == 6 );
  }
  REGISTER_TEST(element06);

  // Prune in X after assigning I, offset=1
  void element07()
  {
    Solver s;
    cspvar R = s.newCSPVar(5, 10);
    cspvar I = s.newCSPVar(1, 10);
    vector<cspvar> X = s.newCSPVarArray(10, 1, 10);
    post_element(s, R, I, X, 1);
    s.propagate();

    s.newDecisionLevel();
    I.assign(s, 5, NO_REASON);
    R.remove(s, 8, NO_REASON);
    s.propagate();
    assert( !X[4].indomain(s, 8) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(element07);

}

void element_test()
{
  cerr << "element tests\n";

  the_test_container().run();
}
