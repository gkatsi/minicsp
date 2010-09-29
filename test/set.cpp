#include <iostream>
#include "test.hpp"
#include "solver.hpp"
#include "cons.hpp"
#include "setcons.hpp"

namespace {
  void encoding01()
  {
    Solver s;
    setvar x = s.newSetVar(1, 3);
    int numsol = 0;
    bool next;
    do {
      next = s.solve();
      if( next ) {
        ++numsol;
        s.excludeLast();
      }
    } while(next);
    assert(numsol == 8);
  }
  REGISTER_TEST(encoding01);

  void setdiff01()
  {
    Solver s;
    setvar A = s.newSetVar(1, 7);
    setvar B = s.newSetVar(2, 6);
    setvar C = s.newSetVar(0, 5);
    post_setdiff(s, A, B, C);
    assert( !C.includes(s, 0) );
    assert( !A.includes(s, 7) );
  }
  REGISTER_TEST(setdiff01);
}


void set_test()
{
  std::cerr << "set tests\n";

  the_test_container().run();
}
