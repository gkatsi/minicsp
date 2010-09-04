#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include "cons.hpp"
#include "solver.hpp"

using std::pair;
using std::make_pair;
using std::vector;

/* x == y + c */
class cons_eq : public cons {
  cspvar _x, _y;
  int _c;
  vec<Lit> _reason; // cache to avoid the cost of allocation
public:
  cons_eq(Solver &s,
           cspvar x, cspvar y, int c) :
    _x(x), _y(y), _c(c)
  {
    // wake on everything! more immediate consequences
    s.wake_on_dom(x, this);
    s.wake_on_lb(x, this);
    s.wake_on_ub(x, this);
    s.wake_on_fix(x, this);
    s.wake_on_dom(y, this);
    s.wake_on_lb(y, this);
    s.wake_on_ub(y, this);
    s.wake_on_fix(y, this);
    _reason.push(); _reason.push();

    if( _x.min(s) > _y.max(s)+_c ||
        _y.min(s) > _x.max(s)-_c )
      throw unsat();

    DO_OR_THROW(_y.setmin(s, _x.min(s)-_c, NO_REASON));
    DO_OR_THROW(_y.setmax(s, _x.max(s)-_c, NO_REASON));
    DO_OR_THROW(_x.setmin(s, _y.min(s)+_c, NO_REASON));
    DO_OR_THROW(_x.setmax(s, _y.max(s)+_c, NO_REASON));

    for(int i = x.min(s)+1, iend = x.max(s); i != iend; ++i) {
      if( !x.indomain(s, i) )
        DO_OR_THROW(wake(s, x.e_neq(s, i)));
    }

    for(int i = y.min(s)+1, iend = y.max(s); i != iend; ++i) {
      if( !y.indomain(s, i) )
        DO_OR_THROW(wake(s, y.e_neq(s, i)));
    }
  }

  virtual Clause *wake(Solver& s, Lit p);
};

Clause *cons_eq::wake(Solver& s, Lit event)
{
  domevent e = s.event(event);
  _reason[0] = ~event;
  switch(e.type) {
  case domevent::EQ:
    if( e.x == _x ) {
      _reason[1] = _y.e_eq(s, e.d-_c);
      return _y.assign(s, e.d-_c, _reason);
    } else {
      _reason[1] = _x.e_eq(s, e.d+_c);
      return _x.assign(s, e.d+_c, _reason);
    }
    break;
  case domevent::NEQ:
    if( e.x == _x ) {
      _reason[1] = _y.e_neq(s, e.d-_c);
      return _y.remove(s, e.d-_c, _reason);
    } else {
      _reason[1] = _x.e_neq(s, e.d+_c);
      return _x.remove(s, e.d+_c, _reason);
    }
    break;
  case domevent::GEQ:
    if( e.x == _x ) {
      _reason[1] = _y.e_geq(s, e.d-_c);
      return _y.setmin(s, e.d-_c, _reason);
    } else {
      _reason[1] = _x.e_geq(s, e.d+_c);
      return _x.setmin(s, e.d+_c, _reason);
    }
    break;
  case domevent::LEQ:
    if( e.x == _x ) {
      _reason[1] = _y.e_leq(s, e.d-_c);
      return _y.setmax(s, e.d-_c, _reason);
    } else {
      _reason[1] = _x.e_leq(s, e.d+_c);
      return _x.setmax(s, e.d+_c, _reason);
    }
    break;
  case domevent::NONE:
    assert(0);
  }
  return 0L;
}

/* x == y + c */
void post_eq(Solver& s, cspvar x, cspvar y, int c)
{
  cons *con = new cons_eq(s, x, y, c);
  s.addConstraint(con);
}

/* x != y + c */
class cons_neq : public cons {
  cspvar _x, _y;
  int _c;
  vec<Lit> _reason; // cache to avoid the cost of allocation
public:
  cons_neq(Solver &s,
           cspvar x, cspvar y, int c) :
    _x(x), _y(y), _c(c)
  {
    s.wake_on_fix(x, this);
    s.wake_on_fix(y, this);
    _reason.push(); _reason.push();
    if( x.min(s) == x.max(s) ) {
      Clause *c = wake(s, x.e_eq(s, x.min(s)));
      if(c) throw unsat();
    } else if( y.min(s) == y.max(s) ) {
      Clause *c = wake(s, y.e_eq(s, y.min(s)));
      if(c) throw unsat();
    }
  }

  virtual Clause *wake(Solver& s, Lit p);
};

Clause *cons_neq::wake(Solver& s, Lit event)
{
  domevent e = s.event(event);
  assert( e.type == domevent::EQ );
  if( e.x == _x ) {
    _reason[0] = ~event;
    _reason[1] = _y.e_neq(s, e.d-_c);
    return _y.remove(s, e.d-_c, _reason);
  } else {
    _reason[0] = ~event;
    _reason[1] = _x.e_neq(s, e.d+_c);
    return _x.remove(s, e.d+_c, _reason);
  }
}

/* x != y + c */
void post_neq(Solver& s, cspvar x, cspvar y, int c)
{
  cons *con = new cons_neq(s, x, y, c);
  s.addConstraint(con);
}

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
    if( wake(s, lit_Undef) )
      throw unsat();
  }

  virtual Clause *wake(Solver& s, Lit p);
};

Clause *cons_le::wake(Solver& s, Lit)
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
void post_leq(Solver& s, cspvar v1, cspvar v2, int c)
{
  cons *con = new cons_le(s, v1, v2, c);
  s.addConstraint(con);
}

// v1 < v2 + c
void post_less(Solver& s, cspvar v1, cspvar v2, int c)
{
  cons *con = new cons_le(s, v1, v2, c-1);
  s.addConstraint(con);
}


/* Reified versions */
class cons_eq_re;
class cons_neq_re;
class cons_lt_re;
class cons_le_re;

/* cons_lin_le

   N-ary linear inequality

   sum w[i]*v[i] + c <= 0

   The template parameter N is used to tell the compiler to create
   optimized versions for small N. When N = 0, it uses the generic
   loop, otherwise constant propagation will make it a fixed loop.
*/
template<size_t N>
class cons_lin_le : public cons {
  vector< pair<int, cspvar> > _vars;
  int _c;
  btptr _lbptr;

  vec<Lit> _ps;

  // computes the minimum contribution of var x with coeff w:
  // w*x.min() if w>0, w*x.max() if w<0
  int vmin(Solver &s, int w, cspvar x);
  // describe the domain of x to use in the reason for whatever we do:
  // 'x>=x.min()' if w>0, 'x<=x.max()' if w<0
  Lit litreason(Solver &s, int lb, int w, cspvar x);
public:
  cons_lin_le(Solver &s,
              vector< pair<int, cspvar> > const& vars,
              int c);

  Clause *wake(Solver& s, Lit p);
};

template<size_t N>
int cons_lin_le<N>::vmin(Solver &s, int w, cspvar x)
{
  return select(w, w*x.min(s), w*x.max(s));
}

template<size_t N>
Lit cons_lin_le<N>::litreason(Solver &s, int lb,
                              int w, cspvar x)
{
  return toLit( select(w,
                       toInt(x.r_geq(s, x.min(s))),
                       toInt(x.r_leq(s, x.max(s)))));
}

template<size_t N>
cons_lin_le<N>::cons_lin_le(Solver &s,
                            vector< pair<int, cspvar> > const& vars,
                            int c) :
  _vars(vars), _c(c)
{
  assert(N == 0 || N == vars.size());
  _lbptr = s.alloc_backtrackable(sizeof(int));
  int & lb = s.deref<int>(_lbptr);
  lb = _c;
  for(size_t i = 0; i != vars.size(); ++i) {
    lb += vmin(s, _vars[i].first, _vars[i].second);
    if( _vars[i].first > 0 )
      s.wake_on_lb(_vars[i].second, this);
    else
      s.wake_on_ub(_vars[i].second, this);
  }
  if( lb > 0 ) throw unsat();
  _ps.growTo(_vars.size(), lit_Undef);
  if( wake(s, lit_Undef) )
    throw unsat();
}

template<size_t N>
Clause *cons_lin_le<N>::wake(Solver &s, Lit)
{
  const size_t n = (N > 0) ? N : _vars.size();
  int & lb = s.deref<int>(_lbptr);
  int pspos[_vars.size()];

  lb = _c;
  size_t nl = 0;
  for(size_t i = 0; i != n; ++i) {
    int w = _vars[i].first;
    cspvar x = _vars[i].second;
    _ps[nl] = litreason(s, lb, w, x);
    pspos[i] = nl;
    nl += select( toInt(_ps[nl]), 1, 0 );
    lb += vmin(s, w, x);
  }

  if( lb > 0 ) {
    // shrink _ps to contruct the clause, then bring it back to its
    // previous size
    _ps.shrink(_vars.size() - nl );
    Clause *r = Clause_new(_ps);
    _ps.growTo(_vars.size(), lit_Undef);
    s.addInactiveClause(r);
    return r;
  }

  _ps.shrink(_vars.size() - nl );
  for(size_t i = 0; i != n; ++i) {
    Lit l = _ps[pspos[i]];
    int w = _vars[i].first;
    cspvar x = _vars[i].second;
    int gap = lb-vmin(s, w, x);
    double newbound = -gap/(double)w;
    if( w > 0 ) {
      int ibound = floor(newbound);
      if( litreason(s, lb, w, x) == lit_Undef ) {
        _ps.push(x.e_leq(s, ibound));
        x.setmax(s, ibound, _ps);
        _ps.pop();
      } else {
        _ps[pspos[i]] = x.e_leq(s, ibound);
        x.setmax(s, ibound, _ps);
        _ps[pspos[i]] = l;
      }
    } else {
      int ibound = ceil(newbound);
      if( litreason(s, lb, w, x) == lit_Undef ) {
        _ps.push(x.e_geq(s, ibound));
        x.setmin(s, ibound, _ps);
        _ps.pop();
      } else {
        _ps[pspos[i]] = x.e_geq(s, ibound);
        x.setmin(s, ibound, _ps);
        _ps[pspos[i]] = l;
      }
    }
  }
  _ps.growTo(_vars.size(), lit_Undef);

  return 0L;
}

namespace lin {
  struct cmp_varid {
    bool operator()(pair<int, cspvar> x1, pair<int, cspvar> x2) const {
      return x1.second.id() < x2.second.id();
    }
  };
}

void post_lin_leq(Solver &s, vector<cspvar> const& vars,
                   vector<int> const &coeff, int c)
{
  assert(vars.size() == coeff.size());
  vector< pair<int, cspvar> > pairs;
  for(size_t i = 0; i != vars.size(); ++i) {
    if( !coeff[i] ) continue;
    if( vars[i].min(s) == vars[i].max(s) ) {
      c += coeff[i]*vars[i].min(s);
      continue;
    }
    pairs.push_back( make_pair(coeff[i], vars[i]));
  }

  if( pairs.size() == 0 ) {
    if( c > 0 ) throw unsat();
    return;
  }

  sort(pairs.begin(), pairs.end(), lin::cmp_varid());
  vector< pair<int, cspvar> >::iterator i = pairs.begin(), j = i;
  ++j;
  for(; j != pairs.end(); ++j) {
    if( i->second == j->second )
      i->first += j->first;
    else {
      ++i;
      *i = *j;
    }
  }
  ++i;
  pairs.erase(i, pairs.end());

  cons *con;
  switch(pairs.size()) {
  case 1:   con = new cons_lin_le<1>(s, pairs, c); break;
  case 2:   con = new cons_lin_le<2>(s, pairs, c); break;
  case 3:   con = new cons_lin_le<3>(s, pairs, c); break;
  default:  con = new cons_lin_le<0>(s, pairs, c); break;
  }
  s.addConstraint(con);
}

void post_lin_less(Solver &s, vector<cspvar> const& vars,
                    vector<int> const &coeff, int c)
{
  post_lin_leq(s, vars, coeff, c+1);
}

void post_lin_leq_imp_b_re(Solver &s, vector<cspvar> const&vars,
                            vector<int> const &coeff,
                            int c, cspvar b)
{
  assert(vars.size() == coeff.size());
  assert(b.min(s) >= 0 && b.max(s) <= 1);

  if(b.max(s) == 0) {
    vector<int> c1(coeff);
    for(size_t i = 0; i != vars.size(); ++i)
      c1[i] = -c1[i];
    post_lin_leq(s, vars, c1, -c+1);
    return;
  }

  vector<cspvar> v1(vars);
  vector<int> c1(coeff);

  /* Let L = c + sum coeff[i]*vars[i] */
  /* Discover lb, ub, s.t. lb <= L <= ub */
  int ub = c, lb = c;
  for(size_t i = 0; i != vars.size(); ++i) {
    if( coeff[i] > 0 ) {
      ub += coeff[i] * vars[i].max(s);
      lb += coeff[i] * vars[i].min(s);
    } else {
      ub += coeff[i] * vars[i].min(s);
      lb += coeff[i] * vars[i].max(s);
    }
    c1[i] = -c1[i];
  }

  if( ub <= 0 ) { /* L <= 0 always, b is true */
    Clause *confl = b.assign(s, 1, NO_REASON);
    if( confl ) throw unsat();
    return;
  }
  if( lb > 0 ) { /* L > 0 always, rhs unaffected */
    return;
  }

  v1.push_back(b);
  c1.push_back(lb-1);

  // post -L + 1 + (lb - 1)*b <= 0
  post_lin_leq(s, v1, c1, -c+1);
}

void post_lin_less_imp_b_re(Solver &s, vector<cspvar> const&vars,
                             vector<int> const &coeff,
                             int c, cspvar b)
{
  post_lin_leq_imp_b_re(s, vars, coeff, c+1, b);
}

void post_b_imp_lin_leq_re(Solver &s,
                            cspvar b,
                            std::vector<cspvar> const&vars,
                            std::vector<int> const &coeff,
                            int c)
{
  assert(vars.size() == coeff.size());
  assert(b.min(s) >= 0 && b.max(s) <= 1);

  if( b.min(s) == 1 ) {
    post_lin_leq(s, vars, coeff, c);
    return;
  }

  vector<cspvar> v1(vars);
  vector<int> c1(coeff);

  /* Let L = c + sum coeff[i]*vars[i] */
  /* Discover lb, ub, s.t. lb <= L <= ub */
  int ub = c, lb = c;
  for(size_t i = 0; i != vars.size(); ++i) {
    if( coeff[i] > 0 ) {
      ub += coeff[i] * vars[i].max(s);
      lb += coeff[i] * vars[i].min(s);
    } else {
      ub += coeff[i] * vars[i].min(s);
      lb += coeff[i] * vars[i].max(s);
    }
  }

  if( ub <= 0 ) { /* L <= 0 always, lhs unaffected */
    return;
  }
  if( lb > 0 ) { /* L > 0 always, set b to false */
    Clause *confl = b.assign(s, 0, NO_REASON);
    if( confl ) throw unsat();
    return;
  }

  v1.push_back(b);
  c1.push_back(ub+1);

  // post -L + 1 + (lb - 1)*b <= 0
  post_lin_leq(s, v1, c1, c-ub-1);
}

void post_b_imp_lin_less_re(Solver &s,
                             cspvar b,
                             std::vector<cspvar> const&vars,
                             std::vector<int> const &coeff,
                             int c)
{
  post_b_imp_lin_leq_re(s, b, vars, coeff, c+1);
}

void post_lin_leq_iff_re(Solver &s, std::vector<cspvar> const& vars,
                          std::vector<int> const& coeff,
                          int c, cspvar b)
{
  post_b_imp_lin_leq_re(s, b, vars, coeff, c);
  post_lin_leq_imp_b_re(s, vars, coeff, c, b);
}

void post_lin_less_iff_re(Solver &s, std::vector<cspvar> const& vars,
                           std::vector<int> const& coeff,
                           int c, cspvar b)
{
  post_b_imp_lin_leq_re(s, b, vars, coeff, c);
  post_lin_leq_imp_b_re(s, vars, coeff, c, b);
}



/* cons_pb

   PseudoBoolean constraint

   sum_i weight[i]*var[i] >= lb
*/
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

  virtual Clause *wake(Solver& s, Lit p);
};

void post_pb(Solver& s, vector<Var> const& vars,
              vector<Var> const& weights, int lb)
{
  cons *c = new cons_pb(s, vars, weights, lb);
  s.addConstraint(c);
}

void post_pb(Solver& s, vector<cspvar> const& vars,
              vector<Var> const& weights, int lb)
{
  vector<Var> vbool(vars.size());
  for(size_t i = 0; i != vars.size(); ++i)
    vbool[i] = vars[i].eqi(s, 1);

  cons *c = new cons_pb(s, vbool, weights, lb);
  s.addConstraint(c);
}

// sum ci*bi >= lb ==> b
void post_pb_right_imp_re(Solver& s, std::vector<cspvar> const& vars,
                          std::vector<int> const& weights, int lb,
                          cspvar b)
{
  vector<cspvar> v1(vars);
  vector<int> w1(weights);
  int lub = 0, llb = 0;
  for(size_t i = 0; i != vars.size(); ++i) {
    if( weights[i] > 0 )
      lub += weights[i];
    else
      llb += weights[i];
    w1[i] = -w1[i];
  }

  int diff = lub - llb + 1;
  v1.push_back(b);
  w1.push_back( diff );
  post_pb(s, v1, w1, - lb + 1);
}

// b ==> sum ci*bi >= lb
void post_pb_left_imp_re(Solver& s, std::vector<cspvar> const& vars,
                         std::vector<int> const& weights, int lb,
                         cspvar b)
{
  vector<cspvar> v1(vars);
  vector<int> w1(weights);
  int lub = 0, llb = 0;
  for(size_t i = 0; i != vars.size(); ++i) {
    if( weights[i] > 0 )
      lub += weights[i];
    else
      llb += weights[i];
  }

  int diff = lub - llb + 1;
  v1.push_back(b);
  w1.push_back( -diff );
  post_pb(s, v1, w1, lb - diff);
}

void post_pb_iff_re(Solver& s, std::vector<cspvar> const& vars,
                    std::vector<int> const& weights, int lb,
                    cspvar b)
{
  post_pb_right_imp_re(s, vars, weights, lb, b);
  post_pb_left_imp_re(s, vars, weights, lb, b);
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

Clause *cons_pb::wake(Solver& s, Lit)
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

void post_alldiff(Solver &s, std::vector<cspvar> const &x)
{
  for(size_t i = 0; i != x.size(); ++i)
    for(size_t j = i+1; j != x.size(); ++j)
      post_neq(s, x[i], x[j], 0);
}
