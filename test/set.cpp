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
    assert_num_solutions(s, 8);
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

  // a couple of tests from flatzinc on which it initially barfed
  void setdiff02()
  {
    Solver s;
    setvar A = s.newSetVar(1, 3);
    setvar B = s.newSetVar(1, 5);
    setvar C = s.newSetVar(2, 2);
    for(int i = 1; i <= 3; ++i)
      A.include(s, i, NO_REASON);
    B.include(s, 1, NO_REASON);
    B.include(s, 3, NO_REASON);
    B.include(s, 5, NO_REASON);
    B.exclude(s, 2, NO_REASON);
    B.exclude(s, 4, NO_REASON);
    post_setdiff(s, A, B, C);
  }
  REGISTER_TEST(setdiff02);

  void setdiff03()
  {
    Solver s;
    setvar A = s.newSetVar(1, 3);
    setvar B = s.newSetVar(0, 1);
    setvar C = s.newSetVar(0, 4);
    post_setdiff(s, A, B, C);

    s.newDecisionLevel();
    A.include(s, 3, NO_REASON);
    assert( !s.propagate() );
    assert( C.includes(s, 3) );
    s.cancelUntil(0);

    assert_num_solutions(s, 32);
  }
  REGISTER_TEST(setdiff03);

  void seteq01()
  {
    Solver s;
    setvar A = s.newSetVar(0, 3);
    setvar B = s.newSetVar(1, 4);
    post_seteq(s, A, B);
    assert_num_solutions(s, 8);
  }
  REGISTER_TEST(seteq01);

  void seteq02()
  {
    Solver s;
    setvar A = s.newSetVar(0, 3);
    setvar B = s.newSetVar(1, 4);
    post_seteq(s, A, B);

    A.card(s).setmax(s, 2, NO_REASON);

    s.newDecisionLevel();
    A.include(s, 1, NO_REASON);
    assert(!s.propagate());
    assert( B.includes(s, 1) );

    s.newDecisionLevel();
    A.exclude(s, 2, NO_REASON);
    assert(!s.propagate());
    assert( B.excludes(s, 2));
    s.cancelUntil(0);

    assert_num_solutions(s, 7);
  }
  REGISTER_TEST(seteq02);

  void setneq01()
  {
    Solver s;
    setvar A = s.newSetVar(0,0);
    setvar B = s.newSetVar(0,0);
    post_setneq(s, A, B);

    s.newDecisionLevel();
    A.exclude(s, 0, NO_REASON);
    B.exclude(s, 0, NO_REASON);
    assert( s.propagate() );
    s.cancelUntil(0);

    s.newDecisionLevel();
    A.include(s, 0, NO_REASON);
    B.include(s, 0, NO_REASON);
    assert( s.propagate() );
    s.cancelUntil(0);

    assert_num_solutions(s, 2);
  }
  REGISTER_TEST(setneq01);

  void setneq02()
  {
    Solver s;
    setvar A = s.newSetVar(3,4);
    setvar B = s.newSetVar(2,3);
    post_setneq(s, A, B);

    s.newDecisionLevel();
    A.include(s, 3, NO_REASON);
    A.exclude(s, 4, NO_REASON);
    B.exclude(s, 2, NO_REASON);
    assert( !s.propagate() );
    assert( B.excludes(s, 3) );
    s.cancelUntil(0);

    s.newDecisionLevel();
    A.include(s, 3, NO_REASON);
    assert( !s.propagate() );
    assert( !B.excludes(s, 3) );

    s.newDecisionLevel();
    B.exclude(s, 2, NO_REASON);
    A.exclude(s, 4, NO_REASON);
    assert( !s.propagate() );
    assert( B.excludes(s, 3) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(setneq02);

  void setneq03()
  {
    Solver s;
    setvar A = s.newSetVar(3,5);
    setvar B = s.newSetVar(2,4);
    post_setneq(s, A, B);

    assert_num_solutions(s, 60);
  }
  REGISTER_TEST(setneq03);
}


void set_test()
{
  std::cerr << "set tests\n";

  the_test_container().run();
}
