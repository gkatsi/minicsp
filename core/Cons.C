#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include "Cons.h"
#include "Solver.h"

using std::pair;
using std::make_pair;
using std::vector;

class cons_eq;  // ==
class cons_neq; // !=

/* x <= y + c */
class cons_le : public cons {
  cspvar _x, _y;
  int _c;
  vec<Lit> _reason; // cache to avoid the cost of allocation
public:
  cons_le(Solver &s,
          cspvar x, cspvar y, int c) :
    _x(x), _y(y), _c(c)
  {
    s.wake_on_lb(x, this);
    s.wake_on_ub(y, this);
    _reason.push(); _reason.push();
    if( wake(s, 0) )
      throw unsat();
  }

  virtual Clause *wake(Solver& s, Var p);
};

Clause *cons_le::wake(Solver& s, Var)
{
  if( _y.max(s) + _c < _x.min(s) ) { // failure
    _reason[0] = _x.r_geq( s, _x.min(s) );
    _reason[1] = _y.r_leq( s, _x.min(s) - _c );
    Clause *r = Clause_new(_reason);
    s.addInactiveClause(r);
    return r;
  }

  if( _y.min(s) + _c < _x.min(s) ) {
    _reason[1] = _x.r_geq(s, _x.min(s));
    _reason[0] = _y.e_geq(s, _x.min(s) - _c);
    if( _reason[1] == lit_Undef )
      _y.setmin(s, _x.min(s) - _c, NO_REASON);
    else
      _y.setmin(s, _x.min(s) - _c, _reason);
  }
  if( _x.max(s) - _c > _y.max(s) ) {
    _reason[1] = _y.r_leq( s, _y.max(s));
    _reason[0] = _x.e_leq( s, _y.max(s) + _c );
    if( _reason[1] == lit_Undef )
      _x.setmax(s, _y.max(s) + _c, NO_REASON);
    else
      _x.setmax(s, _y.max(s) + _c, _reason);
  }

  return 0L;
}

// v1 <= v2 + c
cons *post_leq(Solver& s, cspvar v1, cspvar v2, int c)
{
  cons *con = new cons_le(s, v1, v2, c);
  s.addConstraint(con);
  return con;
}

// v1 < v2 + c
cons *post_less(Solver& s, cspvar v1, cspvar v2, int c)
{
  cons *con = new cons_le(s, v1, v2, c-1);
  s.addConstraint(con);
  return con;
}


/* Reified versions */
class cons_eq_re;
class cons_neq_re;
class cons_lt_re;
class cons_le_re;

/* N-ary linear inequality, fixed arity (by template), non-fixed arity
   and reified versions of both, Boolean and non-Boolean versions */
template<int N> class cons_fixed_lin_le;
template<int N> class cons_fixed_lin_le_re;
class cons_nary_lin_le;
class cons_nary_lin_le_re;
/* no cons_lin_eq, because it's NP-hard. Just simulate with le and
   ge. Also no lt, simulate with c == 1 */

/* sum_i weight[i]*var[i] >= lb */
namespace pb {
  struct compare_abs_weights {
    bool operator()(pair<int, Var> a, pair<int, Var> b)
    {
      return abs(a.first) > abs(b.first);
    }
  };
}

class cons_pb : public cons {
  std::vector< std::pair<int, Var> > _posvars;
  std::vector< std::pair<int, Var> > _negvars;
  std::vector< std::pair<int, Var> > _svars; // sorted by absolute value
  int _lb;
  int _ub; // the perfect upper bound
public:
  cons_pb(Solver &s,
          std::vector<Var> const& vars,
          std::vector<int> const& weights, int lb);

  virtual Clause *wake(Solver& s, Var p);
};

cons *post_pb(Solver& s, vector<Var> const& vars,
              vector<Var> const& weights, int lb)
{
  cons *c = new cons_pb(s, vars, weights, lb);
  s.addConstraint(c);
  return c;
}

cons_pb::cons_pb(Solver& s,
                 vector<Var> const& vars, vector<int> const& weights,
                 int lb) : _lb(lb), _ub(0)
{
  assert(vars.size() == weights.size());
  size_t n = vars.size();
  for(size_t i = 0; i != n; ++i) {
    if( weights[i] > 0 ) {
      _posvars.push_back(make_pair(weights[i], vars[i]));
      _ub += weights[i];
    }
    else if( weights[i] < 0 )
      _negvars.push_back(make_pair(weights[i], vars[i]));
    else if( weights[i] == 0 ) continue;
    _svars.push_back(make_pair(weights[i], vars[i]));
  }
  sort(_svars.begin(), _svars.end(), pb::compare_abs_weights());

  // wake on every assignment
  n = _svars.size();
  for(size_t i = 0; i != n; ++i)
    s.wake_on_lit(_svars[i].second, this);
}

Clause *cons_pb::wake(Solver& s, Var)
{
  size_t np = _posvars.size(),
    nn = _negvars.size();
  int ub = 0;
  for(size_t i = 0; i != np; ++i) {
    pair<int, Var> const& cv = _posvars[i];
    int q = toInt(s.value(cv.second));
    /* the expression 1 - ((-q+1) >> 1) is
       1 <=> true or unset
       0 <=> false */
    int p = 1-((1-q)>>1);
    ub += p*cv.first;
  }
  for(size_t i = 0; i != nn; ++i) {
    pair<int, Var> const& cv = _negvars[i];
    int q = toInt(s.value(cv.second));
    /* the expression (q+1)>>1 is
       0 <=> false or unset
       1 <=> true */
    int p = (q+1)>>1;
    ub += p*cv.first;
  }
  if( ub >= _lb ) return 0L; // satisfiable

  vec<Lit> ps;
  size_t n = _svars.size();
  int aub = _ub;
  for(size_t i = 0; i != n; ++i) {
    pair<int, Var> const& cv = _svars[i];
    lbool sv = s.value(cv.second);
    if( sv == l_Undef ) continue;
    if( cv.first > 0 && sv == l_False ) {
      aub -= cv.first;
      Lit lx(cv.second);
      ps.push( lx );
    } else if( cv.first < 0 && sv == l_True ) {
      aub += cv.first;
      Lit lx(cv.second, true);
      ps.push( lx );
    }
    if( aub < _lb ) break;
  }

  Clause *conf = Clause_new(ps, true);
  s.addInactiveClause(conf);
  return conf;
}

class cons_pb_re;

class cons_div; // integer division
class cons_min;
class cons_max;
class cons_abs;

/* Element */
class cons_element;

/* Set constraints */
class cons_set_in;
class cons_set_subset;
class cons_set_union;
class cons_set_card;

class cons_table;

/* Global constraints */
class cons_alldiff;
class cons_gcc;
