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
}

void pb_test()
{
  std::cerr << "pb tests\n";

  std::cerr << "test01..." << std::flush;
  test01();
  std::cerr << "OK\n";

  std::cerr << "test02..." << std::flush;
  test02();
  std::cerr << "OK\n";

  std::cerr << "test03..." << std::flush;
  test03();
  std::cerr << "OK\n";

  std::cerr << "test04..." << std::flush;
  test04();
  std::cerr << "OK\n";

  std::cerr << "test05..." << std::flush;
  test05();
  std::cerr << "OK\n";

  std::cerr << "test06..." << std::flush;
  test06();
  std::cerr << "OK\n";

  std::cerr << "test07..." << std::flush;
  test07();
  std::cerr << "OK\n";

  std::cerr << "test08..." << std::flush;
  test08();
  std::cerr << "OK\n";
}
