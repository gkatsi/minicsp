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

  void test07()
  {
    Solver s;
    cspvar x = s.newCSPVar(1, 5);
    x.setmin(s, 3, NO_REASON);
    x.setmax(s, 3, NO_REASON);
    assert( s.value(x.eqi(s, 3)) == l_True );
    check_consistency(s,x);
  }

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
}

void dom_test()
{
  cerr << "dom tests\n";

  cerr << "test01..." << flush;
  test01();
  cerr << "OK\n";

  cerr << "test02..." << flush;
  test02();
  cerr << "OK\n";

  cerr << "test03..." << flush;
  test03();
  cerr << "OK\n";

  cerr << "test04..." << flush;
  test04();
  cerr << "OK\n";

  cerr << "test05..." << flush;
  test05();
  cerr << "OK\n";

  cerr << "test06..." << flush;
  test06();
  cerr << "OK\n";

  cerr << "test07..." << flush;
  test07();
  cerr << "OK\n";

  cerr << "test08..." << flush;
  test08();
  cerr << "OK\n";

  cerr << "test09..." << flush;
  test09();
  cerr << "OK\n";

  cerr << "test10..." << flush;
  test10();
  cerr << "OK\n";
}
