#include <iostream>

#include "Solver.h"
#include "Cons.h"

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
}

void le_test()
{
  cerr << "le tests\n";

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
  test04();
  cerr << "OK" << endl;
}
