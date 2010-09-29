#include <iostream>
#include "test.hpp"
#include "solver.hpp"
#include "cons.hpp"

using std::vector;

namespace {
  /* a or -b or -c
     ast: -a, b, c
     exp: fail, clause (a, -b, -c)
  */
  void test01()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ 1, -1, -1 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, -1);

    // assignment
    s.enqueue(~Lit(v[0]), 0L);
    s.enqueue(Lit(v[1]), 0L);
    s.enqueue(Lit(v[2]), 0L);

    // clause
    vec<Lit> expected;
    expected.push( Lit(v[0]) );
    expected.push( ~Lit(v[1]) );
    expected.push( ~Lit(v[2]) );

    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, expected);
  }
  REGISTER_TEST(test01);

  /* -2*a + b + c >= 0
     ast: a, -b
     exp: fail, clause (-a, b)
  */
  void test02()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ -2, 1, 1 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(Lit(v[0]), 0L);
    s.enqueue(~Lit(v[1]), 0L);

    // clause
    vec<Lit> expected;
    expected.push( ~Lit(v[0]) );
    expected.push( Lit(v[1]) );

    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, expected);
  }
  REGISTER_TEST(test02);

  /* -2*a + b + c >= 0
     ast: a
     exp: no fail
  */
  void test03()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ -2, 1, 1 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(Lit(v[0]), 0L);

    Clause *c = s.propagate();
    assert(!c);
  }
  REGISTER_TEST(test03);

  /* -100*a + 75*b + 75*c >= 0
     ast: a, b
     exp: no fail
  */
  void test04()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ -100, 75, 75 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(Lit(v[0]), 0L);
    s.enqueue(Lit(v[1]), 0L);

    Clause *c = s.propagate();
    assert(!c);
  }
  REGISTER_TEST(test04);

  /* -100*a + 75*b + 75*c >= 0
     ast: b, c
     exp: no fail
  */
  void test05()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ -100, 75, 75 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(Lit(v[1]), 0L);
    s.enqueue(Lit(v[2]), 0L);

    Clause *c = s.propagate();
    assert(!c);
  }
  REGISTER_TEST(test05);

  /* +100*a - 75*b - 75*c >= 0
     ast: b, c
     exp: fail, clause (-b, -c)
  */
  void test06()
  {
    Solver s;
    vector<Var> v(3);
    for(int i = 0; i != 3; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ 100, -75, -75 };
    vector<int> w(weights, weights+3);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(Lit(v[1]), 0L);
    s.enqueue(Lit(v[2]), 0L);

    // clause
    vec<Lit> expected;
    expected.push( ~Lit(v[1]) );
    expected.push( ~Lit(v[2]) );

    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, expected);
  }
  REGISTER_TEST(test06);

  /* +100*a - 75*b - 75*c + 10*d >= 0
     ast: -d, b, c
     exp: fail, clause (-b, -c)
     tests clauses minimality
  */
  void test07()
  {
    Solver s;
    vector<Var> v(4);
    for(int i = 0; i != 4; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ 100, -75, -75, 10 };
    vector<int> w(weights, weights+4);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(~Lit(v[3]), 0L);
    s.enqueue(Lit(v[1]), 0L);
    s.enqueue(Lit(v[2]), 0L);

    // clause
    vec<Lit> expected;
    expected.push( ~Lit(v[1]) );
    expected.push( ~Lit(v[2]) );

    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, expected);
  }
  REGISTER_TEST(test07);

  /* +10*a - 75*b - 75*c + 100*d >= 0
     ast: -a, b, c
     exp: fail, clause (-b, -c)
     tests clauses minimality and _svars sorting
  */
  void test08()
  {
    Solver s;
    vector<Var> v(4);
    for(int i = 0; i != 4; ++i)
      v[i] = s.newVar();

    // constraint
    int weights[]={ 10, -75, -75, 100 };
    vector<int> w(weights, weights+4);
    post_pb(s, v, w, 0);

    // assignment
    s.enqueue(~Lit(v[0]), 0L);
    s.enqueue(Lit(v[1]), 0L);
    s.enqueue(Lit(v[2]), 0L);

    // clause
    vec<Lit> expected;
    expected.push( ~Lit(v[1]) );
    expected.push( ~Lit(v[2]) );

    Clause *c = s.propagate();
    assert(c);
    assert_clause_exact(c, expected);
  }
  REGISTER_TEST(test08);

  /* 5*x1+5*x2 >= 0 ==> b = 1
     exp: b=1
   */
  void test09()
  {
    // test disabled for now because the pb constraint does not do
    // propagation
#if 0
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(2, 0, 1);
    cspvar b = s.newCSPVar(0, 1);
    vector<int> w(2);
    w[0] = 5;
    w[1] = 5;
    post_pb_right_imp_re(s, x, w, 0, b);
#endif
  }
  REGISTER_TEST(test09);

  /* pbvar, unit weights, c = 0 */
  void test10()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 0, 1);
    vector<int> w(5);
    for(int i = 0; i != 5; ++i) w[i] = 1;
    cspvar rhs = s.newCSPVar(-5, 10);
    post_pb(s, x, w, 0, rhs);
    assert( rhs.min(s) == 0 );
    assert( rhs.max(s) == 5 );

    s.newDecisionLevel();
    x[0].setmin(s, 1, NO_REASON);
    x[1].setmax(s, 0, NO_REASON);
    s.propagate();
    assert(rhs.min(s) == 1);
    assert(rhs.max(s) == 4);

    s.newDecisionLevel();
    rhs.setmax(s, 1, NO_REASON);
    s.propagate();
    for(int i = 2; i != 5; ++i)
      assert(x[i].max(s) == 0);
    s.cancelUntil(0);
  }
  REGISTER_TEST(test10);
}

void pb_test()
{
  std::cerr << "pb tests\n";

  the_test_container().run();
}
