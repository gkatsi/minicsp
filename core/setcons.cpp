#include "solver.hpp"
#include "setcons.hpp"
#include "cons.hpp"

void post_setdiff(Solver &s, setvar a, setvar b, setvar c)
{
  vec<Lit> ps;
  for(int i = c.umin(s); i < a.umin(s); ++i)
    c.exclude(s, i, NO_REASON);
  for(int i = a.umax(s)+1; i <= c.umax(s); ++i)
    c.exclude(s, i, NO_REASON);
  for(int i = a.umin(s); i <= a.umax(s); ++i) {
    if( i < b.umin(s) || i > b.umax(s) ) {
      // a.ini(i) <=> c.ini(i)
      if( i >= c.umin(s) && i <= c.umax(s) ) {
        ps.clear();
        ps.push( ~Lit( c.ini(s, i) ) );
        ps.push( Lit( a.ini(s, i) ) );
        s.addClause(ps);

        ps.clear();
        pushifdef( ps, Lit( c.ini(s, i) ) );
        ps.push( ~Lit( a.ini(s, i) ) );
        s.addClause(ps);
      }
    } else {
      // a.ini(i) /\ !b.ini(i) => c.ini(i)
      ps.clear();
      pushifdef( ps, Lit( c.ini(s, i) ) );
      ps.push( Lit( b.ini(s, i) ) );
      ps.push( ~Lit( a.ini(s, i) ) );
      s.addClause(ps);

      // c.ini(i) => a.ini(i) /\ !b.ini(i))
      if( i < c.umin(s) || i > c.umax(s) )
        continue;

      ps.clear();
      ps.push( ~Lit( c.ini(s, i) ) );
      ps.push( Lit( a.ini(s, i) ) );
      s.addClause(ps);

      ps.clear();
      ps.push( ~Lit( c.ini(s, i) ) );
      ps.push( ~Lit( b.ini(s, i) ) );
      s.addClause(ps);
    }
  }
  post_leq(s, c.card(s), a.card(s), 0);
}

void post_seteq(Solver &s, setvar a, setvar b)
{
  post_eq(s, a.card(s), b.card(s), 0);

  for(int i = a.umin(s); i < b.umin(s); ++i)
    a.exclude(s, i, NO_REASON);
  for(int i = b.umax(s)+1; i <= a.umax(s); ++i)
    a.exclude(s, i, NO_REASON);
  for(int i = b.umin(s); i < a.umin(s); ++i)
    b.exclude(s, i, NO_REASON);
  for(int i = a.umax(s)+1; i <= b.umax(s); ++i)
    b.exclude(s, i, NO_REASON);

  vec<Lit> ps;
  for(int i = std::max(a.umin(s), b.umin(s));
      i <= std::min(a.umax(s), b.umax(s)); ++i) {
    ps.clear();
    ps.push( ~Lit( a.ini(s, i) ) );
    ps.push( Lit( b.ini(s, i) ) );
    s.addClause(ps);

    ps.clear();
    ps.push( Lit( a.ini(s, i) ) );
    ps.push( ~Lit( b.ini(s, i) ) );
    s.addClause(ps);
  }
}

namespace setneq {
  void post_unique_used(Solver &s, setvar a, setvar b,
                        vec<Lit>& ps1, vec<Lit>& ps2,
                        Var &onlya)
  {
    if( a.umin(s) < b.umin(s) || a.umax(s) > b.umax(s) ) {
      onlya = s.newVar();
      ps1.push( Lit(onlya) );
      ps1.growTo(2);
      ps2.push( ~Lit(onlya) );
    }

    for(int i = a.umin(s); i < b.umin(s); ++i) {
      ps1[1] =  ~Lit( a.ini(s, i) );
      s.addClause(ps1);
      ps2.push( Lit( a.ini(s, i) ) );
    }
    for(int i = b.umax(s)+1; i <= a.umax(s); ++i) {
      ps1[1] =  ~Lit( a.ini(s, i) );
      s.addClause(ps1);
      ps2.push( Lit( a.ini(s, i) ) );
    }
    if( ps2.size() > 0 )
      s.addClause(ps2);
  }
}

/* We introduce a variable y_i for each element i in both universes
 * such that y_i <=> a.ini(i) != b.ini(i) and variables onlya, onlyb
 * for elements that are only in a's (b's) universe. Finally, a big
 * clause (onlya, onlyb, y_1, ..., y_d) (assuming the shared universe
 * is 1..d)
 *
 * FIXME: if we find an instance that really wants a lot of setneq
 * constraints, check if it is better to post an automaton that
 * asserts at least one of onlya, onlyb, y_1, ..., y_d is
 * true. Propagation on it would be more local...
 */
void post_setneq(Solver &s, setvar a, setvar b)
{
  vec<Lit> ps1, ps2;

  Var onlya = var_Undef, onlyb = var_Undef;

  setneq::post_unique_used(s, a, b, ps1, ps2, onlya);

  ps1.clear();
  ps2.clear();

  setneq::post_unique_used(s, b, a, ps1, ps2, onlyb);

  ps1.clear();
  ps2.clear();

  if( onlya != var_Undef ) ps2.push( Lit(onlya) );
  if( onlyb != var_Undef ) ps2.push( Lit(onlyb) );

  for(int i = std::max(a.umin(s), b.umin(s)),
        iend = std::min(a.umax(s), b.umax(s))+1;
      i != iend; ++i) {
    Var y = s.newVar();
    ps2.push( Lit(y) );

    // y_i => (ai => ~bi)
    ps1.growTo(3);
    ps1[0] = ~Lit(y);
    ps1[1] = ~Lit( a.ini(s, i) );
    ps1[2] = ~Lit( b.ini(s, i) );
    s.addClause(ps1);

    // y_i => (~ai => bi)
    ps1.growTo(3);
    ps1[0] = ~Lit(y);
    ps1[1] = Lit( a.ini(s, i) );
    ps1[2] = Lit( b.ini(s, i) );
    s.addClause(ps1);

    // ~ai /\ bi => y_i
    ps1.growTo(3);
    ps1[0] = Lit(y);
    ps1[1] = Lit( a.ini(s, i) );
    ps1[2] = ~Lit( b.ini(s, i) );
    s.addClause(ps1);

    // ~bi /\ ai => y_i
    ps1.growTo(3);
    ps1[0] = Lit(y);
    ps1[1] = ~Lit( a.ini(s, i) );
    ps1[2] = Lit( b.ini(s, i) );
    s.addClause(ps1);
  }
  s.addClause(ps2);
}

