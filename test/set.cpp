#include <iostream>
#include "test.hpp"
#include "solver.hpp"
#include "cons.hpp"

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
}


void set_test()
{
  std::cerr << "set tests\n";

  the_test_container().run();
}
