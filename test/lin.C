#include <iostream>
#include "Test.h"
#include "Solver.h"
#include "Cons.h"

using namespace std;

namespace {
  void test01()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, -5, 5);
    vector<int> w(5);
    w[0] = 5;
    w[1] = -4;
    w[2] = 3;
    w[3] = -2;
    w[4] = 1;
    post_lin_leq(s, x, w, 0);
    s.propagate();
  }

  /* sum x1 .. x5 >= 24
     x1 .. x5 \in [3,5]

     result x1..x5 >= 4
   */
  void test02()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 3, 5);
    vector<int> w(5);
    w[0] = -1;
    w[1] = -1;
    w[2] = -1;
    w[3] = -1;
    w[4] = -1;
    post_lin_leq(s, x, w, 24);
    s.propagate();
     for(int i = 0; i != 5; ++i) {
      assert( x[i].min(s) == 4 );
    }
  }

  /* sum x1 .. x5 <= 20
     x1 .. x5 \in [3, 9]

     result x1..x5<=8
   */
  void test03()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 3, 9);
    vector<int> w(5);
    w[0] = 1;
    w[1] = 1;
    w[2] = 1;
    w[3] = 1;
    w[4] = 1;
    post_lin_leq(s, x, w, -20);
    s.propagate();
    for(int i = 0; i != 5; ++i) {
      assert( x[i].max(s) == 8 );
    }
  }

  /* sum x1...x4 - x5 <= 20
     x1 .. x5 \in [7,10]
     x6 \in [2, 10]

     result x5 >= 8
     x1..x4 <= 9
   */
  void test04()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(5, 2, 10);
    vector<int> w(5);
    w[0] = 1;
    w[1] = 1;
    w[2] = 1;
    w[3] = 1;
    w[4] = -1;
    for(int i = 0; i != 4; ++i)
      x[i].setmin(s, 7, NO_REASON);
    post_lin_leq(s, x, w, -20);
    s.propagate();
    for(int i = 0; i != 4; ++i) {
      assert( x[i].max(s) == 9 );
    }
    assert( x[4].min(s) == 8 );
  }

  /* sum x1 .. x4 <= 20 implies b
     x1 .. x4 \in [3, 5]

     result: b true
   */
  void test05()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(4, 3, 5);
    vector<int> w(4);
    cspvar b = s.newCSPVar(0, 1);
    w[0] = 1;
    w[1] = 1;
    w[2] = 1;
    w[3] = 1;
    post_lin_leq_imp_b_re(s, x, w, -20, b);
    s.propagate();
    assert( b.min(s) == 1 );
  }

  /* sum x1 .. x4 <= 20 implies b
     x1 .. x4 \in [1,6]

     branch 1:
     set all xi <= 5, result: b true

     branch 2:
     set b false, result: all xi >= 3
  */
  void test06()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(4, 1, 6);
    vector<int> w(4);
    cspvar b = s.newCSPVar(0, 1);
    w[0] = 1;
    w[1] = 1;
    w[2] = 1;
    w[3] = 1;
    post_lin_leq_imp_b_re(s, x, w, -20, b);
    Clause *confl = s.propagate();
    assert(!confl);

    s.newDecisionLevel();
    for(int i = 0; i != 4; ++i)
      x[i].setmax(s, 5, NO_REASON);
    confl = s.propagate();
    assert(!confl);
    assert(b.min(s) == 1);

    s.cancelUntil(0);
    s.newDecisionLevel();
    b.setmax(s, 0, NO_REASON);
    confl = s.propagate();
    assert(!confl);
    for(int i = 0; i != 4; ++i)
      assert( x[i].min(s) == 3 );
  }

  /* b --> sum x1 .. x4 <= 20
     x1 .. x4 \in 6..10

     result: b false
   */
  void test07()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(4, 6, 10);
    vector<int> w(4);
    cspvar b = s.newCSPVar(0, 1);
    w[0] = 1;
    w[1] = 1;
    w[2] = 1;
    w[3] = 1;
    post_b_imp_lin_leq_re(s, b, x, w, -20);
    s.propagate();
    assert(b.max(s) == 0);
  }

  /* b --> sum -x1 .. -x4 <= -20
     x1 .. x4 \in [1,6]

     branch 1:
     set all xi <= 4, result: b false

     branch 2:
     set b true, result: all xi >= 2
   */
  void test08()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(4, 1, 6);
    vector<int> w(4);
    cspvar b = s.newCSPVar(0, 1);
    w[0] = -1;
    w[1] = -1;
    w[2] = -1;
    w[3] = -1;
    post_b_imp_lin_leq_re(s, b, x, w, 20);
    Clause *confl = s.propagate();
    assert(!confl);

    s.newDecisionLevel();
    for(int i = 0; i != 4; ++i)
      x[i].setmax(s, 4, NO_REASON);
    s.propagate();
    assert(b.max(s) == 0);

    s.cancelUntil(0);
    s.newDecisionLevel();
    b.assign(s, 1, NO_REASON);
    s.propagate();
    for(int i = 0; i != 4; ++i)
      assert( x[i].min(s) == 2 );
  }

  /* SEND+MORE=MONEY */
  void test09()
  {
    Solver s;
    vector<cspvar> x = s.newCSPVarArray(8, 0, 9);
    cspvar S = x[0];
    cspvar E = x[1];
    cspvar N = x[2];
    cspvar D = x[3];
    cspvar M = x[4];
    cspvar O = x[5];
    cspvar R = x[6];
    cspvar Y = x[7];

    vector<cspvar> v;
    vector<int> c;
    v.push_back(S); c.push_back(1000);
    v.push_back(E); c.push_back(100);
    v.push_back(N); c.push_back(10);
    v.push_back(D); c.push_back(1);
    v.push_back(M); c.push_back(1000);
    v.push_back(O); c.push_back(100);
    v.push_back(R); c.push_back(10);
    v.push_back(E); c.push_back(1);
    v.push_back(M); c.push_back(-10000);
    v.push_back(O); c.push_back(-1000);
    v.push_back(N); c.push_back(-100);
    v.push_back(E); c.push_back(-10);
    v.push_back(Y); c.push_back(-1);

    vector<int> c1(c);
    for(size_t i = 0; i != c1.size(); ++i) c1[i] = -c1[i];

    post_lin_leq(s, v, c, 0);
    post_lin_leq(s, v, c1, 0);

    s.solve();
  }
}

void lin_test()
{
  cerr << "lin tests\n";

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
}
