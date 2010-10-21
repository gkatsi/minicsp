#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <limits>
#include <cassert>
#include "cons.hpp"
#include "solver.hpp"

using std::ostream;
using std::pair;
using std::make_pair;
using std::vector;
using std::set;

/* x == y + c     and        x == -y + c

   implemented as x = W*y + c, but with W == 1 or W == -1
*/
namespace eq {
  template<int W>
  Clause *eq_propagate(Solver &s, cspvar _x, cspvar _y, int _c, Lit event,
                       vec<Lit>& _reason)
  {
    domevent e = s.event(event);
    _reason.push(~event);
    switch(e.type) {
    case domevent::EQ:
      if( e.x == _x ) {
        return _y.assignf(s, W*e.d-W*_c, _reason);
      } else {
        return _x.assignf(s, W*e.d+_c, _reason);
      }
      break;
    case domevent::NEQ:
      if( e.x == _x ) {
        return _y.removef(s, W*e.d-W*_c, _reason);
      } else {
        return _x.removef(s, W*e.d+_c, _reason);
      }
      break;
    case domevent::GEQ:
      if( e.x == _x ) {
        if( W > 0 ) {
          return _y.setminf(s, e.d-_c, _reason);
        } else {
          return _y.setmaxf(s, -e.d+_c, _reason);
        }
      } else {
        if( W > 0 ) {
          return _x.setminf(s, e.d+_c, _reason);
        } else {
          return _x.setmaxf(s, -e.d+_c, _reason);
        }
      }
      break;
    case domevent::LEQ:
      if( e.x == _x ) {
        if( W > 0 ) {
          return _y.setmaxf(s, e.d-_c, _reason);
        } else {
          _reason.push(_y.e_geq(s, -e.d+_c));
          return _y.setminf(s, -e.d+_c, _reason);
        }
      } else {
        if( W > 0 ) {
          return _x.setmaxf(s, e.d+_c, _reason);
        } else {
          return _x.setminf(s, -e.d+_c, _reason);
        }
      }
      break;
    case domevent::NONE:
      assert(0);
    }
    return 0L;
  }

  template<int W>
  Clause *eq_initialize(Solver &s, cspvar _x, cspvar _y, int _c,
                        vec<Lit>& _reason)
  {
    if( W > 0 ) {
      {
        PUSH_TEMP( _reason,  _x.r_min(s) );
        DO_OR_RETURN(_y.setminf(s, _x.min(s)-_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _x.r_max(s) );
        DO_OR_RETURN(_y.setmaxf(s, _x.max(s)-_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _y.r_min(s) );
        DO_OR_RETURN(_x.setminf(s, _y.min(s)+_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _y.r_max(s) );
        DO_OR_RETURN(_x.setmaxf(s, _y.max(s)+_c, _reason));
      }
    } else {
      {
        PUSH_TEMP( _reason,  _x.r_max(s) );
        DO_OR_RETURN(_y.setminf(s, -_x.max(s)+_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _x.r_min(s) );
        DO_OR_RETURN(_y.setmaxf(s, -_x.min(s)+_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _y.r_max(s) );
        DO_OR_RETURN(_x.setminf(s, -_y.max(s)+_c, _reason));
      }

      {
        PUSH_TEMP( _reason,  _y.r_min(s) );
        DO_OR_RETURN(_x.setmaxf(s, -_y.min(s)+_c, _reason));
      }
    }

    for(int i = _x.min(s)+1, iend = _x.max(s); i < iend; ++i) {
      if( !_x.indomain(s, i) )
        DO_OR_RETURN(eq_propagate<W>(s, _x, _y, _c, _x.e_neq(s, i),
                                     _reason));
    }

    for(int i = _y.min(s)+1, iend = _y.max(s); i < iend; ++i) {
      if( !_y.indomain(s, i) )
        DO_OR_RETURN(eq_propagate<W>(s, _x, _y, _c, _y.e_neq(s, i),
                                     _reason));
    }

    return 0L;
  }
}

template<int W>
class cons_eq : public cons {
  cspvar _x, _y;
  int _c;
  vec<Lit> _reason; // cache to avoid the cost of allocation

  void requires() {
    int w[ (W==1)+(W==-1)-1 ] __attribute__ ((__unused__));
  }
public:
  cons_eq(Solver &s,
           cspvar x, cspvar y, int c) :
    _x(x), _y(y), _c(c)
  {
    requires();
    // wake on everything! more immediate consequences
    s.wake_on_dom(x, this);
    s.wake_on_lb(x, this);
    s.wake_on_ub(x, this);
    s.wake_on_fix(x, this);
    s.wake_on_dom(y, this);
    s.wake_on_lb(y, this);
    s.wake_on_ub(y, this);
    s.wake_on_fix(y, this);
    _reason.capacity(2);

    if( eq::eq_initialize<W>(s, _x, _y, _c, _reason) )
      throw unsat();
  }

  virtual Clause *wake(Solver& s, Lit p) {
    _reason.clear();
    return eq::eq_propagate<W>(s, _x, _y, _c, p, _reason);
  }
  virtual void clone(Solver& other);
  virtual ostream& print(Solver &s, ostream& os) const;
  virtual ostream& printstate(Solver& s, ostream& os) const;
};

template<int W>
void cons_eq<W>::clone(Solver &other)
{
  cons *con = new cons_eq<W>(other, _x, _y, _c);
  other.addConstraint(con);
}

template<int W>
ostream& cons_eq<W>::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " = ";
  if( W == -1 ) os << "-";
  os << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  return os;
}

template<int W>
ostream& cons_eq<W>::printstate(Solver& s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  os << cspvar_printer(s, _x) << " in " << domain_as_set(s, _x) << ", "
     << cspvar_printer(s, _y) << " in " << domain_as_set(s, _y)
     << ")";
  return os;
}

/* x == y + c */
void post_eq(Solver& s, cspvar x, cspvar y, int c)
{
  cons *con = new cons_eq<1>(s, x, y, c);
  s.addConstraint(con);
}

/* x == -y + c */
void post_neg(Solver &s, cspvar x, cspvar y, int c)
{
  cons *con = new cons_eq<-1>(s, x, y, c);
  s.addConstraint(con);
}

/* x != y + c */
namespace neq {
  Clause *neq_propagate(Solver &s, cspvar _x, cspvar _y, int _c, Lit event,
                        vec<Lit>& _reason);

  Clause *neq_initialize(Solver &s, cspvar _x, cspvar _y, int _c,
                       vec<Lit>& _reason)
  {
    if( _x.min(s) == _x.max(s) )
      return neq_propagate(s, _x, _y, _c,
                           _x.e_eq(s, _x.min(s)),
                           _reason);
    else if( _y.min(s) == _y.max(s) )
      return neq_propagate(s, _x, _y, _c,
                           _y.e_eq(s, _y.min(s)),
                           _reason);
    return 0L;
  }

  Clause *neq_propagate(Solver &s, cspvar _x, cspvar _y, int _c, Lit event,
                       vec<Lit>& _reason)
  {
    domevent e = s.event(event);
    if( e.type != domevent::EQ )
      return 0L;
    _reason.push(~event);
    if( e.x == _x ) {
      return _y.removef(s, e.d-_c, _reason);
    } else {
      return _x.removef(s, e.d+_c, _reason);
    }
    return 0L;
  }
}

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
    _reason.capacity(2);
    DO_OR_THROW(neq::neq_initialize(s, _x, _y, _c, _reason));
  }

  virtual Clause *wake(Solver& s, Lit p);
  virtual void clone(Solver& other);
  virtual ostream& print(Solver &s, ostream& os) const;
};

Clause *cons_neq::wake(Solver& s, Lit event)
{
  _reason.clear();
  return neq::neq_propagate(s, _x, _y, _c, event, _reason);
}

void cons_neq::clone(Solver &other)
{
  cons *con = new cons_neq(other, _x, _y, _c);
  other.addConstraint(con);
}

ostream& cons_neq::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " != " << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  return os;
}

/* x != y + c */
void post_neq(Solver& s, cspvar x, cspvar y, int c)
{
  cons *con = new cons_neq(s, x, y, c);
  s.addConstraint(con);
}

/* x = y + c <=> b */
namespace eq_re {
  /* Return true if the domains of _x and _y are disjoint (offset
     _c). Also describe the prunings that made it so in _reason. Note
     if it returns false, _reason contains garbage. */
  bool disjoint_domains(Solver &s, cspvar _x, cspvar _y, int _c,
                        vec<Lit>& _reason) {
    using std::min;
    using std::max;
    if( _x.min(s) > _y.max(s) + _c ) {
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _y.r_max(s));
      return true;
    } else if( _x.max(s) < _y.min(s) + _c ) {
      pushifdef(_reason, _x.r_max(s));
      pushifdef(_reason, _y.r_min(s));
      return true;
    } else {
      for(int i = max(_x.min(s), _y.min(s) + _c),
            iend = min(_x.max(s), _y.max(s) + _c)+1; i != iend; ++i) {
        if( _x.indomain(s, i) ) {
          if( _y.indomain(s, i-_c) )
            return false;
          else
            pushifdef(_reason, _y.r_neq(s, i-_c) );
        } else
          pushifdef(_reason, _x.r_neq(s,i));
      }
      return true;
    }
  }
}

class cons_eq_re : public cons {
  cspvar _x, _y;
  int _c;
  Lit _b;
  vec<Lit> _reason;
public:
  cons_eq_re(Solver &s,
             cspvar x, cspvar y, int c,
             Lit b) :
    _x(x), _y(y), _c(c), _b(b)
  {
    s.wake_on_dom(_x, this);
    s.wake_on_lb(_x, this);
    s.wake_on_ub(_x, this);
    s.wake_on_fix(_x, this);
    s.wake_on_dom(_y, this);
    s.wake_on_lb(_y, this);
    s.wake_on_ub(_y, this);
    s.wake_on_fix(_y, this);
    s.wake_on_lit(var(_b), this);
    _reason.capacity(5);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause *cons_eq_re::wake(Solver &s, Lit p)
{
  domevent pevent = s.event(p);
  _reason.clear();
  if( s.value(_b) == l_True ) {
    _reason.push( ~_b );
    if( p == _b )
      return eq::eq_initialize<1>(s, _x, _y, _c, _reason);
    else
      return eq::eq_propagate<1>(s, _x, _y, _c, p, _reason);
  } else if( s.value(_b) == l_False ) {
    _reason.push( _b );
    if( p == ~_b )
      return neq::neq_initialize(s, _x, _y, _c, _reason);
    else
      return neq::neq_propagate(s, _x, _y, _c, p, _reason);
  }

  if( eq_re::disjoint_domains(s, _x, _y, _c, _reason) ) {
    DO_OR_RETURN(s.enqueueFill(~_b, _reason));
  } else if( _x.min(s) == _y.min(s)+_c &&
             _x.min(s) == _x.max(s) &&
             _x.min(s) == _y.max(s)+_c ) {
    _reason.clear(); // disjoint_domains() may have put garbage here
    _reason.push(_x.r_eq(s));
    _reason.push(_y.r_eq(s));
    DO_OR_RETURN(s.enqueueFill(_b, _reason));
  }

  return 0L;
}

void cons_eq_re::clone(Solver & other)
{
  cons *con = new cons_eq_re(other, _x, _y, _c, _b);
  other.addConstraint(con);
}

ostream& cons_eq_re::print(Solver &s, ostream& os) const
{
  os << "(" << cspvar_printer(s, _x) << " = " << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  os << ") <=> " << lit_printer(s, _b);
  return os;
}

ostream& cons_eq_re::printstate(Solver & s, ostream& os) const
{
  print(s, os);
  os << " (with " << cspvar_printer(s, _x) << " in " << domain_as_set(s, _x)
     << ", " << cspvar_printer(s, _y) << " in " << domain_as_set(s, _y)
     << ", " << lit_printer(s, _b)
     << ")";
  return os;
}

void post_eqneq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  vec<Lit> reason;
  if( eq_re::disjoint_domains(s, x, y, c, reason) ) {
    s.uncheckedEnqueue(~b); // will not cause unsat, because b is
                            // unset by if test in callers
    return;
  } else if( x.min(s) == y.min(s)+c &&
             x.min(s) == x.max(s) &&
             x.min(s) == y.max(s)+c ) {
    s.uncheckedEnqueue(b);
    return;
  }

  cons *con = new cons_eq_re(s, x, y, c, b);
  s.addConstraint(con);
}

void post_eq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  assert( b.min(s) >= 0 && b.max(s) <= 1 );
  if( b.min(s) == 1 ) {
    post_eq(s, x, y, c);
    return;
  } else if( b.max(s) == 0 ) {
    post_neq(s, x, y, c);
    return;
  }

  post_eqneq_re(s, x, y, c, Lit(b.eqi(s, 1)));
}

void post_eq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  if( s.value(b) == l_True ) {
    post_eq(s, x, y, c);
    return;
  } else if( s.value(b) == l_False ) {
    post_neq(s, x, y, c);
    return;
  }
  post_eqneq_re(s, x, y, c, b);
}

void post_neq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  assert( b.min(s) >= 0 && b.max(s) <= 1 );
  if( b.min(s) == 1 ) {
    post_neq(s, x, y, c);
    return;
  } else if( b.max(s) == 0 ) {
    post_eq(s, x, y, c);
    return;
  }

  post_eqneq_re(s, x, y, c, ~Lit(b.eqi(s, 1)));
}

void post_neq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  if( s.value(b) == l_True ) {
    post_neq(s, x, y, c);
    return;
  } else if( s.value(b) == l_False ) {
    post_eq(s, x, y, c);
    return;
  }
  post_eqneq_re(s, x, y, c, ~b);
}

/* x <= y + c */
namespace leq {
  Clause *leq_propagate(Solver &s, cspvar _x, cspvar _y, int _c,
                        vec<Lit>& _reason)
  {
    if( _y.max(s) + _c < _x.min(s) ) { // failure
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _y.r_leq( s, _x.min(s) - _c ));
      Clause *r = Clause_new(_reason);
      s.addInactiveClause(r);
      return r;
    }

    if( _y.min(s) + _c < _x.min(s) ) {
      pushifdef(_reason, _x.r_min(s));
      _y.setminf(s, _x.min(s) - _c, _reason);
    }
    if( _x.max(s) - _c > _y.max(s) ) {
      pushifdef(_reason, _y.r_max(s));
      _x.setmaxf(s, _y.max(s) + _c, _reason);
    }

    return 0L;
  }
}

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
  virtual void clone(Solver& othersolver);
  virtual ostream& print(Solver &s, ostream& os) const;
};

Clause *cons_le::wake(Solver& s, Lit)
{
  _reason.clear();
  return leq::leq_propagate(s, _x, _y, _c, _reason);
}

void cons_le::clone(Solver &other)
{
  cons *con = new cons_le(other, _x, _y, _c);
  other.addConstraint(con);
}

ostream& cons_le::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " <= " << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  return os;
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


/* (x <= y + c) <=> b*/
class cons_leq_re : public cons {
  cspvar _x, _y;
  int _c;
  Lit _b;
  vec<Lit> _reason;
public:
  cons_leq_re(Solver &s,
              cspvar x, cspvar y, int c, Lit b) :
    _x(x), _y(y), _c(c), _b(b)
  {
    s.wake_on_lb(_x, this);
    s.wake_on_ub(_x, this);
    s.wake_on_lb(_y, this);
    s.wake_on_ub(_y, this);
    s.wake_on_lit(var(_b), this);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause *cons_leq_re::wake(Solver &s, Lit p)
{
  domevent pevent = s.event(p);
  _reason.clear();

  if( s.value(_b) == l_True ) {
    _reason.push( ~_b );
    return leq::leq_propagate(s, _x, _y, _c, _reason);
  } else if( s.value(_b) == l_False ) {
    _reason.push( _b );
    return leq::leq_propagate(s, _y, _x, -_c-1, _reason);
  }

  if( _x.min(s) > _y.max(s) + _c ) {
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _y.r_max(s));
    DO_OR_RETURN(s.enqueueFill(~_b, _reason));
  } else if( _x.max(s) <= _y.min(s) + _c ) {
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _y.r_min(s));
    DO_OR_RETURN(s.enqueueFill(_b, _reason));
  }

  return 0L;
}

void cons_leq_re::clone(Solver & other)
{
  cons *con = new cons_leq_re(other, _x, _y, _c, _b);
  other.addConstraint(con);
}

ostream& cons_leq_re::print(Solver &s, ostream& os) const
{
  os << "(" << cspvar_printer(s, _x) << " <= " << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  os << ") <=> " << lit_printer(s, _b);
  return os;
}

ostream& cons_leq_re::printstate(Solver & s, ostream& os) const
{
  print(s, os);
  os << " (with " << cspvar_printer(s, _x) << " in " << domain_as_set(s, _x)
     << ", " << cspvar_printer(s, _y) << " in " << domain_as_set(s, _y)
     << ", " << lit_printer(s, _b)
     << ")";
  return os;
}

void post_leq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  if( s.value(b) == l_True ) {
    post_leq(s, x, y, c);
    return;
  } else if( s.value(b) == l_False ) {
    post_less(s, y, x, -c);
    return;
  }

  if( x.max(s) <= y.min(s) + c ) {
    s.uncheckedEnqueue(b);
    return;
  } else if( x.min(s) > y.max(s) + c ) {
    s.uncheckedEnqueue(~b);
    return;
  }

  cons *con = new cons_leq_re(s, x, y, c, b);
  s.addConstraint(con);
}

void post_leq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  assert( b.min(s) >= 0 && b.max(s) <= 1 );
  if( b.max(s) == 0 ) {
    post_leq_re(s, x, y, c, ~Lit(b.eqi(s, 0)));
    return;
  }

  post_leq_re(s, x, y, c, Lit(b.eqi(s, 1)));
}

void post_less_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  post_leq_re(s, x, y, c-1, b);
}

void post_less_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  post_leq_re(s, x, y, c-1, b);
}

void post_geq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  post_leq_re(s, y, x, -c, b);
}

void post_geq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  post_leq_re(s, y, x, -c, b);
}

void post_gt_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  post_leq_re(s, y, x, -c-1, b);
}

void post_gt_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  post_leq_re(s, y, x, -c-1, b);
}

/* abs: |x| = y + c*/
class cons_abs : public cons {
  cspvar _x, _y;
  int _c;
  vec<Lit> _reason; // cache to avoid the cost of allocation
public:
  cons_abs(Solver &s,
           cspvar x, cspvar y, int c) :
    _x(x), _y(y), _c(c)
  {
    using std::max;
    using std::min;

    // we only wake on dom here, otherwise it is too complicated
    s.wake_on_dom(x, this);
    s.wake_on_dom(y, this);
    _reason.capacity(3); _reason.growTo(2, lit_Undef);

    if( y.min(s) + _c < 0 )
      DO_OR_THROW(_y.setmin(s, 0, NO_REASON));

    if( _y.min(s) + _c > max(abs(x.max(s)), abs(x.min(s))) ||
        _y.max(s) + _c < min(abs(x.max(s)), abs(x.min(s))) )
      throw unsat();

    for(int i = _x.min(s), iend = _x.max(s)+1; i != iend; ++i) {
      if( !_x.indomain(s, i) )
        DO_OR_THROW(wake(s, _x.e_neq(s, i)));
      if( !_y.indomain(s, abs(i)-_c) )
        _x.remove(s, i, NO_REASON);
    }

    for(int i = _y.min(s), iend = _y.max(s)+1; i != iend; ++i) {
      if( !_y.indomain(s, i) )
        DO_OR_THROW(wake(s, _y.e_neq(s, i)));
      if( !_x.indomain(s, i+_c) && !_x.indomain(s, -i-_c) )
        _y.remove(s, i, NO_REASON);
    }
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause* cons_abs::wake(Solver &s, Lit event)
{
  domevent e = s.event(event);
  _reason[0] = ~event;

  if( e.x == _x ) {
    if( _y.indomain(s, abs(e.d)-_c) && !_x.indomain(s, -e.d) ) {
      _reason[1] = _x.r_neq( s, -e.d );
      if( var(_reason[1]) == var_Undef ) {
        _reason[1] = _y.e_neq(s, abs(e.d)-_c);
        return _y.remove(s, abs(e.d)-_c, _reason);
      }
      _reason.push( _y.e_neq( s, abs(e.d)-_c) );
      Clause *confl = _y.remove(s, abs(e.d)-_c, _reason);
      _reason.shrink(1);
      return confl;
    }
  } else {
    if( _x.indomain(s, e.d+_c) ) {
      _reason[1] = _x.e_neq( s, e.d+_c);
      DO_OR_RETURN(_x.remove(s, e.d+_c, _reason));
    }
    if( _x.indomain(s, -e.d-_c) ) {
      _reason[1] = _x.e_neq( s, -e.d-_c );
      DO_OR_RETURN(_x.remove(s, -e.d-_c, _reason));
    }
  }

  return 0L;
}

void cons_abs::clone(Solver &other)
{
  cons *con = new cons_abs(other, _x, _y, _c);
  other.addConstraint(con);
}

ostream& cons_abs::print(Solver &s, ostream& os) const
{
  os << "abs(" << cspvar_printer(s, _x) << ") = "
     << cspvar_printer(s, _y);
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  return os;
}

ostream& cons_abs::printstate(Solver& s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  os << cspvar_printer(s, _x) << " in " << domain_as_range(s, _x) << ", "
     << cspvar_printer(s, _y) << " in " << domain_as_range(s, _y) << ")";
  return os;
}

void post_abs(Solver& s, cspvar v1, cspvar v2, int c)
{
  if( v1.min(s) >= 0 ) {
    post_eq(s, v1, v2, c);
    return;
  }
  if( v1.max(s) <= 0 ) {
    post_neg(s, v2, v1, -c);
    return;
  }
  cons *con = new cons_abs(s, v1, v2, c);
  s.addConstraint(con);
}

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
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

template<size_t N>
int cons_lin_le<N>::vmin(Solver &s, int w, cspvar x)
{
  if( w > 0 ) return w*x.min(s);
  else return w*x.max(s);
}

template<size_t N>
Lit cons_lin_le<N>::litreason(Solver &s, int lb,
                              int w, cspvar x)
{
  if( w > 0 )
    return x.r_geq(s, x.min(s));
  else
    return x.r_leq(s, x.max(s));
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
    if( w > 0 ) {
      int ibound = -gap/w;
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
      int ibound;
      // rounding towards zero is weird
      if( gap < 0 )
        ibound = -gap/w;
      else
        ibound = -(gap-w-1)/w;
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

template<size_t N>
void cons_lin_le<N>::clone(Solver &other)
{
  cons *con = new cons_lin_le<N>(other, _vars, _c);
  other.addConstraint(con);
}

template<size_t N>
ostream& cons_lin_le<N>::print(Solver &s, ostream& os) const
{
  for(size_t i = 0; i != _vars.size(); ++i) {
    if( _vars[i].first == 1 ) {
      if( i != 0 )
        os << " + ";
      os << cspvar_printer(s, _vars[i].second);
    } else if( _vars[i].first == -1 ) {
      if( i != 0 )
        os << " ";
      os << "- " << cspvar_printer(s, _vars[i].second);
    } else if( _vars[i].first > 0 ) {
      if( i != 0 )
        os << " +";
      os << _vars[i].first << "*" << cspvar_printer(s, _vars[i].second);
    }
    else if( _vars[i].first < 0 ) {
      if( i != 0 )
        os << " ";
      os << "- " << -_vars[i].first
         << "*" << cspvar_printer(s, _vars[i].second);
    }
  }
  if( _c > 0 )
    os << " + " << _c;
  else
    os << " - " << -_c;
  os << " <= 0";

  return os;
}

template<size_t N>
ostream& cons_lin_le<N>::printstate(Solver& s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  for(size_t i = 0; i != _vars.size(); ++i) {
    if( i ) os << ", ";
    cspvar x = _vars[i].second;
    os << cspvar_printer(s, x) << " in " << domain_as_range(s, x);
  }
  os << ")";
  return os;
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

void post_lin_leq_right_imp_re(Solver &s, vector<cspvar> const&vars,
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

void post_lin_less_right_imp_re(Solver &s, vector<cspvar> const&vars,
                                vector<int> const &coeff,
                                int c, cspvar b)
{
  post_lin_leq_right_imp_re(s, vars, coeff, c+1, b);
}

void post_lin_leq_left_imp_re(Solver &s,
                              std::vector<cspvar> const&vars,
                              std::vector<int> const &coeff,
                              int c,
                              cspvar b)
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

void post_lin_less_left_imp_re(Solver &s,
                               std::vector<cspvar> const&vars,
                               std::vector<int> const &coeff,
                               int c,
                               cspvar b)
{
  post_lin_leq_left_imp_re(s, vars, coeff, c+1, b);
}

void post_lin_leq_iff_re(Solver &s, std::vector<cspvar> const& vars,
                          std::vector<int> const& coeff,
                          int c, cspvar b)
{
  post_lin_leq_left_imp_re(s, vars, coeff, c, b);
  post_lin_leq_right_imp_re(s, vars, coeff, c, b);
}

void post_lin_less_iff_re(Solver &s, std::vector<cspvar> const& vars,
                           std::vector<int> const& coeff,
                           int c, cspvar b)
{
  post_lin_leq_left_imp_re(s, vars, coeff, c, b);
  post_lin_leq_right_imp_re(s, vars, coeff, c, b);
}

// sum coeff[i]*vars[i] + c = 0
void post_lin_eq(Solver &s,
                 std::vector<cspvar> const& vars,
                 std::vector<int> const& coeff,
                 int c)
{
  post_lin_leq(s, vars, coeff, c);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_leq(s, vars, c1, -c);
}

// sum coeff[i]*vars[i] + c = 0 implies b = 1
void post_lin_eq_right_imp_re(Solver &s,
                              std::vector<cspvar> const& vars,
                              std::vector<int> const& coeff,
                              int c,
                              cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_leq_right_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_leq_right_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps;
  ps.push( ~Lit(b1.eqi(s, 1)) );
  ps.push( ~Lit(b2.eqi(s, 1)) );
  ps.push( Lit(b.eqi(s, 1)) );
  s.addClause(ps);
}

// b = 1 implies sum coeff[i]*vars[i] + c = 0
void post_lin_eq_left_imp_re(Solver &s,
                             std::vector<cspvar> const& vars,
                             std::vector<int> const& coeff,
                             int c,
                             cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_leq_left_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_leq_left_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps;
  ps.push( ~Lit(b1.eqi(s, 1)) );
  ps.push( ~Lit(b2.eqi(s, 1)) );
  ps.push( Lit(b.eqi(s, 1)) );
  s.addClause(ps);
}

// b=1 iff sum coeff[i]*vars[i] + c = 0
//
// L >= 0 => b1, L <= 0 => b2, b <=> b1 /\ b2
void post_lin_eq_iff_re(Solver &s,
                        std::vector<cspvar> const& vars,
                        std::vector<int> const& coeff,
                        int c,
                        cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_leq_right_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_leq_right_imp_re(s, vars, c1, -c, b2);

  vec<Lit> ps1, ps2, ps3;
  ps1.push( ~Lit( b.eqi(s, 1) ) );
  ps1.push( Lit( b1.eqi(s, 1) ) );

  ps2.push( ~Lit( b.eqi(s, 1) ) );
  ps2.push( Lit( b2.eqi(s, 1) ) );

  ps3.push( ~Lit( b1.eqi(s, 1) ) );
  ps3.push( ~Lit( b2.eqi(s, 1) ) );
  ps3.push( Lit( b.eqi(s, 1)) );

  s.addClause(ps1);
  s.addClause(ps2);
  s.addClause(ps3);
}

/* linear inequality: L != 0

   implemented as
   L >= 0 => b1, L <= 0 => b2, not b1 or not b2 */
void post_lin_neq(Solver &s,
                  std::vector<cspvar> const& vars,
                  std::vector<int> const& coeff,
                  int c)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_leq_right_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_leq_right_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps;
  ps.push( ~Lit(b1.eqi(s, 1)) );
  ps.push( ~Lit(b2.eqi(s, 1)) );
  s.addClause(ps);
}

/* L != 0 => b = 1

   implemented as
   L > 0 => b1, L < 0 => b2, b1 or b2 => b
*/
void post_lin_neq_right_imp_re(Solver &s,
                               std::vector<cspvar> const& vars,
                               std::vector<int> const& coeff,
                               int c,
                               cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_less_right_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_less_right_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps;
  ps.push( ~Lit(b1.eqi(s, 1)) );
  ps.push( ~Lit(b2.eqi(s, 1)) );
  ps.push( Lit( b.eqi(s, 1)) );
  s.addClause(ps);
}

/* b = 1 => L != 0

   implemented as
   b1 => L > 0, b2 => L < 0, b => b1 or b2
*/
void post_lin_neq_left_imp_re(Solver &s,
                              std::vector<cspvar> const& vars,
                              std::vector<int> const& coeff,
                              int c,
                              cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_less_left_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_less_left_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps;
  ps.push( Lit(b1.eqi(s, 1)) );
  ps.push( Lit(b2.eqi(s, 1)) );
  ps.push( ~Lit( b.eqi(s, 1)) );
  s.addClause(ps);
}

/* b = 1 <=> L != 0

   implemented as
   b1 => L > 0, b2 => L < 0, b <=> b1 or b2
 */
void post_lin_neq_iff_re(Solver &s,
                         std::vector<cspvar> const& vars,
                         std::vector<int> const& coeff,
                         int c,
                         cspvar b)
{
  cspvar b1 = s.newCSPVar(0,1);
  cspvar b2 = s.newCSPVar(0,1);
  post_lin_less_left_imp_re(s, vars, coeff, c, b1);
  vector<int> c1(coeff);
  for(size_t i = 0; i != vars.size(); ++i)
    c1[i] = -coeff[i];
  post_lin_less_left_imp_re(s, vars, c1, -c, b2);
  vec<Lit> ps1, ps2, ps3;
  ps1.push( Lit(b1.eqi(s, 1)) );
  ps1.push( Lit(b2.eqi(s, 1)) );
  ps1.push( ~Lit( b.eqi(s, 1)) );

  ps2.push( ~Lit(b2.eqi(s, 1)) );
  ps2.push( Lit( b.eqi(s, 1)) );

  ps3.push( ~Lit(b2.eqi(s, 1)) );
  ps3.push( Lit( b.eqi(s, 1)) );

  s.addClause(ps1);
  s.addClause(ps2);
  s.addClause(ps3);
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
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
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

void cons_pb::clone(Solver& other)
{
  vector<Var> v;
  vector<int> w;
  for(size_t i = 0; i != _svars.size(); ++i) {
    v.push_back(_svars[i].second);
    w.push_back(_svars[i].first);
  }
  cons *con = new cons_pb(other, v, w, _lb);
  other.addConstraint(con);
}

ostream& cons_pb::print(Solver &s, ostream& os) const
{
  for(size_t i = 0; i != _svars.size(); ++i) {
    if( _svars[i].first == 1 ) {
      if( i != 0 )
        os << " + ";
      os << var_printer(s, _svars[i].second);
    } else if( _svars[i].first == -1 ) {
      if( i != 0 )
        os << " ";
      os << "- " << var_printer(s, _svars[i].second);
    } else if( _svars[i].first > 0 ) {
      if( i != 0 )
        os << " +";
      os << _svars[i].first << "*" << var_printer(s, _svars[i].second);
    }
    else if( _svars[i].first < 0 ) {
      if( i != 0 )
        os << " ";
      os << "- " << -_svars[i].first << "*"
         << var_printer(s, _svars[i].second);
    }
  }
  os << " >= " << _lb;
  return os;
}

ostream& cons_pb::printstate(Solver& s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  for(size_t i = 0; i != _svars.size(); ++i) {
    if( i ) os << ", ";
    Var x = _svars[i].second;
    int xmin = 0, xmax = 1;
    if( s.value(x) == l_True ) xmin = 1;
    else if( s.value(x) == l_False ) xmax = 0;
    os << var_printer(s, x) << " in [" << xmin << ", " << xmax << "]";
  }
  os << ")";
  return os;
}

/* pseudoboolean with a var on the rhs: sum w[i]*v[i] = rhs */
class cons_pbvar : public cons {
  std::vector< std::pair<int, Var> > _posvars;
  std::vector< std::pair<int, Var> > _negvars;
  std::vector< std::pair<int, Var> > _svars; // sorted by absolute value
  int _c;
  cspvar _rhs;
  vec<Lit> _ubreason, _lbreason;
public:
  cons_pbvar(Solver &s,
             std::vector< std::pair<int, Var> > const &vars,
             int c, cspvar rhs)
    : _c(c), _rhs(rhs)
  {
    size_t n = vars.size();
    int lb = _c, ub = _c;
    for(size_t i = 0; i != n; ++i) {
      if( vars[i].first > 0 ) {
        _posvars.push_back( vars[i] );
        ub += vars[i].first;
      }
      else if( vars[i].first < 0 ) {
        _negvars.push_back( vars[i] );
        lb += vars[i].first;
      }
      else
        continue;
      _svars.push_back( vars[i] );
    }
    sort(_svars.begin(), _svars.end(), pb::compare_abs_weights());

    _rhs.setmin(s, lb, NO_REASON);
    _rhs.setmax(s, ub, NO_REASON);

    _ubreason.growTo(n+1);
    _lbreason.growTo(n+1);

    // wake on every assignment
    n = _svars.size();
    for(size_t i = 0; i != n; ++i)
      s.wake_on_lit(_svars[i].second, this);
    s.wake_on_lb(_rhs, this);
    s.wake_on_ub(_rhs, this);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause *cons_pbvar::wake(Solver &s, Lit)
{
  size_t np = _posvars.size(),
    nn = _negvars.size(),
    n = np+nn;

  int lb = _c, ub = _c;

  _ubreason.growTo(n+1);
  _lbreason.growTo(n+1);

  size_t ubi = 0, lbi = 0;
  for(size_t i = 0; i != np; ++i) {
    pair<int, Var> const& cv = _posvars[i];
    int q = toInt(s.value(cv.second));
    if( q >= 0 )
      ub += cv.first;
    else
      _ubreason[ubi++] = Lit(cv.second);
    if( q > 0 ) {
      lb += cv.first;
      _lbreason[lbi++] = Lit(cv.second, true);
    }
  }
  for(size_t i = 0; i != nn; ++i) {
    pair<int, Var> const& cv = _negvars[i];
    int q = toInt(s.value(cv.second));
    if( q >= 0 )
      lb += cv.first;
    else
      _lbreason[lbi++] = Lit(cv.second);
    if( q > 0 ) {
      ub += cv.first;
      _ubreason[ubi++] = Lit(cv.second, true);
    }
  }

  bool nonimpliedlb = false,
    nonimpliedub = false;
  if( _rhs.min(s) > lb )
    nonimpliedlb = true;
  if( _rhs.max(s) < ub )
    nonimpliedub = true;

  _lbreason.shrink(n - lbi + 1);
  DO_OR_RETURN(_rhs.setminf(s, lb, _lbreason));
  _ubreason.shrink(n - ubi + 1);
  DO_OR_RETURN(_rhs.setmaxf(s, ub, _ubreason));

  for(int i = 0; i != _ubreason.size(); ++i)
    _lbreason.push(_ubreason[i]);

  int rhslb = _rhs.min(s);
  int rhsub = _rhs.max(s);

  // note we gather everything in _lbreason now
  if( nonimpliedlb )
    _lbreason.push( _rhs.r_min(s) );
  if( nonimpliedub )
    _lbreason.push( _rhs.r_max(s) );

  for(size_t i = 0; i != np; ++i) {
    pair<int, Var> const& cv = _posvars[i];
    int q = toInt(s.value(cv.second));
    if( q ) continue;
    if( lb + cv.first > rhsub )
      DO_OR_RETURN(s.enqueueFill( Lit( cv.second, true ), _lbreason ));
    if( ub - cv.first < rhslb )
      DO_OR_RETURN(s.enqueueFill( Lit( cv.second ), _lbreason ));
  }
  for(size_t i = 0; i != nn; ++i) {
    pair<int, Var> const& cv = _negvars[i];
    int q = toInt(s.value(cv.second));
    if( q ) continue;
    if( ub + cv.first < rhslb )
      DO_OR_RETURN(s.enqueueFill( Lit(cv.second, true ), _lbreason ));
    if( lb - cv.first > rhsub )
      DO_OR_RETURN(s.enqueueFill( Lit(cv.second ), _lbreason ));
  }

  return 0L;
}

void cons_pbvar::clone(Solver &other)
{
  cons *con = new cons_pbvar(other, _svars, _c, _rhs);
  other.addConstraint(con);
}

ostream& cons_pbvar::print(Solver &s, ostream& os) const
{
  for(size_t i = 0; i != _svars.size(); ++i) {
    if( _svars[i].first == 1 ) {
      if( i != 0 )
        os << " + ";
      os << var_printer(s, _svars[i].second);
    } else if( _svars[i].first == -1 ) {
      if( i != 0 )
        os << " ";
      os << "- " << var_printer(s, _svars[i].second);
    } else if( _svars[i].first > 0 ) {
      if( i != 0 )
        os << " +";
      os << _svars[i].first << "*" << var_printer(s, _svars[i].second);
    }
    else if( _svars[i].first < 0 ) {
      if( i != 0 )
        os << " ";
      os << "- " << -_svars[i].first
         << "*" << var_printer(s, _svars[i].second);
    }
  }
  os << " = " << cspvar_printer(s, _rhs);

  return os;
}

ostream& cons_pbvar::printstate(Solver& s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  for(size_t i = 0; i != _svars.size(); ++i) {
    if( i ) os << ", ";
    Var x = _svars[i].second;
    int xmin = 0, xmax = 1;
    if( s.value(x) == l_True ) xmin = 1;
    else if( s.value(x) == l_False ) xmax = 0;
    os << var_printer(s, x) << " in [" << xmin << ", " << xmax << "]";
  }
  os << ", " << cspvar_printer(s, _rhs)
     << " in " << domain_as_range(s, _rhs);
  os << ")";
  return os;
}

void post_pb(Solver &s, std::vector<Var> const& vars,
             std::vector<int> const& weights, int c, cspvar rhs)
{
  vector< pair< int, Var> > vbool;
  vbool.reserve(vars.size());
  for(size_t i = 0; i != vars.size(); ++i) {
    if( s.value(vars[i]) == l_True ) {
      c += weights[i];
      continue;
    } else if( s.value(vars[i]) == l_False ) {
      continue;
    }
    vbool.push_back( make_pair( weights[i], vars[i] ));
  }

  cons *con = new cons_pbvar(s, vbool, c, rhs);
  s.addConstraint(con);
}

void post_pb(Solver& s, std::vector<cspvar> const& vars,
             std::vector<int> const& weights, int c, cspvar rhs)
{
  vector< pair< int, Var> > vbool;
  vbool.reserve(vars.size());
  for(size_t i = 0; i != vars.size(); ++i) {
    if( vars[i].min(s) == 1 ) {
      c += weights[i];
      continue;
    } else if( vars[i].max(s) == 0 ) {
      continue;
    }
    vbool.push_back( make_pair( weights[i], vars[i].eqi(s, 1)));
  }

  cons *con = new cons_pbvar(s, vbool, c, rhs);
  s.addConstraint(con);
}

/* x = y*z */
class cons_mult : public cons {
  cspvar _x, _y, _z;
  vec<Lit> _reason;

  // divide and round up
  int divup(int d, int n) {
    if( d > 0 ) {
      if( n > 0 )
        return (d+n-1)/n;
      else
        return d/n;
    } else {
      if( n > 0 )
        return d/n;
      else
        return (d-n-1)/n;
    }
  }
  // divide and round down
  int divdn(int d, int n) {
    if( d > 0 ) {
      if( n > 0 )
        return d/n;
      else
        return (d-n-1)/n;
    } else {
      if( n > 0 )
        return (d-n+1)/n;
      else
        return d/n;
    }
  }
public:
  cons_mult(Solver &s, cspvar x, cspvar y, cspvar z) :
    _x(x), _y(y), _z(z)
  {
    s.wake_on_lb(_x, this);
    s.wake_on_ub(_x, this);
    s.wake_on_lb(_y, this);
    s.wake_on_ub(_y, this);
    s.wake_on_lb(_z, this);
    s.wake_on_ub(_z, this);

    _reason.capacity(5);
    wake(s, lit_Undef);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause* cons_mult::wake(Solver &s, Lit)
{
  using std::min;
  using std::max;
  if( _y.min(s) > 0 && _z.min(s) > 0 ) {
    _reason.clear();
    pushifdef(_reason, _z.r_min(s));
    pushifdef(_reason, _y.r_min(s));
    DO_OR_RETURN(_x.setminf( s, _y.min(s)*_z.min(s), _reason ));

    _reason.clear();
    pushifdef(_reason, _z.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    DO_OR_RETURN(_x.setmaxf( s, _y.max(s)*_z.max(s), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _z.r_max(s));
    DO_OR_RETURN(_y.setminf( s, divup(_x.min(s), _z.max(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _z.r_min(s));
    DO_OR_RETURN(_y.setmaxf( s, divdn(_x.max(s), _z.min(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _y.r_max(s));
    DO_OR_RETURN(_z.setminf( s, divup(_x.min(s), _y.max(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _y.r_min(s));
    DO_OR_RETURN(_z.setmaxf( s, divdn(_x.max(s), _y.min(s)), _reason ));
  } if( _y.max(s) < 0 && _z.max(s) < 0 ) {
    _reason.clear();
    pushifdef(_reason, _z.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    DO_OR_RETURN(_x.setminf( s, _y.max(s)*_z.max(s), _reason ));

    _reason.clear();
    pushifdef(_reason, _z.r_min(s));
    pushifdef(_reason, _y.r_min(s));
    DO_OR_RETURN(_x.setmaxf( s, _y.min(s)*_z.min(s), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _z.r_max(s));
    DO_OR_RETURN(_y.setminf( s, divup(_x.max(s), _z.max(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _z.r_min(s));
    DO_OR_RETURN(_y.setmaxf( s, divdn(_x.min(s), _z.min(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    DO_OR_RETURN(_z.setminf( s, divup(_x.max(s), _y.max(s)), _reason ));

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _y.r_min(s));
    DO_OR_RETURN(_z.setmaxf( s, divdn(_x.min(s), _y.min(s)), _reason ));
  } else {
    _reason.clear();
    pushifdef(_reason, _y.r_min(s));
    pushifdef(_reason, _y.r_max(s));
    pushifdef(_reason, _z.r_min(s));
    pushifdef(_reason, _z.r_max(s));
    DO_OR_RETURN(_x.setminf( s, min( min( _y.min(s)*_z.min(s),
                                          _y.min(s)*_z.max(s)),
                                     min( _y.max(s)*_z.min(s),
                                          _y.max(s)*_z.max(s))),
                             _reason));
    DO_OR_RETURN(_x.setmaxf( s, max( max( _y.min(s)*_z.min(s),
                                          _y.min(s)*_z.max(s)),
                                     max( _y.max(s)*_z.min(s),
                                          _y.max(s)*_z.max(s))),
                             _reason));
    if( _y.min(s) > 0 ) {
      _reason.clear();
      pushifdef(_reason, _y.r_min(s));
      pushifdef(_reason, _y.r_max(s));
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _x.r_max(s));
      DO_OR_RETURN(_z.setminf( s, min( min( _x.min(s) / _y.min(s),
                                            _x.min(s) / _y.max(s)),
                                       min( _x.max(s) / _y.min(s),
                                            _x.max(s) / _y.max(s))),
                               _reason));
      DO_OR_RETURN(_z.setmaxf( s, max( max( _x.min(s) / _y.min(s),
                                            _x.min(s) / _y.max(s)),
                                       max( _x.max(s) / _y.min(s),
                                            _x.max(s) / _y.max(s))),
                               _reason));
    }
    if( _z.min(s) > 0 ) {
      _reason.clear();
      pushifdef(_reason, _z.r_min(s));
      pushifdef(_reason, _z.r_max(s));
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _x.r_max(s));
      DO_OR_RETURN(_y.setminf( s, min( min( _x.min(s) / _z.min(s),
                                            _x.min(s) / _z.max(s)),
                                       min( _x.max(s) / _z.min(s),
                                            _x.max(s) / _z.max(s))),
                               _reason));
      DO_OR_RETURN(_y.setmaxf( s, max( max( _x.min(s) / _z.min(s),
                                            _x.min(s) / _z.max(s)),
                                       max( _x.max(s) / _z.min(s),
                                            _x.max(s) / _z.max(s))),
                               _reason));
    }
  }
  return 0L;
}

void cons_mult::clone(Solver& other)
{
  cons *con = new cons_mult(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_mult::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " = "
     << cspvar_printer(s, _y) << "*" << cspvar_printer(s, _z);
  return os;
}

ostream& cons_mult::printstate(Solver &s, ostream& os) const
{
  print(s, os);
  os << " (with " <<  cspvar_printer(s, _x)
     << " in " << domain_as_range(s, _x)
     << ", " <<   cspvar_printer(s, _y)
     << " in " << domain_as_range(s, _y)
     << ", " <<  cspvar_printer(s, _z)
     << " in " << domain_as_range(s, _z);
  return os;
}

void post_mult(Solver& s, cspvar x, cspvar y, cspvar z)
{
  cons *con = new cons_mult(s, x, y, z);
  s.addConstraint(con);
}

void post_div(Solver& s, cspvar x, cspvar y, cspvar z)
{
  post_mult(s, y, x, z);
}

class cons_mod;

// x = min(y,z)
class cons_min : public cons {
  cspvar _x, _y, _z;
  int _c;
  vec<Lit> _reason;
public:
  cons_min(Solver &s,
           cspvar x, cspvar y, cspvar z) :
    _x(x), _y(y), _z(z)
  {
    s.wake_on_lb(_x, this);
    s.wake_on_ub(_x, this);
    s.wake_on_lb(_y, this);
    s.wake_on_ub(_y, this);
    s.wake_on_lb(_z, this);
    s.wake_on_ub(_z, this);
    wake(s, lit_Undef);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause *cons_min::wake(Solver &s, Lit p)
{
  using std::min;

  domevent pevent = s.event(p);
  _reason.clear();

  {
    PUSH_TEMP(_reason, _y.r_min(s));
    PUSH_TEMP(_reason, _z.r_min(s));
    DO_OR_RETURN(_x.setminf(s, min(_y.min(s), _z.min(s)), _reason));
  }
  {
    PUSH_TEMP(_reason, _y.r_max(s));
    PUSH_TEMP(_reason, _z.r_max(s));
    DO_OR_RETURN(_x.setmaxf(s, min(_y.max(s), _z.max(s)), _reason));
  }

  _reason.clear();
  pushifdef(_reason,_x.r_min(s));
  DO_OR_RETURN(_y.setminf(s, _x.min(s), _reason));
  DO_OR_RETURN(_z.setminf(s, _x.min(s), _reason));

  return 0L;
}

void cons_min::clone(Solver & other)
{
  cons *con = new cons_min(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_min::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " = min("
     << cspvar_printer(s, _y) << ", "
     << cspvar_printer(s, _z) << ")";
  return os;
}

ostream& cons_min::printstate(Solver & s, ostream& os) const
{
  print(s, os);
  os << " (with " << cspvar_printer(s, _x) << " in " << domain_as_set(s, _x)
     << ", " << cspvar_printer(s, _y) << " in " << domain_as_set(s, _y)
     << ", " << cspvar_printer(s, _z) << " in " << domain_as_set(s, _z)
     << ")";
  return os;
}

void post_min(Solver &s, cspvar x, cspvar y, cspvar z)
{
  cons *con = new cons_min(s, x, y, z);
  s.addConstraint(con);
}

// x = max(y,z)
class cons_max : public cons {
  cspvar _x, _y, _z;
  vec<Lit> _reason;
public:
  cons_max(Solver &s,
           cspvar x, cspvar y, cspvar z) :
    _x(x), _y(y), _z(z)
  {
    s.wake_on_lb(_x, this);
    s.wake_on_ub(_x, this);
    s.wake_on_lb(_y, this);
    s.wake_on_ub(_y, this);
    s.wake_on_lb(_z, this);
    s.wake_on_ub(_z, this);
    wake(s, lit_Undef);
  }

  Clause *wake(Solver& s, Lit p);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

Clause *cons_max::wake(Solver &s, Lit p)
{
  using std::max;

  domevent pevent = s.event(p);
  _reason.clear();

  {
    PUSH_TEMP(_reason, _y.r_min(s));
    PUSH_TEMP(_reason, _z.r_min(s));
    DO_OR_RETURN(_x.setminf(s, max(_y.min(s), _z.min(s)), _reason));
  }
  {
    PUSH_TEMP(_reason, _y.r_max(s));
    PUSH_TEMP(_reason, _z.r_max(s));
    DO_OR_RETURN(_x.setmaxf(s, max(_y.max(s), _z.max(s)), _reason));
  }

  _reason.clear();
  pushifdef(_reason,_x.r_max(s));
  DO_OR_RETURN(_y.setmaxf(s, _x.max(s), _reason));
  DO_OR_RETURN(_z.setmaxf(s, _x.max(s), _reason));

  return 0L;
}

void cons_max::clone(Solver & other)
{
  cons *con = new cons_max(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_max::print(Solver &s, ostream& os) const
{
  os << cspvar_printer(s, _x) << " = max("
     << cspvar_printer(s, _y) << ", "
     << cspvar_printer(s, _z) << ")";
  return os;
}

ostream& cons_max::printstate(Solver & s, ostream& os) const
{
  print(s, os);
  os << " (with " << cspvar_printer(s, _x) << " in " << domain_as_set(s, _x)
     << ", " << cspvar_printer(s, _y) << " in " << domain_as_set(s, _y)
     << ", " << cspvar_printer(s, _z) << " in " << domain_as_set(s, _z)
     << ")";
  return os;
}

void post_max(Solver &s, cspvar x, cspvar y, cspvar z)
{
  cons *con = new cons_max(s, x, y, z);
  s.addConstraint(con);
}


/* Element: R = X[I]

   Note that indexing is 1-based here, following flatzinc's element */
namespace element {
  size_t idx(int i, int imin,
             int j, int rmin,
             int n) {
    return (j-rmin)*n + (i - imin);
  }
}

void post_element(Solver &s, cspvar R, cspvar I,
                  vector<cspvar> const& X,
                  int offset)
{
  using std::min;
  using std::max;

  assert(!X.empty());

  I.setmin(s, offset, NO_REASON);
  I.setmax(s, X.size()+offset-1, NO_REASON);

  int imin = I.min(s),
    imax = I.max(s);

  int rmin = X[imin-offset].min(s), rmax = X[imin-offset].max(s);
  for(int i = imin+1, iend = imax+1; i < iend; ++i) {
    rmin = min(rmin, X[i-offset].min(s));
    rmax = max(rmax, X[i-offset].max(s));
  }
  rmin = max(rmin, R.min(s));
  rmax = min(rmax, R.max(s));
  R.setmin(s, rmin, NO_REASON);
  R.setmax(s, rmax, NO_REASON);

  // -I=i -Xi=j R=j
  vec<Lit> ps;
  for(int i = imin; i <= imax; ++i) {
    for(int j = rmin; j <= rmax; ++j) {
      ps.clear();
      Lit xij = Lit(X[i-offset].eqi(s, j));
      if( xij == lit_Undef || s.value(xij) == l_False)
        continue; // true clause
      Lit rj = Lit(R.eqi(s, j));
      if( s.value(rj) == l_True )
        continue; // true clause
      ps.push( ~xij );
      ps.push( ~Lit(I.eqi(s, i)) );
      pushifdef( ps, rj );
      s.addClause(ps);
    }
  }

  int n = imax-imin+1;
  int d = rmax-rmin+1;

  Var yfirst, wfirst;

  yfirst = s.newVar();
  for(int i = 1; i != n*d; ++i)
    s.newVar();

  wfirst = s.newVar();
  for(int i = 1; i != n*d; ++i)
    s.newVar();

  using element::idx;

  vec<Lit> ps1, ps2, ps3;
  // define y_ij <=> R=j or Xi=j
  for(int i = imin; i <= imax; ++i)
    for(int j = rmin; j <= rmax; ++j) {
      ps1.clear(); ps2.clear(); ps3.clear();
      Var yij = yfirst + idx(i, imin, j, rmin, n);
      // R=j y_ij
      pushifdef(ps1, Lit(R.eqi(s, j)));
      ps1.push( Lit(yij) );
      s.addClause(ps1);

      // Xi=j y_ij
      pushifdef(ps2, Lit(X[i-offset].eqi(s, j)));
      ps2.push( Lit(yij) );
      s.addClause(ps2);

      // -y_ij -R=j -Xi=j
      Var rij, xij;
      rij = R.eqi(s, j);
      xij = X[i-offset].eqi(s, j);
      if( rij == var_Undef || xij == var_Undef )
        continue;
      ps3.push( ~Lit(yij) );
      ps3.push( ~Lit(rij) );
      ps3.push( ~Lit(xij) );
      s.addClause(ps3);
    }

  // -y_i1 ... -y_id -I=i
  for(int i = imin; i <= imax; ++i) {
    ps.clear();
    if( !I.indomain(s, i) ) continue;
    ps.push( ~Lit(I.eqi(s, i)));
    for(int j = rmin; j <= rmax; ++j) {
      Var yij = yfirst + idx(i, imin, j, rmin, n);
      ps.push( ~Lit(yij) );
    }
    s.addClause(ps);
  }

  // define w_ij <=> -I=i or -Xi=j
  for(int i = imin; i <= imax; ++i)
    for(int j = rmin; j <= rmax; ++j) {
      Var wij = wfirst + idx(i, imin, j, rmin, n);
      ps1.clear(); ps2.clear(); ps3.clear();
      // I=i, w_ij
      pushifdef( ps1, Lit(I.eqi(s,i)));
      ps1.push( Lit(wij) );
      s.addClause(ps1);

      // Xi=j, w_ij
      pushifdef( ps2, Lit(X[i-offset].eqi(s, j)));
      ps2.push( Lit(wij) );
      s.addClause(ps2);

      // -w_ij, -I=i, -Xi=j
      if( I.indomain(s, i) && X[i-offset].indomain(s, j) ) {
        ps3.push( ~Lit( wij) );
        ps3.push( ~Lit(I.eqi(s, i)));
        ps3.push( ~Lit(X[i-offset].eqi(s, j)));
        s.addClause(ps3);
      }
    }

  // -R=j, -w_1j ..., -wnj
  for(int j = rmin; j <= rmax; ++j) {
    if( !R.indomain(s, j) ) continue;
    ps.clear();
    ps.push( ~Lit(R.eqi(s, j)) );
    for(int i = imin; i <= imax; ++i) {
      Var wij = wfirst + idx(i, imin, j, rmin, n);
      ps.push( ~Lit(wij) );
    }
    s.addClause(ps);
  }
}

class cons_table;

/* Global constraints */

/* All different: each variable gets a distinct value */
class cons_alldiff : public cons
{
  vector<cspvar> _x;
  // the matching, kept and updated between calls. No need to update
  // it on backtracking, whatever we had before is still valid
  vector<int> matching;
  vector<int> matching0; // double buffering
  vec<Lit> reason;

  int umin, umax; // the minimum and maximum values in the universe

  unsigned nmatched;

  typedef pair<bool, int> vertex;

  // all the following are here to avoid allocations
  vector<int> valn;     // all edges, sorted by value
  vector<int> valnlim;  // the first edge of a value
  vector<unsigned char> varfree; // is the var/val used by the matching?
  vector<unsigned char> valfree;
  vector<unsigned char> valvisited, varvisited; // for the BFS in matching,
                                       // whether it is in the stack in
                                       // tarjan's scc
  vector<int> valbackp, varbackp; // back pointers to reconstruct the
                                  // augmenting path
  vector<int> valvisited_toclear;
  vector<int> varvisited_toclear;

  vector<int> varindex, varlowlink;
  vector<int> valindex, vallowlink;

  vector<int> varcomp, valcomp;
  vector< vector< vertex > > components;
  vector< unsigned > comp_numvars;
  vector< vertex > tarjan_stack;

  bool find_initial_matching(Solver& s) {
    nmatched = 0;
    greedy_matching(s);
    return find_matching(s);
  }

  // back edges
  void construct_valn(Solver& s);

  // matching
  void greedy_matching(Solver& s);
  void clear_visited();
  bool find_matching(Solver& s);

  /* SCCs */
  // produce a clause that describes why there is no matching
  void explain_conflict(Solver& s, vec<Lit> &ps);
  // add to ps the reason that q is inconsistent. May require other
  // values to be explained and these are pushed to to_explain.
  void explain_value(Solver &s, int q,
                     vec<Lit>& ps,
                     vector<bool>& explained,
                     vector<int>& to_explain);

  void tarjan_unroll_stack(vertex root);
  // start the dfs from variable var
  void tarjan_dfs_var(Solver& s, size_t var, size_t& index);
  // start the dfs from value val
  void tarjan_dfs_val(Solver& s, int val, size_t& index);
  // reset all structures that were touched by tarjan_*
  void tarjan_clear();
public:
  cons_alldiff(Solver &s, vector<cspvar> const& x) : _x(x)
  {
    static const int idx_undef = std::numeric_limits<int>::max();

    umin = _x[0].min(s);
    umax = _x[0].max(s);
    for(size_t i = 0; i != _x.size(); ++i) {
      umin = std::min( umin, _x[i].min(s) );
      umax = std::max( umax, _x[i].max(s) );
      s.schedule_on_dom(_x[i], this);
      s.wake_on_fix(_x[i], this);
    }

    matching.resize(_x.size(), umin-1);

    varfree.resize(_x.size(), true);
    varvisited.resize(_x.size(), false);
    varbackp.resize(_x.size(), umin-1);
    varindex.resize(_x.size(), idx_undef);
    varlowlink.resize(_x.size(), idx_undef);
    varcomp.resize(_x.size(), idx_undef );

    valfree.resize(umax-umin+1, true);
    valvisited.resize(umax-umin+1, false);
    valbackp.resize(umax-umin+1, -1);
    valindex.resize(umax-umin+1, idx_undef);
    vallowlink.resize(umax-umin+1, idx_undef);
    valcomp.resize(umax-umin+1, idx_undef);

    construct_valn(s);

    if( !find_initial_matching(s) )
      throw unsat();

    std::cout << "Initial matching: ";
    for(size_t i = 0; i != _x.size(); ++i) {
      if( i ) std::cout << ", ";
      std::cout << cspvar_printer(s, _x[i]) << " = "
                << matching[i];
    }
    std::cout << "\n";
  }

  Clause* wake(Solver &s, Lit p);
  Clause *propagate(Solver& s);
  void clone(Solver& other);
  ostream& print(Solver &s, ostream& os) const;
  ostream& printstate(Solver& s, ostream& os) const;
};

void cons_alldiff::clone(Solver& other)
{
  cons *con = new cons_alldiff(other, _x);
  other.addConstraint(con);
}

ostream& cons_alldiff::print(Solver& s, ostream& os) const
{
  os << "alldifferent(";
  for(size_t i = 0; i != _x.size(); ++i) {
    if( i ) os << ", ";
    os << cspvar_printer(s, _x[i]);
  }
  os << ")";
  return os;
}

ostream& cons_alldiff::printstate(Solver&s, ostream& os) const
{
  print(s, os);
  os << " (with ";
  for(size_t i = 0; i != _x.size(); ++i) {
    if( i ) os << ", ";
    os << cspvar_printer(s, _x[i]) << " in " << domain_as_set(s, _x[i]);
  }
  os << ")";
  return os;
}

void cons_alldiff::construct_valn(Solver& s)
{
  valnlim.clear();
  valn.clear();
  valnlim.push_back(0);
  for(int i = umin; i <= umax; ++i) {
    for(size_t j = 0; j != _x.size(); ++j)
      if(_x[j].indomain(s, i))
        valn.push_back(j);
    valnlim.push_back(valn.size());
  }
}

void cons_alldiff::greedy_matching(Solver &s)
{
  std::cout << "before greedy, Matching: ";
  for(size_t i = 0; i != _x.size(); ++i) {
    if( i ) std::cout << ", ";
    std::cout << cspvar_printer(s, _x[i]) << " = "
              << matching[i];
  }
  std::cout << "\n";
  const size_t n = _x.size();
  for(size_t i = 0; i != n; ++i) {
    for(int q = _x[i].min(s); q <= _x[i].max(s); ++q) {
      if( valfree[q-umin] ) {
        valfree[q-umin] = false;
        varfree[i] = false;
        matching[i] = q;
        ++nmatched;
        break;
      }
    }
  }
  std::cout << "greedily matched " << nmatched << " variables\n";
  std::cout << "Matching: ";
  for(size_t i = 0; i != _x.size(); ++i) {
    if( i ) std::cout << ", ";
    std::cout << cspvar_printer(s, _x[i]) << " = "
              << matching[i];
  }
  std::cout << "\n";
}

void cons_alldiff::clear_visited()
{
  for(size_t i = 0; i != valvisited_toclear.size(); ++i)
    valvisited[ valvisited_toclear[i]-umin ] = false;
  for(size_t i = 0; i != varvisited_toclear.size(); ++i)
    varvisited[ varvisited_toclear[i] ] = false;
  valvisited_toclear.clear();
  varvisited_toclear.clear();
}

bool cons_alldiff::find_matching(Solver &s)
{
  const size_t n = _x.size();
  while( nmatched < n ) {
    // find a free variable
    size_t fvar;
    for(fvar = 0; !varfree[fvar]; ++fvar)
      ;
    // do a bfs for an augmenting path
    std::list<int> varfrontier;
    std::list<int> valfrontier;
    varfrontier.push_back(fvar);
    int pathend = umin-1;
    do {
      while( !varfrontier.empty() ) {
        size_t var = varfrontier.front();
        varfrontier.pop_front();
        int qend = _x[var].max(s);
        for(int q = _x[var].min(s); q <= qend ; ++q) {
          if( !_x[var].indomain(s, q) ) continue;
          if( matching[var] == q )
            continue;
          if( valvisited[q-umin] )
            continue;
          valbackp[q-umin] = var;
          if( valfree[q-umin] ) {
            pathend = q;
            goto augment_path;
          }
          valfrontier.push_back(q);
          valvisited[q-umin] = true;
          valvisited_toclear.push_back(q);
          std::cout << "\tedge (x" << var << ", " << q << ")\n";
        }
      }
      while( !valfrontier.empty() ) {
        int q = valfrontier.front();
        valfrontier.pop_front();
        for(size_t vni = valnlim[q-umin], vniend = valnlim[q+1-umin];
            vni != vniend; ++vni) {
          size_t var = valn[ vni ];
          if( matching[var] != q )
            continue;
          if( varvisited[var] )
            continue;
          varfrontier.push_back(var);
          varbackp[var] = q;
          varvisited[var] = true;
          varvisited_toclear.push_back(var);
          std::cout << "\tedge (" << q << ", x" << var << ")\n";
        }
      }
    } while( !varfrontier.empty() );
    clear_visited();
    return false;
  augment_path:
    valfree[pathend-umin] = false;
    int val = pathend;
    int var = valbackp[pathend-umin];
    matching[var] = pathend;
    while( !varfree[var] ) {
      val = varbackp[var];
      var = valbackp[val-umin];
      matching[var] = val;
    }
    ++nmatched;
    varfree[fvar] = false;
    clear_visited();
  }
  return true;
}

void cons_alldiff::explain_value(Solver &s, int q,
                                 vec<Lit>& ps,
                                 vector<bool>& explained,
                                 vector<int>& to_explain)
{
  vector<bool> hallvals( umax-umin+1, false );
  int minhval = umax, maxhval = umin;
  int scc = valcomp[q - umin];
  assert( 2*comp_numvars[scc] == components[scc].size() );

  std::cout << q << " in component {";
  for(size_t i = 0; i != components[scc].size(); ++i) {
    if( i ) std::cout << ", ";
    vertex vx = components[scc][i];
    if( vx.first ) std::cout << cspvar_printer(s, _x[vx.second]);
    else std::cout << vx.second;
  }
  std::cout << "}\n";

  // first gather the values of the Hall set
  for(size_t i = 0; i != components[scc].size(); ++i) {
    vertex vx = components[scc][i];
    if( vx.first ) continue; // var
    hallvals[vx.second-umin] = true;
    explained[vx.second-umin] = true;
    minhval = std::min(minhval, vx.second);
    maxhval = std::max(maxhval, vx.second);
  }
  // now describe that the variables in the SCC have been reduced to
  // form a Hall set
  for(size_t i = 0; i != components[scc].size(); ++i) {
    vertex vx = components[scc][i];
    if( !vx.first ) continue; // val
    cspvar v2 = _x[vx.second];
    pushifdef( ps, v2.r_geq(s, minhval) );
    pushifdef( ps, v2.r_leq(s, maxhval) );
    for(int p = minhval+1; p < maxhval; ++p)
      if( !hallvals[p - umin] ) {
        if( v2.indomain(s, p) )
          to_explain.push_back(p);
        else
          ps.push( v2.r_neq(s, p) );
      }
  }
}

void cons_alldiff::explain_conflict(Solver &s, vec<Lit>& ps)
{
  std::cout << "explaining conflict\n";
  // find a free var...
  size_t fvar;
  for(fvar = 0; !varfree[fvar]; ++fvar)
    ;
  // ... and all SCCs reachable from it ...
  size_t index = 0;
  tarjan_dfs_var(s, fvar, index);
  cspvar v = _x[fvar];

  std::cout << "free var is " << cspvar_printer(s, v) << "\n";

  ps.clear();
  // describe the domain of fvar
  pushifdef( ps, v.r_min(s) );
  pushifdef( ps, v.r_max(s) );
  for(int q = v.min(s)+1; q < v.max(s); ++q)
    if( !v.indomain(s, q) )
      ps.push( v.r_neq(s, q) );

  // and all the Hall sets that remove the rest of the values
  vector<bool> explained( umax-umin+1, false );
  vector<int> to_explain;
  for(int q = v.min(s); q <= v.max(s); ++q) {
    if( !v.indomain(s, q) ) continue;
    if( explained[q-umin] ) continue;
    explain_value(s, q, ps, explained, to_explain);
  }

  // note in this loop that to_explain.size() might change in the body
  // of the loop
  for(size_t i = 0; i != to_explain.size(); ++i) {
    if( explained[ to_explain[i] - umin ] ) continue;
    explain_value(s, to_explain[i], ps, explained, to_explain);
  }

  tarjan_clear();
}

void cons_alldiff::tarjan_unroll_stack(vertex root)
{
  int scc = components.size();
  components.push_back( vector<vertex>() );
  comp_numvars.push_back( 0 );
  vector<vertex> & comp = components.back();
  unsigned & numvars = comp_numvars.back();
  vertex vp;
  do {
    vp = tarjan_stack.back(); tarjan_stack.pop_back();
    comp.push_back(vp);
    if( vp.first ) {
      varcomp[vp.second] = scc;
      varvisited[vp.second] = false;
      ++numvars;
    } else {
      valcomp[vp.second-umin] = scc;
      valvisited[vp.second-umin] = false;
    }
  } while( vp != root );
}

void cons_alldiff::tarjan_clear()
{
  static const int idx_undef = std::numeric_limits<int>::max();

  for(size_t i = 0; i != varvisited_toclear.size(); ++i) {
    int var = varvisited_toclear[i];
    varindex[var] = idx_undef;
    varlowlink[var] = idx_undef;
    varcomp[var] = idx_undef;
  }
  for(size_t i = 0; i != valvisited_toclear.size(); ++i) {
    int val = valvisited_toclear[i];
    valindex[val - umin] = idx_undef;
    vallowlink[val - umin] = idx_undef;
    valcomp[val - umin] = idx_undef;
  }
  valvisited_toclear.clear();
  varvisited_toclear.clear();
  components.clear();
  comp_numvars.clear();
}

void cons_alldiff::tarjan_dfs_var(Solver &s, size_t var, size_t& index)
{
  static const int idx_undef = std::numeric_limits<int>::max();

  varvisited_toclear.push_back(var);
  varindex[var] = index;
  varlowlink[var] = index;
  ++index;
  tarjan_stack.push_back( make_pair(true, var) );
  for(int q = _x[var].min(s), qend = _x[var].max(s); q <= qend; ++q) {
    if( matching[var] == q || // edge is (q, var), not (var, q)
        !_x[var].indomain(s, q) ) // no edge at all
      continue;
    if( valindex[q-umin] == idx_undef  ) {
      tarjan_dfs_val(s, q, index);
      varlowlink[var] = std::min(varlowlink[var], vallowlink[q-umin]);
    } else if( valvisited[q-umin] ) { // q is in stack
      varlowlink[var] = std::min(varlowlink[var], valindex[q-umin] );
    }
  }
  if( varlowlink[var] == varindex[var] )
    tarjan_unroll_stack( make_pair(true, var) );
}

void cons_alldiff::tarjan_dfs_val(Solver &s, int val, size_t& index)
{
  static const int idx_undef = std::numeric_limits<int>::max();

  valvisited_toclear.push_back(val);
  valindex[val-umin] = index;
  vallowlink[val-umin] = index;
  valvisited[val-umin] = true;
  ++index;
  tarjan_stack.push_back( make_pair(false, val) );
  for(int q = valnlim[val-umin], qend = valnlim[val+1-umin]; q != qend; ++q) {
    int var = valn[q];
    if( matching[var] != val ) // edge is (var, val)
      continue;
    if( varindex[var] == idx_undef  ) {
      tarjan_dfs_var(s, var, index);
      vallowlink[val-umin] = std::min(vallowlink[val-umin], varlowlink[var]);
    } else if( varvisited[var] ) { // var is in stack
      vallowlink[val-umin] = std::min(vallowlink[val-umin], varindex[var] );
    }
  }
  if( vallowlink[val-umin] == valindex[val-umin] )
    tarjan_unroll_stack( make_pair(false, val) );
}

Clause* cons_alldiff::wake(Solver &s, Lit p)
{
  const size_t n = _x.size();
  domevent de = s.event(p);
  if( de.type != domevent::EQ ) return 0L;

  reason.clear();
  reason.push(~p);
  for(size_t i = 0; i != n; ++i) {
    if( de.x == _x[i] ) continue;
    if( !_x[i].indomain(s, de.d) ) continue;
    DO_OR_RETURN(_x[i].removef(s, de.d, reason));
  }
  return 0L;
}

Clause* cons_alldiff::propagate(Solver &s)
{
  std::cout << "propagating " << cons_state_printer(s, *this) << "\n";
  const size_t n = _x.size();
  bool valid = true; // is the current matching still valid?
  for(size_t i = 0; i != n; ++i) {
    if( !_x[i].indomain( s, matching[i] ) ) {
      if(valid) {
        valid = false;
        matching0 = matching; // if we detect failure, we will copy
                              // matching0 back to matching so on
                              // backtracking we will still have a
                              // valid matching
      }
      varfree[i] = true;
      valfree[ matching[i] - umin ] = true;
      matching[i] = umin-1;
      --nmatched;
    }
  }
  if( valid ) {
    std::cout << "Matching: ";
    for(size_t i = 0; i != _x.size(); ++i) {
      if( i ) std::cout << ", ";
      std::cout << cspvar_printer(s, _x[i]) << " = "
                << matching[i];
    }
    std::cout << "\n";
    return 0L;
  }

  construct_valn(s);
  if( find_matching(s) ) {
    std::cout << "Matching: ";
    for(size_t i = 0; i != _x.size(); ++i) {
      if( i ) std::cout << ", ";
      std::cout << cspvar_printer(s, _x[i]) << " = "
                << matching[i];
    }
    std::cout << "\n";
    return 0L;
  }

  std::cout << "Best matching: ";
  for(size_t i = 0; i != _x.size(); ++i) {
    if( i ) std::cout << ", ";
    if( matching[i] >= umin )
      std::cout << cspvar_printer(s, _x[i]) << " = "
                << matching[i];
    else
      std::cout << cspvar_printer(s, _x[i]) << " = "
                << "XX";
  }
  std::cout << "\n";

  reason.clear();
  explain_conflict(s, reason);
  Clause *r = Clause_new(reason);
  s.addInactiveClause(r);
  matching = matching0;
  return r;
}

void post_alldiff(Solver &s, std::vector<cspvar> const &x)
{
  vector<cspvar> xp;
  vector<int> rem;
  for(size_t i = 0; i != x.size(); ++i) {
    if( x[i].min(s) == x[i].max(s) ) {
      rem.push_back(x[i].min(s));
    } else
      xp.push_back(x[i]);
  }
  for(size_t i = 0; i != xp.size(); ++i) {
    for(size_t q = 0; q != rem.size(); ++q)
      xp[i].remove(s, q, NO_REASON);
  }
  // FIXME: find disconnected components and post one alldiff for
  // each. or do it dynamically in the propagator

  if( xp.empty() ) return;

  cons *con = new cons_alldiff(s, xp);
  s.addConstraint(con);
}

class cons_gcc;

/* Regular */
namespace regular {
  struct layered_fa {
    vector<transition> d;     // all transitions, ordered by layer
    vector<int> layer_trans;  // an index into the beginning of each
                              // layer in d
    vector<int> layer_states; // the first state of each layer
    vector<int> state_trans;  // index of first transition of each state
    vector<int> state_layer;  // the layer of each state
    int q0;
    set<int> F;

    int nlayers() const { return layer_states.size()-1; }
  };

  struct rejecting {
    typedef bool value_type;
    bool operator()(transition const& t) {
      return t.q1 == 0;
    }
  };

  struct compare_transition {
    typedef bool value_type;
    bool operator()(transition t1, transition t2) {
      // (t1.q0,t1.s,t1.q1) <=lex (t2.q0,t2.s, t2.q1)
      return t1.q0 < t2.q0 ||
        (t1.q0 == t2.q0 && (t1.s < t2.s ||
                            (t1.s == t2.s && t1.q1 <= t2.q1)));
    }
  };

  void unfold(automaton const& aut,
              Solver& solver,
              vector<cspvar> X,
              layered_fa& l)
  {
    // note for a string of length $n$ we need $n+1$ layers
    size_t n = X.size();
    using std::max;
    size_t ns = 0;
    for(size_t i = 0; i != aut.d.size(); ++i) {
      transition const& t = aut.d[i];
      ns = max(ns, max(t.q0, t.q1));
    }

    vector<transition> d = aut.d;
    sort(d.begin(), d.end(), compare_transition());

    // transitions and pointers into the transition table
    for(size_t i = 0; i != n; ++i) {
      // special handling for state 0. we assume it is at layer 0
      if( i == 0 )
        l.layer_states.push_back(0);
      else
        l.layer_states.push_back(ns*i+1);
      l.layer_trans.push_back(l.d.size());
      for(size_t j = 0; j != aut.d.size(); ++j) {
        transition const& t = aut.d[j];
        if( t.q0 == 0 || t.q1 == 0 ) continue;
        if( !X[i].indomain(solver, t.s) ) continue;
        if( j == 0 || t.q0 != aut.d[j-1].q0 ) {
          while( l.state_trans.size() < (unsigned)t.q0 ) {
            l.state_trans.push_back(l.d.size() );
            l.state_layer.push_back( i );
          }
          l.state_trans.push_back( l.d.size() );
          l.state_layer.push_back( i );
        }
        l.d.push_back( transition(t.q0+i*ns, t.s, t.q1+(i+1)*ns) );
      }
    }
    // the last layer has all the states but no transitions
    l.layer_states.push_back(ns*n+1);
    l.layer_states.push_back(ns*(n+1)+1);
    l.layer_trans.push_back(l.d.size());
    l.layer_trans.push_back(l.d.size());

    while( l.state_trans.size() <= (n+1)*ns+1 ) {
      l.state_trans.push_back(l.d.size());
      l.state_layer.push_back(l.nlayers()-1);
    }

    // starting/accepting states
    l.q0 = aut.q0;
    for( set<int>::const_iterator i = aut.F.begin(), iend = aut.F.end();
         i != iend; ++i) {
      l.F.insert( *i + n*ns );
    }
  }

  void minimize_internal(layered_fa& l)
  {
    using std::max;
    size_t ns = 0;
    for(size_t i = 0; i != l.d.size(); ++i) {
      transition& t = l.d[i];
      ns = max(ns, max(t.q0, t.q1));
    }

    size_t accepting=0; // all accepting states are merged into one
    vector<int> remap(ns+1);
    // map from set of (s, q1) to all the q0s that have this. In the
    // end we merge all q0s in the same bucket. inefficient, this
    // should be a trie
    typedef std::map< set< pair<int, size_t> >, vector< size_t > > mergemap;
    mergemap tomerge;
    for(int n = l.nlayers()-1; n >= 0; --n) {
      tomerge.clear();
      for(int s = l.layer_states[n]; s != l.layer_states[n+1]; ++s) {
        remap[s] = s;
        set< pair<int, size_t> > out;
        for(int i = l.state_trans[s]; i != l.state_trans[s+1]; ++i) {
          // note we both gather the outgoing (s,q1) of each q0 AND
          // update the transitions to reflect the states we have
          // fixed already in layer n+1
          transition& t = l.d[i];
          t.q1 = remap[t.q1];
          if( t.q1 != 0 )
            out.insert( make_pair( t.s, t.q1 ) );
        }
        if( out.empty() ) { // final layer state or no path to accepting
          if( l.F.find(s) == l.F.end() ) { // final layer rejecting or
                                           // non-final layer and no
                                           // path to accepting
            remap[s] = 0;
            continue;
          } // else final layer accepting state
        }
        tomerge[out].push_back(s);
      }
      for(mergemap::const_iterator i=tomerge.begin(), iend=tomerge.end();
          i != iend; ++i) {
        vector<size_t> const & mergestates = i->second;
        for(vector<size_t>::const_iterator j = mergestates.begin(),
              jend = mergestates.end();
            j != jend; ++j) {
          if( i->first.empty() ) // last layer, therefore accepting
            accepting = mergestates[0];
          remap[*j] = mergestates[0];
        }
      }
    }
    assert( accepting > 0 );
    l.F.clear();
    l.F.insert( accepting );
  }

  void minimize_unfolded(layered_fa& l)
  {
    // FIXME: for now this only minimizes DFAs, but if the automaton
    // is non-deterministic, we should do heuristic minimization as in
    // [KNW CPAIOR09]
    minimize_internal(l);
  }

  void gather_reachable(layered_fa& aut, set<size_t>& r)
  {
    using std::max;
    size_t ns = 0;
    for(size_t i = 0; i != aut.d.size(); ++i) {
      transition& t = aut.d[i];
      ns = max(ns, max(t.q0, t.q1));
    }
    vector<int> remap(ns, 0);

    std::list<size_t> Q;
    Q.push_back(aut.q0);
    r.insert(aut.q0);
    while( !Q.empty() ) {
      size_t s = Q.front();
      Q.pop_front();

      for(int t = aut.state_trans[s]; t != aut.state_trans[s+1]; ++t) {
        transition const& tr = aut.d[t];
        if( tr.q1 == 0 ) continue;
        if( r.find(tr.q1) == r.end() ) {
          r.insert(tr.q1);
          Q.push_back(tr.q1);
        }
      }
    }
  }

  void ensure_var(Solver& s, Var& var)
  {
    if( var == var_Undef )
      var = s.newVar();
  }
};

void post_regular(Solver &s, vector< cspvar > const& vars,
                  regular::automaton const& aut,
                  bool gac)
{
  using namespace regular;
  layered_fa l;
  unfold(aut, s, vars, l);
  minimize_unfolded(l);

  assert(l.F.size() == 1);

  set<size_t> r;
  gather_reachable(l, r);

  if( r.find(*l.F.begin()) == r.end() )
    throw unsat();

  vector<Var> sv(l.state_trans.size(), var_Undef);
  vector<Var> tv(l.d.size(), var_Undef);

  vec<Lit> ps, ps2;

  ensure_var(s, sv[l.q0]);
  ps.push( Lit(sv[l.q0]) );
  s.addClause(ps);

  ps.clear();
  ensure_var(s, sv[*l.F.begin()]);
  ps.push( Lit(sv[*l.F.begin()]) );
  s.addClause(ps);

  for( set<size_t>::iterator si = r.begin(), siend = r.end();
       si != siend; ++si) {
    size_t st = *si;
    size_t layer = l.state_layer[st];
    ensure_var(s, sv[st]);

    if( (unsigned)l.state_layer[st] == vars.size() ) continue;

    ps2.clear();
    ps2.push( ~Lit( sv[st] ) );
    for(int i = l.state_trans[st]; i != l.state_trans[st+1]; ++i) {
      transition tr = l.d[i];
      if( tr.q1 == 0 ) continue;
      assert( tr.q0 == st );
      ensure_var(s, tv[i]);
      ensure_var(s, sv[tr.q1]);

      ps.clear();
      ps.push( ~Lit( tv[i] ) );
      ps.push( Lit( sv[tr.q0] ) );
      s.addClause(ps);

      ps.clear();
      ps.push( ~Lit( tv[i] ) );
      ps.push( Lit( sv[tr.q1] ) );
      s.addClause(ps);

      ps.clear();
      ps.push( ~Lit( tv[i] ) );
      ps.push( Lit( vars[layer].eqi(s, tr.s) ) );
      s.addClause(ps);

      ps2.push( Lit( tv[i] ) );
    }
    s.addClause(ps2);
  }

  if( !gac ) return;
}

