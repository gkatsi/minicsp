#include <iostream>
#include "test.hpp"
#include "solver.hpp"

using namespace std;

namespace {
  void check_consistency(Solver& s, cspvar x)
  {
    assert(x.indomain(s, x.min(s)));
    assert(x.indomain(s, x.max(s)));
    int xmin = x.min(s);
    --xmin;
    while( x.eqi(s, xmin) != var_Undef ) {
      Var eqi = x.eqi(s, xmin);
      Var leqi = x.leqi(s, xmin);
      assert( s.value( eqi ) == l_False );
      assert( s.value( leqi ) == l_False );
      --xmin;
    }
    int xmax = x.max(s);
    assert( s.value( x.leqi(s, xmax) ) == l_True );
    ++xmax;
    while( x.eqi(s, xmax) != var_Undef ) {
      Var eqi = x.eqi(s, xmax);
      Var leqi = x.leqi(s, xmax);
      assert( s.value( eqi ) == l_False );
      assert( s.value( leqi ) == l_True );
      ++xmax;
    }
  }

  void test01()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    x.setmin(s, 7, (Clause*)0L);
    assert( x.min(s) == 7 );
    assert( x.indomain(s, 7) );
    assert( !x.indomain(s, 6) );
    assert( !x.indomain(s, 5) );
    assert( x.domsize(s) == 4 );
    check_consistency(s,x);

    x.setmin(s, 9, (Clause*)0L);
    assert(x.min(s) == 9 );
    check_consistency(s,x);
  }
  REGISTER_TEST(test01);

  void test02()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    x.remove(s, 6, (Clause*)0L);
    x.setmin(s, 6, (Clause*)0L);
    assert( x.min(s) == 7 );
    assert( x.indomain(s, 7) );
    assert( !x.indomain(s, 6) );
    assert( !x.indomain(s, 5) );
    assert( x.domsize(s) == 4 );
    check_consistency(s,x);
  }
  REGISTER_TEST(test02);

  void test03()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    x.setmax(s, 7, (Clause*)0L);
    assert( x.max(s) == 7 );
    assert( x.indomain(s, 7) );
    assert( !x.indomain(s, 8) );
    assert( !x.indomain(s, 9) );
    assert( x.domsize(s) == 3 );
    check_consistency(s,x);

    x.setmax(s, 6, (Clause*)0L);
    assert( x.max(s) == 6 );
    check_consistency(s,x);
  }
  REGISTER_TEST(test03);

  void test04()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    x.remove(s, 7, (Clause*)0L);
    x.setmax(s, 7, (Clause*)0L);
    assert( x.max(s) == 6 );
    assert( x.indomain(s, 6) );
    assert( !x.indomain(s, 7) );
    assert( !x.indomain(s, 8) );
    assert( !x.indomain(s, 9) );
    assert( x.domsize(s) == 2 );
    check_consistency(s,x);
  }
  REGISTER_TEST(test04);

  void test05()
  {
    Solver s;
    cspvar x = s.newCSPVar(-5, 5);
    x.assign(s, 1, (Clause*)0L);
    assert(x.max(s) == 1);
    assert(x.min(s) == 1);
    for( int i = -5; i != 6; ++i) {
      if( i != 1 )
        assert( !x.indomain(s, i) );
      else
        assert( x.indomain(s, i) );
    }
    assert(x.domsize(s) == 1);
    check_consistency(s,x);
  }
  REGISTER_TEST(test05);

  void test06()
  {
    Solver s;
    cspvar x = s.newCSPVar( -5, 5 );
    x.remove(s, -5, (Clause*)0L);
    assert(x.min(s) == -4);
    assert( x.domsize(s) == 10 );
    x.remove(s, 5, (Clause*)0L);
    assert(x.max(s) == 4);
    assert( x.domsize(s) == 9 );
    check_consistency(s,x);
  }
  REGISTER_TEST(test06);

  void test07()
  {
    Solver s;
    cspvar x = s.newCSPVar(1, 5);
    x.setmin(s, 3, NO_REASON);
    x.setmax(s, 3, NO_REASON);
    assert( s.value(x.eqi(s, 3)) == l_True );
    check_consistency(s,x);
  }
  REGISTER_TEST(test07);

  // do something twice
  void test08()
  {
    Solver s;
    cspvar x = s.newCSPVar(1, 20);
    x.setmin(s, 3, NO_REASON);
    x.setmin(s, 3, NO_REASON);

    x.setmax(s, 15, NO_REASON);
    x.setmax(s, 15, NO_REASON);

    x.remove(s, 10, NO_REASON);
    x.remove(s, 10, NO_REASON);

    x.assign(s, 9, NO_REASON);
    x.assign(s, 9, NO_REASON);
    check_consistency(s,x);
  }
  REGISTER_TEST(test08);

  // assign to a value k when some values p,q s.t. k<p<=ub and
  // lb<=q<k are already pruned
  void test09()
  {
    Solver s;
    cspvar x = s.newCSPVar(1, 20);

    x.remove(s, 7, NO_REASON);
    x.remove(s, 9, NO_REASON);
    x.remove(s, 5, NO_REASON);
    x.remove(s, 11, NO_REASON);
    x.assign(s, 8, NO_REASON);
    check_consistency(s,x);
  }
  REGISTER_TEST(test09);

  // binary domains.
  void test10()
  {
    Solver s;
    cspvar x = s.newCSPVar(0,1);
    cspvar y = s.newCSPVar(0,1);

    s.newDecisionLevel();
    x.assign(s, 0, NO_REASON);
    y.assign(s, 1, NO_REASON);

    s.cancelUntil(0);
    x.setmax(s, 0, NO_REASON);
    y.setmin(s, 1, NO_REASON);
    check_consistency(s,x);
  }
  REGISTER_TEST(test10);

  // unary
  void test11()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 5);
    assert( !x.assign(s, 5, NO_REASON) );
    assert( !x.setmax(s, 5, NO_REASON) );
    assert( !x.setmin(s, 5, NO_REASON) );
    assert( x.min(s) == 5 );
    assert( x.max(s) == 5 );
    assert( x.indomain(s, 5) );

    MUST_BE_UNSAT( x.remove(s, 5, NO_REASON) );
  }
  REGISTER_TEST(test11);

  // really basic stuff that should have been tested from the
  // beginning :(
  void test12()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);

    assert(!x.indomain(s, 1));
    assert(!x.indomain(s, 20));
    MUST_BE_UNSAT( x.assign(s, 15, NO_REASON) );
    MUST_BE_UNSAT( x.setmin(s, 15, NO_REASON) );
    MUST_BE_UNSAT( x.setmax(s, 1, NO_REASON) );

    Clause *c = (Clause*)0x12345678;
    assert( x.assign(s, 15, c) );
    assert( x.setmin(s, 15, c) );
    assert( x.setmax(s, 1, c) );
  }
  REGISTER_TEST(test12);
}

void dom_test()
{
  cerr << "dom tests\n";
  the_test_container().run();
}
