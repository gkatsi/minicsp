#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include "cons.hpp"
#include "solver.hpp"

using std::ostream;
using std::pair;
using std::make_pair;
using std::vector;

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
          return _x.setmaxf(s, -e.d+_c, _reason);
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
};

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
  virtual ostream& print(ostream& os) const;
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

ostream& cons_neq::print(ostream& os) const
{
  os << _x << " != " << _y;
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
      _reason.push(_x.r_min(s));
      _reason.push(_y.r_max(s));
      return true;
    } else if( _x.max(s) < _y.min(s) + _c ) {
      _reason.push(_x.r_max(s));
      _reason.push(_y.r_min(s));
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
  ostream& print(ostream& os) const;
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
    s.enqueueFill(~_b, _reason);
  } else if( _x.min(s) == _y.min(s)+_c &&
             _x.min(s) == _x.max(s) &&
             _x.min(s) == _y.max(s)+_c ) {
    _reason.clear(); // disjoint_domains() may have put garbage here
    _reason.push(_x.r_eq(s));
    _reason.push(_y.r_eq(s));
    s.enqueueFill(_b, _reason);
  }

  return 0L;
}

void cons_eq_re::clone(Solver & other)
{
  cons *con = new cons_eq_re(other, _x, _y, _c, _b);
  other.addConstraint(con);
}

ostream& cons_eq_re::print(ostream& os) const
{
  os << "(" << _x << " = " << _y;
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  os << ") <=> " << _b;
  return os;
}

ostream& cons_eq_re::printstate(Solver & s, ostream& os) const
{
  print(os);
  os << " (with " << _x << " in " << domain_as_set(s, _x)
     << ", " << _y << " in " << domain_as_set(s, _y)
     << ", " << _b << " in " << ::print(s, _b)
     << ")";
  return os;
}

void post_eqneq_re(Solver &s, cspvar x, cspvar y, int c, Lit b)
{
  vec<Lit> reason;
  if( eq_re::disjoint_domains(s, x, y, c, reason) ) {
    s.uncheckedEnqueue(~b); // will not cause unsat, because b is
                            // unset by if test above
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
  virtual ostream& print(ostream& os) const;
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

ostream& cons_le::print(ostream& os) const
{
  os << _x << " <= " << _y;
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
  ostream& print(ostream& os) const;
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
    _reason.push(_x.r_min(s));
    _reason.push(_y.r_max(s));
    s.enqueueFill(~_b, _reason);
  } else if( _x.max(s) <= _y.min(s) + _c ) {
    _reason.push(_x.r_max(s));
    _reason.push(_y.r_min(s));
    s.enqueueFill(_b, _reason);
  }

  return 0L;
}

void cons_leq_re::clone(Solver & other)
{
  cons *con = new cons_leq_re(other, _x, _y, _c, _b);
  other.addConstraint(con);
}

ostream& cons_leq_re::print(ostream& os) const
{
  os << "(" << _x << " <= " << _y;
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  os << ") <=> " << _b;
  return os;
}

ostream& cons_leq_re::printstate(Solver & s, ostream& os) const
{
  print(os);
  os << " (with " << _x << " in " << domain_as_set(s, _x)
     << ", " << _y << " in " << domain_as_set(s, _y)
     << ", " << _b << " in " << ::print(s, _b)
     << ")";
  return os;
}

void post_leq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  assert( b.min(s) >= 0 && b.max(s) <= 1 );
  if( b.min(s) == 1 ) {
    post_leq(s, x, y, c);
    return;
  } else if( b.max(s) == 0 ) {
    post_less(s, y, x, -c);
    return;
  }

  if( x.max(s) <= y.min(s) + c ) {
    b.setmin(s, 1, NO_REASON);
    return;
  } else if( x.min(s) > y.max(s) + c ) {
    b.setmax(s, 0, NO_REASON);
    return;
  }

  cons *con = new cons_leq_re(s, x, y, c, Lit(b.eqi(s, 1)));
  s.addConstraint(con);
}

void post_less_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  post_leq_re(s, x, y, c-1, b);
}

void post_geq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
{
  post_leq_re(s, y, x, -c, b);
}

void post_gt_re(Solver &s, cspvar x, cspvar y, int c, cspvar b)
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
  ostream& print(ostream& os) const;
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

ostream& cons_abs::print(ostream& os) const
{
  os << "abs(" << _x << ") = " << _y;
  if( _c > 0 )
    os << " + " << _c;
  else if( _c < 0 )
    os << " - " << -_c;
  return os;
}

ostream& cons_abs::printstate(Solver& s, ostream& os) const
{
  print(os);
  os << " (with ";
  os << _x << " in [" << _x.min(s) << ", " << _x.max(s) << "], ";
  os << _y << " in [" << _y.min(s) << ", " << _y.max(s) << "])";
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
  ostream& print(ostream& os) const;
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
ostream& cons_lin_le<N>::print(ostream& os) const
{
  for(size_t i = 0; i != _vars.size(); ++i) {
    if( _vars[i].first == 1 ) {
      if( i != 0 )
        os << " + ";
      os << _vars[i].second;
    } else if( _vars[i].first == -1 ) {
      if( i != 0 )
        os << " ";
      os << "- " << _vars[i].second;
    } else if( _vars[i].first > 0 ) {
      if( i != 0 )
        os << " +";
      os << _vars[i].first << "*" << _vars[i].second;
    }
    else if( _vars[i].first < 0 ) {
      if( i != 0 )
        os << " ";
      os << "- " << -_vars[i].first << "*" << _vars[i].second;
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
  print(os);
  os << " (with ";
  for(size_t i = 0; i != _vars.size(); ++i) {
    if( i ) os << ", ";
    cspvar x = _vars[i].second;
    os << x << " in [" << x.min(s) << ", " << x.max(s) << "]";
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
  ostream& print(ostream& os) const;
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
    _x.setminf( s, _y.min(s)*_z.min(s), _reason );

    _reason.clear();
    pushifdef(_reason, _z.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    _x.setmaxf( s, _y.max(s)*_z.max(s), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _z.r_max(s));
    _y.setminf( s, divup(_x.min(s), _z.max(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _z.r_min(s));
    _y.setmaxf( s, divdn(_x.max(s), _z.min(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _y.r_max(s));
    _z.setminf( s, divup(_x.min(s), _y.max(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _y.r_min(s));
    _z.setmaxf( s, divdn(_x.max(s), _y.min(s)), _reason );
  } if( _y.max(s) < 0 && _z.max(s) < 0 ) {
    _reason.clear();
    pushifdef(_reason, _z.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    _x.setminf( s, _y.max(s)*_z.max(s), _reason );

    _reason.clear();
    pushifdef(_reason, _z.r_min(s));
    pushifdef(_reason, _y.r_min(s));
    _x.setmaxf( s, _y.min(s)*_z.min(s), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _z.r_max(s));
    _y.setminf( s, divup(_x.max(s), _z.max(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _z.r_min(s));
    _y.setmaxf( s, divdn(_x.min(s), _z.min(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_max(s));
    pushifdef(_reason, _y.r_max(s));
    _z.setminf( s, divup(_x.max(s), _y.max(s)), _reason );

    _reason.clear();
    pushifdef(_reason, _x.r_min(s));
    pushifdef(_reason, _y.r_min(s));
    _z.setmaxf( s, divdn(_x.min(s), _y.min(s)), _reason );
  } else {
    _reason.clear();
    pushifdef(_reason, _y.r_min(s));
    pushifdef(_reason, _y.r_max(s));
    pushifdef(_reason, _z.r_min(s));
    pushifdef(_reason, _z.r_max(s));
    _x.setminf( s, min( min( _y.min(s)*_z.min(s),
                             _y.min(s)*_z.max(s)),
                        min( _y.max(s)*_z.min(s),
                             _y.max(s)*_z.max(s))), _reason);
    _x.setmaxf( s, max( max( _y.min(s)*_z.min(s),
                             _y.min(s)*_z.max(s)),
                        max( _y.max(s)*_z.min(s),
                             _y.max(s)*_z.max(s))), _reason);
    if( _y.min(s) > 0 ) {
      _reason.clear();
      pushifdef(_reason, _y.r_min(s));
      pushifdef(_reason, _y.r_max(s));
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _x.r_max(s));
      _z.setminf( s, min( min( _x.min(s) / _y.min(s),
                               _x.min(s) / _y.max(s)),
                          min( _x.max(s) / _y.min(s),
                               _x.max(s) / _y.max(s))), _reason);
      _z.setmaxf( s, max( max( _x.min(s) / _y.min(s),
                               _x.min(s) / _y.max(s)),
                          max( _x.max(s) / _y.min(s),
                               _x.max(s) / _y.max(s))), _reason);
    }
    if( _z.min(s) > 0 ) {
      _reason.clear();
      pushifdef(_reason, _z.r_min(s));
      pushifdef(_reason, _z.r_max(s));
      pushifdef(_reason, _x.r_min(s));
      pushifdef(_reason, _x.r_max(s));
      _y.setminf( s, min( min( _x.min(s) / _z.min(s),
                               _x.min(s) / _z.max(s)),
                          min( _x.max(s) / _z.min(s),
                               _x.max(s) / _z.max(s))), _reason);
      _y.setmaxf( s, max( max( _x.min(s) / _z.min(s),
                               _x.min(s) / _z.max(s)),
                          max( _x.max(s) / _z.min(s),
                               _x.max(s) / _z.max(s))), _reason);
    }
  }
  return 0L;
}

void cons_mult::clone(Solver& other)
{
  cons *con = new cons_mult(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_mult::print(ostream& os) const
{
  os << "_x" << " = " << _y << "*" << _z;
  return os;
}

ostream& cons_mult::printstate(Solver &s, ostream& os) const
{
  print(os);
  os << " (with " <<  _x << " in " << domain_as_range(s, _x)
     << ", " <<   _y << " in " << domain_as_range(s, _y)
     << ", " <<   _z << " in " << domain_as_range(s, _z);
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
  ostream& print(ostream& os) const;
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
    _x.setminf(s, min(_y.min(s), _z.min(s)), _reason);
  }
  {
    PUSH_TEMP(_reason, _y.r_max(s));
    PUSH_TEMP(_reason, _z.r_max(s));
    _x.setmaxf(s, min(_y.max(s), _z.max(s)), _reason);
  }

  _reason.clear();
  pushifdef(_reason,_x.r_min(s));
  _y.setminf(s, _x.min(s), _reason);
  _z.setminf(s, _x.min(s), _reason);

  return 0L;
}

void cons_min::clone(Solver & other)
{
  cons *con = new cons_min(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_min::print(ostream& os) const
{
  os << _x << " = min(" << _y << ", " << _z << ")";
  return os;
}

ostream& cons_min::printstate(Solver & s, ostream& os) const
{
  print(os);
  os << " (with " << _x << " in " << domain_as_set(s, _x)
     << ", " << _y << " in " << domain_as_set(s, _y)
     << ", " << _z << " in " << domain_as_set(s, _z)
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
  int _c;
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
  ostream& print(ostream& os) const;
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
    _x.setminf(s, max(_y.min(s), _z.min(s)), _reason);
  }
  {
    PUSH_TEMP(_reason, _y.r_max(s));
    PUSH_TEMP(_reason, _z.r_max(s));
    _x.setmaxf(s, max(_y.max(s), _z.max(s)), _reason);
  }

  _reason.clear();
  pushifdef(_reason,_x.r_max(s));
  _y.setmaxf(s, _x.max(s), _reason);
  _z.setmaxf(s, _x.max(s), _reason);

  return 0L;
}

void cons_max::clone(Solver & other)
{
  cons *con = new cons_max(other, _x, _y, _z);
  other.addConstraint(con);
}

ostream& cons_max::print(ostream& os) const
{
  os << _x << " = max(" << _y << ", " << _z << ")";
  return os;
}

ostream& cons_max::printstate(Solver & s, ostream& os) const
{
  print(os);
  os << " (with " << _x << " in " << domain_as_set(s, _x)
     << ", " << _y << " in " << domain_as_set(s, _y)
     << ", " << _z << " in " << domain_as_set(s, _z)
     << ")";
  return os;
}

void post_max(Solver &s, cspvar x, cspvar y, cspvar z)
{
  cons *con = new cons_max(s, x, y, z);
  s.addConstraint(con);
}


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
