/***********************************************************************************[SolverTypes.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/


#ifndef SolverTypes_h
#define SolverTypes_h

#include <cassert>
#include <stdint.h>
#include <iostream>

class Solver;

//=================================================================================================
// Variables, literals, lifted booleans, clauses:


// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int Var;
#define var_Undef (-1)


class Lit {
    int     x;
 public:
    Lit() : x(2*var_Undef)                                              { }   // (lit_Undef)
    explicit Lit(Var var, bool sign = false) : x((var+var) + (int)sign) { }

    // Don't use these for constructing/deconstructing literals. Use the normal constructors instead.
    friend int  toInt       (Lit p);  // Guarantees small, positive integers suitable for array indexing.
    friend Lit  toLit       (int i);  // Inverse of 'toInt()'
    friend Lit  operator   ~(Lit p);
    friend bool sign        (Lit p);
    friend int  var         (Lit p);
    friend Lit  unsign      (Lit p);
    friend Lit  id          (Lit p, bool sgn);

    bool operator == (Lit p) const { return x == p.x; }
    bool operator != (Lit p) const { return x != p.x; }
    bool operator <  (Lit p) const { return x < p.x;  } // '<' guarantees that p, ~p are adjacent in the ordering.
};

inline  int  toInt       (Lit p)           { return p.x; }
inline  Lit  toLit       (int i)           { Lit p; p.x = i; return p; }
inline  Lit  operator   ~(Lit p)           { Lit q; q.x = p.x ^ 1; return q; }
inline  bool sign        (Lit p)           { return p.x & 1; }
inline  int  var         (Lit p)           { return p.x >> 1; }
inline  Lit  unsign      (Lit p)           { Lit q; q.x = p.x & ~1; return q; }
inline  Lit  id          (Lit p, bool sgn) { Lit q; q.x = p.x ^ (int)sgn; return q; }

const Lit lit_Undef(var_Undef, false);  // }- Useful special constants.
const Lit lit_Error(var_Undef, true );  // }


//=================================================================================================
// Lifted booleans:


class lbool {
    char     value;
    explicit lbool(int v) : value(v) { }

public:
    lbool()       : value(0) { }
    lbool(bool x) : value((int)x*2-1) { }
    int toInt(void) const { return value; }

    bool  operator == (lbool b) const { return value == b.value; }
    bool  operator != (lbool b) const { return value != b.value; }
    lbool operator ^ (bool b) const { return b ? lbool(-value) : lbool(value); }

    friend int   toInt  (lbool l);
    friend lbool toLbool(int   v);
};
inline int   toInt  (lbool l) { return l.toInt(); }
inline lbool toLbool(int   v) { return lbool(v);  }

const lbool l_True  = toLbool( 1);
const lbool l_False = toLbool(-1);
const lbool l_Undef = toLbool( 0);

//=================================================================================================
// Clause -- a simple class for representing a clause:


class Clause {
    uint32_t size_etc;
    union { float act; uint32_t abst; } extra;
    Lit     data[0];

public:
    void calcAbstraction() {
        uint32_t abstraction = 0;
        for (int i = 0; i < size(); i++)
            abstraction |= 1 << (var(data[i]) & 31);
        extra.abst = abstraction;  }

    // NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
    template<class V>
    Clause(const V& ps, bool learnt) {
        size_etc = (ps.size() << 3) | (uint32_t)learnt;
        for (int i = 0; i < ps.size(); i++) data[i] = ps[i];
        if (learnt) extra.act = 0; else calcAbstraction(); }

    // -- use this function instead:
    template<class V>
    friend Clause* Clause_new(const V& ps, bool learnt = false) {
        assert(sizeof(Lit)      == sizeof(uint32_t));
        assert(sizeof(float)    == sizeof(uint32_t));
        void* mem = malloc(sizeof(Clause) + sizeof(uint32_t)*(ps.size()));
        return new (mem) Clause(ps, learnt); }

    int          size        ()      const   { return size_etc >> 3; }
    void         shrink      (int i)         { assert(i <= size()); size_etc = (((size_etc >> 3) - i) << 3) | (size_etc & 7); }
    void         pop         ()              { shrink(1); }
    bool         learnt      ()      const   { return size_etc & 1; }
    uint32_t     mark        ()      const   { return (size_etc >> 1) & 3; }
    void         mark        (uint32_t m)    { size_etc = (size_etc & ~6) | ((m & 3) << 1); }
    const Lit&   last        ()      const   { return data[size()-1]; }

    // NOTE: somewhat unsafe to change the clause in-place! Must manually call 'calcAbstraction' afterwards for
    //       subsumption operations to behave correctly.
    Lit&         operator [] (int i)         { return data[i]; }
    Lit          operator [] (int i) const   { return data[i]; }
    operator const Lit* (void) const         { return data; }

    float&       activity    ()              { return extra.act; }
    uint32_t     abstraction () const { return extra.abst; }

    Lit          subsumes    (const Clause& other) const;
    void         strengthen  (Lit p);
};

#define INVALID_CLAUSE ((Clause*)0x4)
#define NO_REASON ((Clause*)0L)

/*_________________________________________________________________________________________________
|
|  subsumes : (other : const Clause&)  ->  Lit
|
|  Description:
|       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
|       by subsumption resolution.
|
|    Result:
|       lit_Error  - No subsumption or simplification
|       lit_Undef  - Clause subsumes 'other'
|       p          - The literal p can be deleted from 'other'
|________________________________________________________________________________________________@*/
inline Lit Clause::subsumes(const Clause& other) const
{
    if (other.size() < size() || (extra.abst & ~other.extra.abst) != 0)
        return lit_Error;

    Lit        ret = lit_Undef;
    const Lit* c  = (const Lit*)(*this);
    const Lit* d  = (const Lit*)other;

    for (int i = 0; i < size(); i++) {
        // search for c[i] or ~c[i]
        for (int j = 0; j < other.size(); j++)
            if (c[i] == d[j])
                goto ok;
            else if (ret == lit_Undef && c[i] == ~d[j]){
                ret = c[i];
                goto ok;
            }

        // did not find it
        return lit_Error;
    ok:;
    }

    return ret;
}


inline void Clause::strengthen(Lit p)
{
    remove(*this, p);
    calcAbstraction();
}

/**********************
 *
 * CSP stuff
 *
 **********************/

class Solver;
class cons;

// a container for operations, lightweight enough to pass by value
class cspvar
{
  friend class Solver;
  int _id;
 public:
  cspvar() : _id(-1) {}
  explicit cspvar(int id) : _id(id) {}

  int id() const { return _id; }
  bool valid() const { return _id >= 0; }

  bool indomain(Solver& s, int d) const;
  int min(Solver& s) const;
  int max(Solver& s) const;

  int domsize(Solver& s) const;

  /* the reason can be an existing clause, a clause to be generated
     from a vec<Lit> or a cons, which will lazily be called on to
     generate a clause later, if needed.

     These return a Clause *, which is NULL if the operation does
     not cause a conflict, otherwise a failed clause (i.e. all false
     literals).
  */
  Clause *remove(Solver& s, int d, Clause *c);
  Clause *remove(Solver& s, int d, vec<Lit>& reason);
  Clause *remove(Solver& s, int d, cons *c);

  Clause *removerange(Solver &s, int rb, int re, Clause *c);
  Clause *removerange(Solver &s, int rb, int re, vec<Lit>& reason);
  Clause *removerange(Solver &s, int rb, int re, cons *c);

  Clause *setmin(Solver& s, int m, Clause *c);
  Clause *setmin(Solver& s, int m, vec<Lit>& reason);
  Clause *setmin(Solver& s, int m, cons *c);

  Clause *setmax(Solver& s, int m, Clause *c);
  Clause *setmax(Solver& s, int m, vec<Lit>& reason);
  Clause *setmax(Solver& s, int m, cons *c);

  Clause *assign(Solver& s, int d, Clause *c);
  Clause *assign(Solver& s, int d, vec<Lit>& reason);
  Clause *assign(Solver& s, int d, cons *c);

  /* Generating reasons */
  Var eqi(Solver &s, int d) const;
  Var leqi(Solver &s, int d) const;

  /* Generating reasons, convenience

     prefix r_ means 'reason'
     prefix e_ means 'effect'

     Note when constructing a reason, there can be at most one e_
     literal.
  */
  Lit r_geq(Solver &s, int d) const;
  Lit r_leq(Solver &s, int d) const;
  Lit r_neq(Solver &s, int d) const;
  Lit r_eq(Solver& s, int d) const;

  Lit e_geq(Solver &s, int d) const;
  Lit e_leq(Solver &s, int d) const;
  Lit e_neq(Solver &s, int d) const;
  Lit e_eq(Solver &s, int d) const;
};

class cons
{
  int id;
 public:
  virtual ~cons() {}

  /* propagate, setting a clause or itself as reason for each
   * pruning. In case of failure, return a clause that describes the
   * reason.
   *
   * wake() is executed during propagation of a single variable, while
   * propagate() is a delayed call. */
  virtual Clause *wake(Solver&, Lit);
  virtual Clause *propagate(Solver&);
  /* if this constraint forced a literal and set itself as reason, add
   * to c all literals that explain this pruning.
   */
  virtual void explain(Solver&, Lit p, vec<Lit>& c);

  /* Clone this constraint and post it to othersolver. Useful for
   * debugging clauses */
  virtual void clone(Solver &othersolver);

  /* human readable representation of the constraint for debugging */
  virtual std::ostream& print(std::ostream& os) const;
  /* as above, but also any state (var domains, etc) */
  virtual std::ostream& printstate(Solver& s, std::ostream& os) const;
};

// all the fixed or non-backtracked data of a csp variable
class cspvar_fixed
{
  friend class Solver;

  int omin; // min and max in the *original* domain
  int omax;
  Var firstbool;

  /* a cons may either wake immediately (like an ilog demon or a
     gecode advisor) when we process a literal, or it may be scheduled
     for execution later. */
  vec<cons*>
    wake_on_dom,
    wake_on_lb,
    wake_on_ub,
    wake_on_fix;
  vec<cons*>
    schedule_on_dom,
    schedule_on_lb,
    schedule_on_ub,
    schedule_on_fix;

  vec<Clause*> ps1, ps2, ps3, ps4;

  // accessing the propositional encoding
  bool ind(int i) const { return i >= omin && i <= omax; }
  Var eqi(int i) const { return ind(i)
      ? firstbool+2*(i-omin)
      : var_Undef; }
  Var leqi(int i) const { return ind(i)
      ? firstbool+2*(i-omin)+1
      : var_Undef; }

  void copyclauses(vec<Clause*>& target, vec<Clause*> const& source)
  {
    target.growTo(source.size(), 0L);
    for(int i = 0; i != source.size(); ++i) {
      if( source[i] && source[i] != INVALID_CLAUSE )
        target[i] = Clause_new(*source[i]);
    }
  }
public:
  cspvar_fixed() {}
  cspvar_fixed(cspvar_fixed& f) :
    omin(f.omin), omax(f.omax), firstbool(f.firstbool)
  {
    copyclauses(ps1, f.ps1);
    copyclauses(ps2, f.ps2);
    copyclauses(ps3, f.ps3);
    copyclauses(ps4, f.ps4);
  }
};

// backtracked data of a csp variable
class cspvar_bt
{
  friend class Solver;
  int min;
  int max;
  int dsize;
};

struct consqueue
{
  vec<unsigned char> inq;
  vec<cons*> q;
};

struct domevent
{
  cspvar x;
  enum event_type { NEQ, EQ, GEQ, LEQ, NONE };
  event_type type;
  int d;

  domevent() :  x(0), type(NONE), d(0) {}
  domevent(cspvar px, event_type ptype, int pd) :
     x(px), type(ptype), d(pd) {}
};

inline bool noevent(domevent d) { return d.type == domevent::NONE; }

//==================================================
// output

std::ostream& operator<<(std::ostream& os, cons const&);
std::ostream& operator<<(std::ostream& os, domevent);
std::ostream& operator<<(std::ostream& os, Lit);
std::ostream& operator<<(std::ostream& os, cspvar);

struct cons_printer {
  Solver& _s;
  cons const & _c;
  cons_printer(Solver& s, cons const& c) : _s(s), _c(c) {}
};

inline
std::ostream& operator<<(std::ostream& os, cons_printer cp)
{
  return cp._c.printstate(cp._s, os);
}

struct lit_printer {
  Solver& _s;
  Lit _p;
  lit_printer(Solver& s, Lit p) : _s(s), _p(p) {}
};

std::ostream& operator<<(std::ostream& os, lit_printer lp);

template<typename T>
struct clause_printer {
  Solver& _s;
  T const * _c;
  clause_printer(Solver& s, T const * c) : _s(s), _c(c) {}
};

template<typename T>
inline
std::ostream& operator<<(std::ostream& os, clause_printer<T> cp)
{
  if( !cp._c ) {
    os << "NO_REASON";
    return os;
  }
  os << "( ";
  for(int i = 0; i != cp._c->size(); ++i)
    os << lit_printer(cp._s, (*cp._c)[i]) << " ";
  os << ")@" << cp._c;
  return os;
}

inline
cons_printer print(Solver& s, cons const& c)
{
  return cons_printer(s, c);
}

inline
lit_printer print(Solver& s, Lit p)
{
  return lit_printer(s, p);
}

template<typename T>
inline
clause_printer<T> print(Solver& s, T const* c)
{
  return clause_printer<T>(s, c);
}

//==================================================
// exceptions

// thrown by the constructor of a constraint only
struct unsat : public std::exception
{
};

// thrown when we expect a var to be assigned
struct unassigned_var : public std::exception
{
  Solver const&_s;
  cspvar _x;

  unassigned_var(Solver const& s, cspvar x) : _s(s), _x(x) {}
  const char* what() const throw();
};

//==================================================
// convenience macros/inlines

#define DO_OR_THROW(action) do {                \
    Clause *confl##__LINE__ = action;           \
    if( confl##__LINE__ ) throw unsat();        \
  } while(0)

#define DO_OR_RETURN(action) do {                \
    Clause *confl##__LINE__ = action;            \
    if( confl##__LINE__ ) return confl##__LINE__;\
  } while(0)

template<typename T>
inline
T* throw_if_null(T* t)
{
  if(!t) throw unsat();
  return t;
}

#endif
