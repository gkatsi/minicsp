#include "solver.hpp"
#include "setcons.hpp"

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
      ps.clear();
      pushifdef( ps, ~Lit( c.ini(s, i) ) );
      ps.push( Lit( a.ini(s, i) ) );
      s.addClause(ps);

      ps.clear();
      pushifdef( ps, Lit( c.ini(s, i) ) );
      ps.push( ~Lit( a.ini(s, i) ) );
      s.addClause(ps);
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
}
