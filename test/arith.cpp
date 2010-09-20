#include <iostream>

#include "solver.hpp"
#include "cons.hpp"
#include "test.hpp"

using namespace std;

namespace {
  void test01()
  {
    Solver s;
    cspvar x = s.newCSPVar(2, 7);
    cspvar y = s.newCSPVar(0, 6);
    post_less(s, x, y, 0);
    s.propagate();
    assert( x.max(s) == 5 && y.min(s) == 3 );

    x.setmin(s, 4, NO_REASON);
    s.propagate();
    assert( y.min(s) == 5 );

    y.setmax(s, 5, NO_REASON);
    s.propagate();
    assert( x.max(s) == 4 );
  }

  void test02()
  {
    Solver s;
    cspvar x = s.newCSPVar(2, 7);
    cspvar y = s.newCSPVar(0, 6);
    post_leq(s, x, y, 0);
    s.propagate();
    assert(x.max(s) == 6 && y.min(s) == 2);

    x.setmin(s, 4, NO_REASON);
    s.propagate();
    assert(y.min(s) == 4);

    y.setmax(s, 5, NO_REASON);
    s.propagate();
    assert( x.max(s) == 5 );
  }

  void test03()
  {
    Solver s;
    cspvar x = s.newCSPVar(2, 7);
    cspvar y = s.newCSPVar(10, 16);
    post_less(s, x, y, -10);
    s.propagate();
    assert( x.max(s) == 5 && y.min(s) == 13 );

    x.setmin(s, 4, NO_REASON);
    s.propagate();
    assert( y.min(s) == 15 );

    y.setmax(s, 15, NO_REASON);
    s.propagate();
    assert( x.max(s) == 4 );
  }

  void test04()
  {
    Solver s;
    cspvar x = s.newCSPVar(2, 7);
    cspvar y = s.newCSPVar(-10, -4);
    post_less(s, x, y, 10);
    s.propagate();
    assert( x.max(s) == 5 && y.min(s) == -7 );

    x.setmin(s, 4, NO_REASON);
    s.propagate();
    assert( y.min(s) == -5 );

    y.setmax(s, -5, NO_REASON);
    s.propagate();
    assert( x.max(s) == 4 );
  }

  void test05()
  {
    Solver s;
    cspvar x = s.newCSPVar(0, 100);
    cspvar y = s.newCSPVar(0, 100);
    cspvar z = s.newCSPVar(0, 100);

    post_less(s, x, y, 0);
    post_less(s, y, z, 0);
    post_less(s, z, x, 0);
    Clause *c = s.propagate();
    assert(c);
  }

  void test06()
  {
    Solver s;
    cspvar x = s.newCSPVar(0, 10);
    cspvar y = s.newCSPVar(5, 15);
    post_neq(s, x, y, 0);
    Clause *c = s.propagate();
    assert(!c);

    s.newDecisionLevel();
    x.assign(s, 7, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( !y.indomain(s, 7) );
    s.cancelUntil(0);

    s.newDecisionLevel();
    x.assign(s, 4, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( y.domsize(s) == 11 );
    s.cancelUntil(0);

    s.newDecisionLevel();
    y.assign(s, 7, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( !x.indomain(s, 7) );
    s.cancelUntil(0);

    s.newDecisionLevel();
    y.assign(s, 14, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( x.domsize(s) == 11 );
    s.cancelUntil(0);

    cspvar z = s.newCSPVar(5, 7);
    cspvar w = s.newCSPVar(5, 7);
    z.assign(s, 5, NO_REASON);
    w.assign(s, 5, NO_REASON);
    bool caught=false;
    try {
      post_neq(s, z, w, 0);
    } catch(unsat) {
      caught = true;
    }
    assert(caught);
  }

  void test07()
  {
    Solver s;
    cspvar p = s.newCSPVar(5, 7);
    cspvar q = s.newCSPVar(3, 10);
    p.assign(s, 5, NO_REASON);
    post_neq(s, p, q, 0);
    assert( !q.indomain(s, 5) );
  }

  void test08()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    cspvar y = s.newCSPVar(15, 20);
    post_neq(s, x, y, -11);
    Clause *c = s.propagate();
    assert(!c);

    s.newDecisionLevel();
    x.assign(s, 7, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( !y.indomain(s, 18));
    s.cancelUntil(0);

    s.newDecisionLevel();
    y.assign(s, 19, NO_REASON);
    c = s.propagate();
    assert(!c);
    assert( !x.indomain(s, 8));
    s.cancelUntil(0);
  }

  // eq, c == 0
  void test09()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    cspvar y = s.newCSPVar(7, 12);
    post_eq(s, x, y, 0);
    assert(x.min(s) == 7);
    assert(y.max(s) == 10);

    s.newDecisionLevel();
    x.remove(s, 9, NO_REASON);
    y.remove(s, 8, NO_REASON);
    s.propagate();
    assert(!y.indomain(s, 9));
    assert(!x.indomain(s, 8));
    s.cancelUntil(0);

    x.assign(s, 10, NO_REASON);
    s.propagate();
    assert(y.min(s) == 10 && y.max(s) == 10);
  }

  // eq, c != 0
  void test10()
  {
    Solver s;
    cspvar x = s.newCSPVar(15, 20);
    cspvar y = s.newCSPVar(7, 12);
    post_eq(s, x, y, 10);
    assert(x.min(s) == 17);
    assert(y.max(s) == 10);

    s.newDecisionLevel();
    x.remove(s, 19, NO_REASON);
    y.remove(s, 8, NO_REASON);
    s.propagate();
    assert(!y.indomain(s, 9));
    assert(!x.indomain(s, 18));
    s.cancelUntil(0);

    x.assign(s, 20, NO_REASON);
    s.propagate();
    assert(y.min(s) == 10 && y.max(s) == 10);
  }

  // eq unsat
  void test11()
  {
    Solver s;
    cspvar x = s.newCSPVar(15, 20);
    cspvar y = s.newCSPVar(7, 12);
    bool caught = false;
    try {
      post_eq(s, x, y, 0);
    } catch( unsat ) {
      caught = true;
    }
    assert(caught);
  }

  // neg, c == 0
  void test12()
  {
    Solver s;
    cspvar x = s.newCSPVar(5, 10);
    cspvar y = s.newCSPVar(-12, -7);
    post_neg(s, x, y, 0);
    assert(x.min(s) == 7);
    assert(y.min(s) == -10);

    s.newDecisionLevel();
    x.remove(s, 9, NO_REASON);
    y.remove(s, -8, NO_REASON);
    s.propagate();
    assert(!y.indomain(s, -9));
    assert(!x.indomain(s, 8));
    s.cancelUntil(0);

    x.assign(s, 10, NO_REASON);
    s.propagate();
    assert(y.min(s) == -10 && y.max(s) == -10);
  }

  // neg, c != 0
  void test13()
  {
    Solver s;
    cspvar x = s.newCSPVar(15, 20);
    cspvar y = s.newCSPVar(-12, -7);
    post_neg(s, x, y, 10);
    assert(x.min(s) == 17);
    assert(y.min(s) == -10);

    s.newDecisionLevel();
    x.remove(s, 19, NO_REASON);
    y.remove(s, -8, NO_REASON);
    s.propagate();
    assert(!y.indomain(s, -9));
    assert(!x.indomain(s, 18));
    s.cancelUntil(0);

    x.assign(s, 20, NO_REASON);
    s.propagate();
    assert(y.min(s) == -10 && y.max(s) == -10);
  }

  // neg unsat
  void test14()
  {
    Solver s;
    cspvar x = s.newCSPVar(15, 20);
    cspvar y = s.newCSPVar(-12, -7);
    bool caught = false;
    try {
      post_neg(s, x, y, 0);
    } catch( unsat ) {
      caught = true;
    }
    assert(caught);
  }

  // abs, c == 0, x > 0
  // abs, c == 0, x < 0
  // abs, c == 0, min(x) < 0, max(x) > 0
  void abs01()
  {
    Solver s;
    cspvar x = s.newCSPVar(12, 20);
    cspvar y = s.newCSPVar(7, 15);
    post_abs(s, x, y, 0);
    assert( x.max(s) == 15 );
    assert( y.min(s) == 12 );

    cspvar x1 = s.newCSPVar(-20, -12);
    cspvar y1 = s.newCSPVar(7, 15);
    post_abs(s, x1, y1, 0);
    assert( x1.min(s) == -15);
    assert( y1.min(s) == 12);

    cspvar x2 = s.newCSPVar(-5, 7);
    cspvar y2 = s.newCSPVar(3, 8);

    post_abs(s, x2, y2, 0);
    for(int i = -2; i <= 2; ++i)
      assert(!x2.indomain(s, i));
    assert(y2.max(s) == 7);
    assert(y2.min(s) == 3);
    assert(x2.min(s) == -5);
    assert(x2.max(s) == 7);

    s.newDecisionLevel();
    assert(! x2.remove(s, 4, NO_REASON) );
    assert(! x2.remove(s, -4, NO_REASON) );
    assert(! s.propagate() );
    assert( !y2.indomain(s, 4) );
    s.cancelUntil(0);

    s.newDecisionLevel();
    assert(! y2.remove(s, 5, NO_REASON) );
    assert(! s.propagate() );
    assert( !x2.indomain(s, 5) );
    assert( !x2.indomain(s, -5) );
    s.cancelUntil(0);
  }

  // abs, c != 0, x > 0
  // abs, c != 0, x < 0
  // abs, c != 0, min(x) < 0, max(x) > 0
  void abs02()
  {
    Solver s;
    cspvar x = s.newCSPVar(12, 20);
    cspvar y = s.newCSPVar(-3, 5);
    post_abs(s, x, y, 10);
    assert( x.max(s) == 15 );
    assert( x.min(s) == 12 );
    assert( y.min(s) == 2 );
    assert( y.max(s) == 5 );

    cspvar x1 = s.newCSPVar(-20, -12);
    cspvar y1 = s.newCSPVar(-3, 5);
    post_abs(s, x1, y1, 10);
    assert( x1.min(s) == -15);
    assert( y1.min(s) == 2);

    cspvar x2 = s.newCSPVar(-5, 7);
    cspvar y2 = s.newCSPVar(-7, -2);

    post_abs(s, x2, y2, 10);
    for(int i = -2; i <= 2; ++i)
      assert(!x2.indomain(s, i));
    assert(y2.max(s) == -3);
    assert(y2.min(s) == -7);
    assert(x2.min(s) == -5);
    assert(x2.max(s) == 7);

    s.newDecisionLevel();
    assert(! x2.remove(s, 4, NO_REASON) );
    assert(! x2.remove(s, -4, NO_REASON) );
    assert(! s.propagate() );
    assert( !y2.indomain(s, -6) );
    s.cancelUntil(0);

    s.newDecisionLevel();
    assert(! y2.remove(s, -5, NO_REASON) );
    assert(! s.propagate() );
    assert( !x2.indomain(s, 5) );
    assert( !x2.indomain(s, -5) );
    s.cancelUntil(0);
  }

  // abs unsat by y < 0
  // abs unsat by |x| < y
  // abs unsat by |x| > y
  void abs03()
  {
    {
      Solver s;
      cspvar x = s.newCSPVar(5, 10);
      cspvar y = s.newCSPVar(-5, -3);
      MUST_BE_UNSAT(post_abs(s, x, y, 0));
    }

    {
      Solver s;
      cspvar x = s.newCSPVar(-10, 10);
      cspvar y = s.newCSPVar(15, 20);
      MUST_BE_UNSAT(post_abs(s, x, y, 0));
    }

    {
      Solver s;
      cspvar x = s.newCSPVar(-10, 10);
      cspvar y = s.newCSPVar(0, 2);
      for(int i = -2; i <= 2; ++i)
        x.remove(s, i, NO_REASON);
      MUST_BE_UNSAT(post_abs(s, x, y, 0));
    }

    {
      Solver s;
      cspvar x = s.newCSPVar(-10, -3);
      cspvar y = s.newCSPVar(0, 2);
      MUST_BE_UNSAT(post_abs(s, x, y, 0));
    }
  }
}

void le_test()
{
  cerr << "arithmetic relation tests\n";

  cerr << "test 01 ... " << flush;
  test01();
  cerr << "OK" << endl;

  cerr << "test 02 ... " << flush;
  test02();
  cerr << "OK" << endl;

  cerr << "test 03 ... " << flush;
  test03();
  cerr << "OK" << endl;

  cerr << "test 04 ... " << flush;
  test04();
  cerr << "OK" << endl;

  cerr << "test 05 ... " << flush;
  test05();
  cerr << "OK" << endl;

  cerr << "test 06 ... " << flush;
  test06();
  cerr << "OK" << endl;

  cerr << "test 07 ... " << flush;
  test07();
  cerr << "OK" << endl;

  cerr << "test 08 ... " << flush;
  test08();
  cerr << "OK" << endl;

  cerr << "test 09 ... " << flush;
  test09();
  cerr << "OK" << endl;

  cerr << "test 10 ... " << flush;
  test10();
  cerr << "OK" << endl;

  cerr << "test 11 ... " << flush;
  test11();
  cerr << "OK" << endl;

  cerr << "test 12 ... " << flush;
  test12();
  cerr << "OK" << endl;

  cerr << "test 13 ... " << flush;
  test13();
  cerr << "OK" << endl;

  cerr << "test 14 ... " << flush;
  test14();
  cerr << "OK" << endl;

  cerr << "abs 01 ... " << flush;
  abs01();
  cerr << "OK" << endl;

  cerr << "abs 02 ... " << flush;
  abs02();
  cerr << "OK" << endl;

  cerr << "abs 03 ... " << flush;
  abs03();
  cerr << "OK" << endl;
}
