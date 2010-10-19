#include <iostream>
#include "test.hpp"
#include "solver.hpp"

using namespace std;

namespace {
  class test_cons : public cons
  {
  public:
    int _id;
    vector<int> & _wakes;

    vector<cspvar> _d;
    vector<cspvar> _l;
    vector<cspvar> _u;
    vector<cspvar> _f;


    test_cons(Solver &s,
              int id,
              vector<int>& wakes,
              vector<cspvar> const& d,
              vector<cspvar> const& l,
              vector<cspvar> const& u,
              vector<cspvar> const& f) :
      _id(id), _wakes(wakes),
      _d(d), _l(l), _u(u), _f(f)
    {
      for(size_t i = 0; i != _d.size(); ++i)
        s.schedule_on_dom(_d[i], this);
      for(size_t i = 0; i != _l.size(); ++i)
        s.schedule_on_lb(_l[i], this);
      for(size_t i = 0; i != _u.size(); ++i)
        s.schedule_on_ub(_u[i], this);
      for(size_t i = 0; i != _f.size(); ++i)
        s.schedule_on_fix(_f[i], this);
    }

    Clause *propagate(Solver& s)
    {
      _wakes.push_back(_id);
      return 0L;
    }
  };

  void post_test(Solver &s,
                 int id,
                 vector<int>& wakes,
                 vector<cspvar> const& d,
                 vector<cspvar> const& l,
                 vector<cspvar> const& u,
                 vector<cspvar> const& f)
  {
    cons *con = new test_cons(s, id, wakes, d, l, u, f);
    s.addConstraint(con);
  }

  bool compare_events(vector<int> const &wakes,
                      int *expected)
  {
    size_t i = 0;
    for(; i != wakes.size() && expected[i] >= 0 ; ++i)
      if(expected[i] != wakes[i])
        return false;
    if( expected[i] >= 0 || i < wakes.size() )
      return false;
    return true;
  }

  // a prop wakes
  void schedule01()
  {
    Solver s;
    vector<cspvar> d, l, u, f;
    d = s.newCSPVarArray(2, 5, 10);

    vector<int> wakes;

    post_test(s, 1, wakes, d, l, u, f);

    int exp0[] = { -1 };
    assert( !s.propagate() );
    assert( compare_events(wakes, exp0) );

    int exp1[] = { 1, -1 };
    s.newDecisionLevel();
    d[0].remove(s, 6, NO_REASON);
    assert( !s.propagate() );
    assert( compare_events(wakes, exp1) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(schedule01);

  // a prop wakes in all event combinations
  void schedule02()
  {
    Solver s;
    vector<cspvar> d, l, u, f;
    d = s.newCSPVarArray(2, 5, 10);
    l = s.newCSPVarArray(2, 5, 10);
    u = s.newCSPVarArray(2, 5, 10);
    f = s.newCSPVarArray(2, 5, 10);

    vector<int> wakes;

    post_test(s, 1, wakes, d, l, u, f);

    int exp[] = { 1, -1 };
    assert( !s.propagate() );
    assert( compare_events(wakes, exp) );

    // lbound change triggers dom schedule
    wakes.clear();
    s.newDecisionLevel();
    d[0].setmin(s, 6, NO_REASON);
    assert( !s.propagate() );
    assert( compare_events(wakes, exp) );
    s.cancelUntil(0);

    // ubound change triggers dom schedule
    wakes.clear();
    s.newDecisionLevel();
    d[0].setmax(s, 6, NO_REASON);
    assert( !s.propagate() );
    assert( compare_events(wakes, exp) );
    s.cancelUntil(0);

    // fix triggers dom schedule
    wakes.clear();
    s.newDecisionLevel();
    d[0].assign(s, 6, NO_REASON);
    assert( !s.propagate() );
    assert( compare_events(wakes, exp) );
    s.cancelUntil(0);
  }
  REGISTER_TEST(schedule02);

  // schedule 2 propagators
  // reschedule 1 propagator, no others present
  // reschedule 1 propagator, first in queue
  // reschedule 1 propagator, middle of queue
  // reschedule 1 propagator, end of queue

  // check that queue is cleared correctly: schedule p, fail before p
  // is executed, backtrack and make decisions that do not schedule p
  // again. p must not be in wakes.

  //--------------------------------------------------
  // multiple priorities

  // 2 priority 0, 1 priority 1
  // 4 priority 0, 2 priority 1, 1 priority 2
}

void schedule_test()
{
  cerr << "propagator scheduling tests\n";
  the_test_container().run();
}
