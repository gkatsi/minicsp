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
