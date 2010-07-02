#include <iostream>
#include "Test.h"
#include "Solver.h"

using namespace std;

namespace {
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
  }

  void test06()
  {
    Solver s;
    cspvar x = s.newCSPVar( -5, 5 );
    x.remove(s, -5, (Clause*)0L);
    assert(x.min(s) == -4);
    assert( x.domsize(s) == 10 );
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
}
