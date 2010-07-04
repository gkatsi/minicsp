#include <iostream>

#include "Solver.h"
#include "Cons.h"

using namespace std;

namespace {
  void test01()
  {
    Solver s;
    btptr p = s.alloc_backtrackable(sizeof(int));
    btptr p1 = s.alloc_backtrackable(sizeof(char));

    int & i = s.deref<int>(p);
    char & c = s.deref<char>(p1);

    i = 0;
    c = 1;

    s.newDecisionLevel();
    i = 1;
    c = 2;

    s.newDecisionLevel();
    i = 2;
    c = 3;

    s.cancelUntil(1);
    assert( i == 1 );
    assert( c == 2 );

    s.newDecisionLevel();
    i = 3;
    c = 4;

    s.newDecisionLevel();
    i = 4;
    c = 5;

    s.cancelUntil(2);
    assert(i == 3);
    assert(c == 4);

    s.cancelUntil(1);
    assert( i == 1 );
    assert( c == 2 );

    s.cancelUntil(0);
    assert( i == 0 );
    assert( c == 1 );
  }
}

void bt_test()
{
  cerr << "bt tests\n";

  cerr << "test 01 ... " << flush;
  test01();
  cerr << "OK" << endl;
}
