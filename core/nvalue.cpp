#include <algorithm>
#include "solver.hpp"

using std::vector;
using std::min;
using std::max;
using std::sort;

class cons_atmostnvalue : public cons
{
  vector<cspvar> _x;
  cspvar _N;

  int dmin, dmax;
public:
  cons_atmostnvalue(Solver &s, vector<cspvar> const& x, cspvar N) :
    _x(x), _N(N)
  {
    dmin = x[0].min(s);
    dmax = x[0].max(s);
    for(size_t i = 1; i != _x.size(); ++i) {
      dmin = min( dmin, x[i].min(s) );
      dmax = max( dmax, x[i].max(s) );
      s.schedule_on_lb(_x[i], this);
      s.schedule_on_ub(_x[i], this);
    }
    s.schedule_on_ub(_N, this);

    DO_OR_THROW(propagate(s));
  }

  Clause *propagate(Solver &s);
};

namespace {
  struct incr_lb {
    Solver &s;
    incr_lb(Solver &ps) : s(ps) {}
    bool operator()(cspvar x1, cspvar x2) {
      return x1.min(s) < x2.min(s);
    }
  };
}

Clause* cons_atmostnvalue::propagate(Solver &s)
{
  const size_t m = _x.size();

  sort(_x.begin(), _x.end(), incr_lb(s));

  int lb = 1;
  int reinit = 1;
  int low = dmin-1, high = dmax+1;

  vec<Lit> ps;

  int highvar = 1;
  for(size_t i = 1; i < m; ) {
    i = i + 1 - reinit;
    if( reinit || low < _x[i-1].min(s) )
      low = _x[i-1].min(s);
    if( reinit || high > _x[i-1].max(s) ) {
      highvar = i-1;
      high = _x[i-1].max(s);
    }
    reinit = (low > high);
    if( reinit ) {
      pushifdef( ps, _x[highvar].r_min(s) );
      ps.push( _x[highvar].r_max(s) );
    }
    lb += reinit;
  }
  pushifdef( ps, _x[highvar].r_min(s) );

  return _N.setminf(s, lb, ps);
}


void post_atmostnvalue(Solver &s, std::vector<cspvar> const& x, cspvar N)
{
  cons *con = new cons_atmostnvalue(s, x, N);
  s.addConstraint(con);
}
