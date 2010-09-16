/****************************************************************************************[Solver.C]
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

#include "solver.hpp"
#include "Sort.h"
#include <cmath>
#include <vector>

using namespace std;

//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :

    // Parameters: (formerly in 'SearchParams')
    trace(false)
  , var_decay(1 / 0.95), clause_decay(1 / 0.999), random_var_freq(0.02)
  , restart_first(32), restart_inc(1.5), learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // More parameters:
    //
  , expensive_ccmin  (true)
  , polarity_mode    (polarity_false)
  , verbosity        (0)
  , phase_saving     (true)
    // Statistics: (formerly in 'SolverStats')
    //
  , starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
  , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

  , ok               (true)
  , cla_inc          (1)
  , var_inc          (1)
  , qhead            (0)
  , simpDB_assigns   (-1)
  , simpDB_props     (0)
  , order_heap       (VarOrderLt(activity))
  , random_seed      (91648253)
  , progress_estimate(0)
  , remove_satisfied (true)

    , backtrackable_size(0)
    , backtrackable_cap(0)
    , backtrackable_space(0)
    , current_space(0L)

    , active_constraint(0L)
{}


Solver::~Solver()
{
  for (int i = 0; i < learnts.size(); i++) free(learnts[i]);
  for (int i = 0; i < clauses.size(); i++) free(clauses[i]);
  for (int i = 0; i < conses.size(); ++i) delete conses[i];
  for (int i = 0; i != inactive.size(); ++i) free(inactive[i]);
  for (int i = 0; i != cspvars.size(); ++i) {
    cspvar_fixed & xf = cspvars[i];
    for(int j = 0; j != (xf.omax-xf.omin+1); ++j) {
      free(xf.ps1[j]);
      free(xf.ps2[j]);
      if( j > 0 ) free(xf.ps3[j]);
      free(xf.ps4[j]);
    }
  }
  for(int i = 0; i != backtrackable_space.size(); ++i)
    free(backtrackable_space[i]);
  free(current_space);
}


//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision_var' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches   .push();          // (list for positive literal)
    watches   .push();          // (list for negative literal)

    wakes_on_lit.push();

    reason    .push(NULL);
    assigns   .push(toInt(l_Undef));
    level     .push(-1);
    activity  .push(0);
    seen      .push(0);

    polarity    .push((char)sign);
    decision_var.push((char)dvar);

    phase.push(l_Undef);

    domevent none;
    events.push(none);
    events.push(none);

    insertVarOrder(v);
    return v;
}


cspvar Solver::newCSPVar(int min, int max)
{
  assert(max - min > 0 );

  cspvar x(cspvars.size());

  cspvars.push();
  cspvar_fixed & xf = cspvars.last();

  btptr p = alloc_backtrackable(sizeof(cspvar_bt));
  cspvarbt.push(p);
  cspvar_bt & xbt = deref<cspvar_bt>(p);

  xf.omin = min;
  xf.omax = max;
  xbt.min = min;
  xbt.max = max;
  xbt.dsize = max-min+1;

  // the propositional encoding of the domain
  xf.firstbool = newVar();
  for(int i = 1; i != 2*xbt.dsize; ++i)
    newVar();

  for(int i = xf.omin; i <= xf.omax; ++i) {
    domevent eq(x, domevent::EQ, i),
      neq(x, domevent::NEQ, i),
      leq(x, domevent::LEQ, i),
      geq(x, domevent::GEQ, i+1);
    events[ toInt( Lit(xf.eqi(i) ) ) ] = eq;
    events[ toInt( ~Lit(xf.eqi(i) ) ) ] = neq;
    events[ toInt( Lit(xf.leqi(i) ) ) ] = leq;
    events[ toInt( ~Lit(xf.leqi(i) ) ) ] = geq;
  }

  xf.ps1.growTo(xbt.dsize);
  xf.ps2.growTo(xbt.dsize);
  xf.ps3.growTo(xbt.dsize);
  xf.ps4.growTo(xbt.dsize);

  // (x <= i) => (x <= i+1)
  // (x = i) <=> (x <= i) /\ -(x <= i-1)
  for(int i = 0; i != xbt.dsize; ++i) {
    vec<Lit> ps1, ps2, ps3, ps4;
    ps1.push( ~Lit(xf.leqi(i+xf.omin)) );
    ps1.push( Lit(xf.leqi(i+1+xf.omin)) );
    Clause *c1 = Clause_new(ps1);
    xf.ps1[i] = c1;

    ps2.push( ~Lit(xf.eqi(i+xf.omin)) );
    ps2.push( Lit(xf.leqi(i+xf.omin)) );
    Clause *c2 = Clause_new(ps2);
    xf.ps2[i] = c2;

    if( i > 0 ) {
      ps3.push( ~Lit(xf.eqi(i+xf.omin)) );
      ps3.push( ~Lit(xf.leqi(i-1+xf.omin) ) );
      Clause *c3 = Clause_new(ps3);
      xf.ps3[i] = c3;

      ps4.push( ~Lit(xf.leqi(i+xf.omin)) );
      ps4.push( Lit(xf.leqi(i-1+xf.omin)) );
      ps4.push( Lit(xf.eqi(i+xf.omin)) );
      Clause *c4 = Clause_new(ps4);
      xf.ps4[i] = c4;
    } else {
      xf.ps3[i] = INVALID_CLAUSE;
      ps4.push( ~Lit(xf.leqi(xf.omin)) );
      ps4.push( Lit(xf.eqi(xf.omin)) );
      Clause *c4 =Clause_new(ps4);
      xf.ps4[i] = c4;
    }
  }

  /* x <= omax, so this is true always. We could just simplify the
     rest of the clauses but this seems easier
   */
  x.setmax(*this, xf.omax, (Clause*)0L);

  return x;
}

std::vector<cspvar> Solver::newCSPVarArray(int n, int min, int max)
{
  std::vector<cspvar> rv;
  for(int i = 0; i != n; ++i)
    rv.push_back(newCSPVar(min, max));
  return rv;
}

bool Solver::addClause(vec<Lit>& ps)
{
    assert(decisionLevel() == 0);

    if (!ok)
        return false;
    else{
        // Check if clause is satisfied and remove false/duplicate literals:
        sort(ps);
        Lit p; int i, j;
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
            if (value(ps[i]) == l_True || ps[i] == ~p)
                return true;
            else if (value(ps[i]) != l_False && ps[i] != p)
                ps[j++] = p = ps[i];
        ps.shrink(i - j);
    }

    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1){
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == NULL);
    }else{
        Clause* c = Clause_new(ps, false);
        clauses.push(c);
        attachClause(*c);
    }

    return true;
}

void Solver::addInactiveClause(Clause* c)
{
  inactive.push(c);
}

void Solver::attachClause(Clause& c) {
    assert(c.size() > 1);
    watches[toInt(~c[0])].push(&c);
    watches[toInt(~c[1])].push(&c);
    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size(); }


void Solver::detachClause(Clause& c) {
    assert(c.size() > 1);
    assert(find(watches[toInt(~c[0])], &c));
    assert(find(watches[toInt(~c[1])], &c));
    remove(watches[toInt(~c[0])], &c);
    remove(watches[toInt(~c[1])], &c);
    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size(); }


void Solver::removeClause(Clause& c) {
    detachClause(c);
    free(&c); }


bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }

bool Solver::addConstraint(cons *c)
{
  conses.push(c);
  return true;
}

void Solver::wake_on_lit(Var v, cons *c)
{
  wakes_on_lit[v].push(c);
}

void Solver::wake_on_dom(cspvar x, cons *c)
{
  cspvars[x._id].wake_on_dom.push(c);
}

void Solver::wake_on_lb(cspvar x, cons *c)
{
  cspvars[x._id].wake_on_lb.push(c);
}

void Solver::wake_on_ub(cspvar x, cons *c)
{
  cspvars[x._id].wake_on_ub.push(c);
}

void Solver::wake_on_fix(cspvar x, cons *c)
{
  cspvars[x._id].wake_on_fix.push(c);
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var     x  = var(trail[c]);
            if( phase_saving )
              phase[x]   = toLbool(assigns[x]);
            assigns[x] = toInt(l_Undef);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }
    memcpy(current_space, backtrackable_space[level],
           backtrackable_size);
}

btptr Solver::alloc_backtrackable(unsigned size)
{
  // it is not strictly necessary that we are at level 0, but it is
  // easier to assert this until we find a use case that requires
  // otherwise
  assert(decisionLevel() == 0);
  assert(size > 0);
  btptr p;
  // make sure the allocation is sizeof(int)-aligned
  p.offset = unsigned(ceil(double(backtrackable_size)/sizeof(int)))*sizeof(int);
  backtrackable_size = p.offset+size;
  if( backtrackable_size > backtrackable_cap ) {
    backtrackable_cap = std::max(backtrackable_cap*2,
                                 backtrackable_size);
    current_space = realloc(current_space, backtrackable_cap);
    if( !current_space ) throw std::bad_alloc();

    // free cached allocations
    for(int i = 0; i != backtrackable_space.size(); ++i) {
      free(backtrackable_space[i]);
      backtrackable_space[i] = 0L;
    }
  }
  return p;
}

//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit(int polarity_mode, double random_var_freq)
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty()){
        next = order_heap[irand(random_seed,order_heap.size())];
        if (toLbool(assigns[next]) == l_Undef && decision_var[next])
            rnd_decisions++; }

    // Activity based decision:
    while (next == var_Undef || toLbool(assigns[next]) != l_Undef || !decision_var[next])
        if (order_heap.empty()){
            next = var_Undef;
            break;
        }else
            next = order_heap.removeMin();

    bool sign = false;
    switch (polarity_mode){
    case polarity_true:  sign = false; break;
    case polarity_false: sign = true;  break;
    case polarity_user:  sign = polarity[next]; break;
    case polarity_rnd:   sign = irand(random_seed, 2); break;
    default: assert(false); }

    if( next != var_Undef && phase_saving && phase[next] != l_Undef ) {
      sign = (phase[next] == l_True);
      phase[next] = l_Undef;
    }

    return next == var_Undef ? lit_Undef : Lit(next, sign);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|
|  Effect:
|    Will undo part of the trail, upto but not beyond the assumption of the current decision level.
|________________________________________________________________________________________________@*/
void Solver::analyze(Clause* confl, vec<Lit>& out_learnt, int& out_btlevel)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;

    do{
        assert(confl != NULL);          // (otherwise should be UIP)
        Clause& c = *confl;

        if (c.learnt())
            claBumpActivity(c);

        for (int j = 0; j < c.size(); j++){
            Lit q = c[j];
            if( p == q ) continue;

            if (!seen[var(q)] && level[var(q)] > 0){
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if (level[var(q)] >= decisionLevel())
                    pathC++;
                else{
                    out_learnt.push(q);
                    if (level[var(q)] > out_btlevel)
                        out_btlevel = level[var(q)];
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason[var(p)];
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    if (expensive_ccmin){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason[var(out_learnt[i])] == NULL || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
    }else{
        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++){
            Clause& c = *reason[var(out_learnt[i])];
            for (int k = 1; k < c.size(); k++)
                if (!seen[var(c[k])] && level[var(c[k])] > 0){
                    out_learnt[j++] = out_learnt[i];
                    break; }
        }
    }
    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else{
        int max_i = 1;
        for (int i = 2; i < out_learnt.size(); i++)
            if (level[var(out_learnt[i])] > level[var(out_learnt[max_i])])
                max_i = i;
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level[var(p)];
    }


    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
#ifdef INVARIANTS
    for(int i = 0; i != nVars(); ++i) assert(seen[i] == 0);
#endif
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason[var(analyze_stack.last())] != NULL);
        Clause& c = *reason[var(analyze_stack.last())]; analyze_stack.pop();

        for (int i = 0; i < c.size(); i++){
            if( c[i] == p ) continue;
            Lit p  = c[i];
            if (!seen[var(p)] && level[var(p)] > 0){
                if (reason[var(p)] != NULL && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason[x] == NULL){
                assert(level[x] > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = *reason[x];
                for (int j = 1; j < c.size(); j++)
                    if (level[var(c[j])] > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::debugclause(Clause *from, cons *c)
{
#ifdef EXPENSIVE_INVARIANTS
  Solver s1;
  // add all variables
  int nv = nVars();
  s1.watches.growTo(2*nv);
  s1.wakes_on_lit.growTo(nv);
  s1.reason.growTo(nv, NULL);
  s1.assigns.growTo(nv, toInt(l_Undef));
  s1.level.growTo(nv, -1);
  s1.activity.growTo(nv, 0);
  s1.seen.growTo(nv, 0);

  polarity.copyTo(s1.polarity);
  decision_var.copyTo(s1.decision_var);

  s1.phase.growTo(nv, l_Undef);
  events.copyTo(s1.events);

  cspvars.copyTo(s1.cspvars);
  for(int i = 0; i != cspvars.size(); ++i) {
    cspvar x(i);
    btptr p = s1.alloc_backtrackable(sizeof(cspvar_bt));
    s1.cspvarbt.push(p);
    cspvar_bt & xbt = s1.deref<cspvar_bt>(p);
    xbt.min = cspvars[i].omin;
    xbt.max = cspvars[i].omax;
    xbt.dsize = xbt.max - xbt.min + 1;
    x.setmax(s1, xbt.max, (Clause*)0L);
  }
  s1.propagate();

  c->clone(s1);
  for(int i = 0; i != from->size(); ++i) {
    Lit q = (*from)[i];
    if(s1.value(q) == l_True )
      return; // conflict already
    if( s1.value( q ) != l_False )
      s1.uncheckedEnqueue(~q, 0L);
  }
  Clause *confl = s1.propagate();
  assert(confl);
#endif
}

void Solver::check_debug_solution(Lit p, Clause *from)
{
#ifdef INVARIANTS
    if( debug_solution.empty() ) return;
    domevent pe = events[toInt(p)];
    switch(pe.type) {
    case domevent::NEQ:
      if( pe.d != debug_solution[pe.x._id] ) return;
      break;
    case domevent::EQ:
      if( pe.d == debug_solution[pe.x._id] ) return;
      break;
    case domevent::LEQ:
      if( pe.d >= debug_solution[pe.x._id] ) return;
      break;
    case domevent::GEQ:
      if( pe.d <= debug_solution[pe.x._id] ) return;
      break;
    case domevent::NONE:
      return;
    }
    cout << "\t\tinconsistent with x"
         << pe.x._id << " == " << debug_solution[pe.x._id] << "\n";
    for(int i = 0; i != cspvars.size(); ++i) {
      if( i != pe.x._id &&
          value(cspvars[i].eqi( debug_solution[i] )) == l_False ) {
        cout << "\t\tx" << i << " != " << debug_solution[i] << " already "
             << "at level " << level[cspvars[i].eqi( debug_solution[i] )]
             << "\n";
        cout << "\t\t\tother assignments inconsistent with solution\n";
        return;
      }
    }
    assert( from == NO_REASON && decisionLevel() > 0);
#endif
}

void Solver::uncheckedEnqueue_np(Lit p, Clause *from)
{
    assert(value(p) == l_Undef);
    assigns [var(p)] = toInt(lbool(!sign(p)));  // <<== abstract but not uttermost effecient
    level   [var(p)] = decisionLevel();
    reason  [var(p)] = from;
    trail.push(p);

#ifdef INVARIANTS
    if( from ) {
      bool foundp = false;
      for(int i = 0; i != from->size(); ++i) {
        assert((*from)[i] != lit_Undef);
        if( (*from)[i] != p )
          assert( value((*from)[i]) == l_False );
        else
          foundp = true;
      }
      assert(foundp);
    }
#endif

#ifdef EXPENSIVE_INVARIANTS
    if( active_constraint )
      debugclause(from, active_constraint);
#endif
}

/* We do manual domain updates here, rather than wait for
   propagate(). This ensures that the encoding of the domain is always
   immediately consistent with what propagators ask. For example, if a
   propagator does x.setmin(5), the literals (x <= 4), (x <= 3) etc,
   and (x = 4), (x = 3), etc will all be false immediately with the
   correct reasons.

   A minor advantage of this is that it's probably faster than doing
   unit propagation and does not bring the clauses ps1...ps4 into the
   cache.
 */
void Solver::uncheckedEnqueue(Lit p, Clause* from)
{
    if(trace) {
      domevent const &pevent = events[toInt(p)];
      if( !noevent(pevent) ) {
        if( active_constraint )
          cout << pevent << " forced by "
               << print(*this, *active_constraint);
        else
          cout << pevent << " forced by clause " << print(*this, from);
      } else {
        if( active_constraint )
          cout << p << " forced by "
               << print(*this, *active_constraint);
        else
          cout << p << " forced by clause " << print(*this, from);
      }
      cout << " at level " << decisionLevel() << "\n";
    }

    check_debug_solution(p, from);
    uncheckedEnqueue_np(p, from);

    // update csp var and propagate, if applicable
    domevent const &pevent = events[toInt(p)];
    if( noevent(pevent) ) return;
    cspvar_fixed& xf = cspvars[pevent.x._id];
    cspvar_bt& xb = deref<cspvar_bt>(cspvarbt[pevent.x._id]);
    if( pevent.type == domevent::EQ ) {
      xb.dsize = 1;
      xb.min = xb.max = pevent.d;
      // propagate towards max
      if( value(xf.leqi(pevent.d)) != l_True ) {
        uncheckedEnqueue_np( Lit(xf.leqi(pevent.d)),
                             xf.ps2[pevent.d-xf.omin] );
        if( value(xf.eqi(pevent.d+1)) != l_False)
          uncheckedEnqueue_np( ~Lit(xf.eqi(pevent.d+1)),
                               xf.ps3[pevent.d+1-xf.omin] );
        int leq = pevent.d+1;
        while( leq < xf.omax && value(xf.leqi(leq)) != l_True ) {
          uncheckedEnqueue_np( Lit(xf.leqi(leq)), xf.ps1[leq-1-xf.omin] );
          if( value(xf.eqi(leq+1)) != l_False)
            uncheckedEnqueue_np( ~Lit(xf.eqi(leq+1)), xf.ps3[leq+1-xf.omin] );
          ++leq;
        }
      }
      // propagate towards min
      if( pevent.d > xf.omin &&
          value(xf.leqi(pevent.d-1)) != l_False ) {
        uncheckedEnqueue_np( ~Lit(xf.leqi(pevent.d-1)),
                             xf.ps3[pevent.d-xf.omin] );
        if( value(xf.eqi(pevent.d-1)) != l_False)
          uncheckedEnqueue_np( ~Lit(xf.eqi(pevent.d-1)),
                               xf.ps2[pevent.d-1-xf.omin] );
        int geq = pevent.d-2;
        while( geq >= xf.omin && value(xf.leqi(geq)) != l_False ) {
          uncheckedEnqueue_np( ~Lit(xf.leqi(geq)), xf.ps1[geq-xf.omin] );
          if( value(xf.eqi(geq)) != l_False )
            uncheckedEnqueue_np( ~Lit(xf.eqi(geq)), xf.ps2[geq-xf.omin] );
          --geq;
        }
      }
    }

    if( pevent.type == domevent::NEQ ) {
      --xb.dsize;
      if( pevent.d == xb.min ) {
        while( value(xf.eqi(xb.min)) == l_False ) {
          assert( value(xf.leqi(xb.min)) != l_False &&
                  (xb.min == xf.omin ||
                   value(xf.leqi(xb.min-1)) == l_False ));
          uncheckedEnqueue_np( ~Lit( xf.leqi(xb.min) ),
                               xf.ps4[xb.min-xf.omin] );
          ++xb.min;
        }
      }
      if( pevent.d == xb.max ) {
        while( value(xf.eqi(xb.max)) == l_False ) {
          assert( (xb.max == xf.omax ||
                   value(xf.leqi(xb.max)) == l_True) &&
                  value(xf.leqi(xb.max-1)) != l_False );
          uncheckedEnqueue_np( Lit( xf.leqi(xb.max-1) ),
                               xf.ps4[xb.max-xf.omin] );
          --xb.max;
        }
      }
    }

    if( pevent.type == domevent::GEQ ) {
      int geq = pevent.d-1;
      if( value(xf.eqi(geq)) != l_False ) {
        uncheckedEnqueue_np( ~Lit( xf.eqi(geq)), xf.ps2[geq-xf.omin] );
        --xb.dsize;
      }
      while(geq > xf.omin &&
            value( xf.leqi(geq-1) ) != l_False ) {
        uncheckedEnqueue_np( ~Lit(xf.leqi(geq-1)),
                             xf.ps1[geq-1-xf.omin] );
        if( value(xf.eqi(geq-1)) != l_False ) {
          uncheckedEnqueue_np( ~Lit( xf.eqi(geq-1)), xf.ps2[geq-1-xf.omin] );
          --xb.dsize;
        }
        --geq;
      }
      xb.min = pevent.d;
      while( value(xf.eqi(xb.min)) == l_False ) {
        assert( value(xf.leqi(xb.min)) != l_False &&
                value(xf.leqi(xb.min-1)) == l_False );
        uncheckedEnqueue_np( ~Lit( xf.leqi(xb.min) ),
                             xf.ps4[xb.min-xf.omin] );
        ++xb.min;
      }
    }
    if( pevent.type == domevent::LEQ ) {
      int leq = pevent.d+1;
      if( leq <= xf.omax && value(xf.eqi(leq)) != l_False ) {
        uncheckedEnqueue_np( ~Lit( xf.eqi(leq)), xf.ps3[leq-xf.omin] );
        --xb.dsize;
      }
      while( leq < xf.omax &&
             value( xf.leqi(leq) ) != l_True ) {
        uncheckedEnqueue_np( Lit( xf.leqi(leq) ), xf.ps1[leq-1-xf.omin] );
        if( value(xf.eqi(leq+1)) != l_False ) {
          uncheckedEnqueue_np( ~Lit( xf.eqi(leq+1)), xf.ps3[leq+1-xf.omin] );
          --xb.dsize;
        }
        ++leq;
      }
      xb.max = pevent.d;
      while( value(xf.eqi(xb.max)) == l_False ) {
        assert( value(xf.leqi(xb.max)) == l_True &&
                value(xf.leqi(xb.max-1)) != l_False );
        uncheckedEnqueue_np( Lit( xf.leqi(xb.max-1) ),
                             xf.ps4[xb.max-xf.omin] );
        --xb.max;
      }
    }

    if( xb.max == xb.min &&
        value( xf.eqi(xb.max) ) != l_True )
      uncheckedEnqueue_np( Lit(xf.eqi(xb.max)),
                           xf.ps4[xb.max-xf.omin] );

#ifdef INVARIANTS
    for(int i = xf.omin; i != xf.omax; ++i ) {
      if( value(xf.leqi(i)) == l_False )
        assert( xb.min > i );
      if( value(xf.leqi(i)) == l_True )
        assert( xb.max <= i );
      if( value(xf.eqi(i)) == l_False )
        assert( xb.min != i && xb.max != i );
    }
#endif
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise NULL.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
Clause* Solver::propagate()
{
    Clause* confl     = NULL;
    int     num_props = 0;

    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Clause*>&  ws  = watches[toInt(p)];
        Clause         **i, **j, **end;
        num_props++;

        for (i = j = (Clause**)ws, end = i + ws.size();  i != end;){
            Clause& c = **i++;

            // Make sure the false literal is data[1]:
            Lit false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;

            assert(c[1] == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            Lit first = c[0];
            if (value(first) == l_True){
                *j++ = &c;
            }else{
                // Look for new watch:
                for (int k = 2; k < c.size(); k++)
                    if (value(c[k]) != l_False){
                        c[1] = c[k]; c[k] = false_lit;
                        watches[toInt(~c[1])].push(&c);
                        goto FoundWatch; }

                // Did not find watch -- clause is unit under assignment:
                *j++ = &c;
                if (value(first) == l_False){
                    confl = &c;
                    qhead = trail.size();
                    // Copy the remaining watches:
                    while (i < end)
                        *j++ = *i++;
                }else
                    uncheckedEnqueue(first, &c);
            }
        FoundWatch:;
        }
        ws.shrink(i - j);

        if(confl)
          break;

        /* Now propagate constraints that wake on this literal */
        vec<cons*>& pwakes = wakes_on_lit[var(p)];
        for(cons **ci = &pwakes[0],
              **ciend = ci+pwakes.size();
            ci != ciend; ++ci) {
          active_constraint = *ci;
          confl = (*ci)->wake(*this, p);
          active_constraint = 0L;
          if( confl ) {
            qhead = trail.size();
            break;
          }
        }

        if(confl)
          break;

        domevent const & pe = events[toInt(p)];
        if( noevent(pe) ) continue;
        vec<cons*> *dewakes = 0L;
        switch(pe.type) {
        case domevent::NEQ: dewakes=&(cspvars[pe.x._id].wake_on_dom); break;
        case domevent::EQ: dewakes=&(cspvars[pe.x._id].wake_on_fix); break;
        case domevent::LEQ:
          if( pe.x.max(*this) < pe.d )
            dewakes = 0L;
          else
            dewakes=&(cspvars[pe.x._id].wake_on_ub); break;
        case domevent::GEQ:
          if( pe.x.min(*this) > pe.d )
            dewakes = 0L;
          else
            dewakes=&(cspvars[pe.x._id].wake_on_lb); break;
        case domevent::NONE: assert(0);
        }
        if( !dewakes ) continue;
        for( cons **ci = &((*dewakes)[0]),
               **ciend = ci+dewakes->size();
             ci != ciend; ++ci) {
          active_constraint = *ci;
          confl = (*ci)->wake(*this, p);
          active_constraint = 0L;
          if( confl ) {
            if( trace ) {
              cout << "Constraint " << print(*this, **ci) << " failed, "
                   << "clause @ " << confl << "\n";
            }
            qhead = trail.size();
            break;
          }
        }
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt { bool operator () (Clause* x, Clause* y) { return x->size() > 2 && (y->size() == 2 || x->activity() < y->activity()); } };
void Solver::reduceDB()
{
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt());
    for (i = j = 0; i < learnts.size() / 2; i++){
        if (learnts[i]->size() > 2 && !locked(*learnts[i]))
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    for (; i < learnts.size(); i++){
        if (learnts[i]->size() > 2 && !locked(*learnts[i]) && learnts[i]->activity() < extra_lim)
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
}


void Solver::removeSatisfied(vec<Clause*>& cs)
{
    int i,j;
    for (i = j = 0; i < cs.size(); i++){
        if (satisfied(*cs[i]))
            removeClause(*cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}


/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != NULL)
        return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied)        // Can be turned off.
        removeSatisfied(clauses);

    // Remove fixed variables from the variable heap:
    order_heap.filter(VarFilter(*this));

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (nof_learnts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts, keeping the number of learnt clauses
|    below the provided limit. NOTE! Use negative value for 'nof_conflicts' or 'nof_learnts' to
|    indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts, double* nof_learnts)
{
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;

    starts++;

    bool first = true;

    for (;;){
        Clause* confl = propagate();
        if (confl != NULL){
            // CONFLICT
            conflicts++; conflictC++;
            if (decisionLevel() == 0) return l_False;

            first = false;

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level);
            cancelUntil(backtrack_level);
            assert(value(learnt_clause[0]) == l_Undef);

            if (learnt_clause.size() == 1){
                uncheckedEnqueue(learnt_clause[0]);
            }else{
                Clause* c = Clause_new(learnt_clause, true);
                learnts.push(c);
                attachClause(*c);
                claBumpActivity(*c);

                uncheckedEnqueue(learnt_clause[0], c);
            }

            if(trace) {
              cout << "Conflict " << conflicts
                   << ", backjumping to level " << backtrack_level
                   << " and setting " << print(*this, learnt_clause[0]);
              if( !noevent(event(learnt_clause[0])) ) {
                cspvar x = event(learnt_clause[0]).x;
                cout << ", " << x << " in ["
                     << x.min(*this) << ", " << x.max(*this) << "]";
              }
              cout << "\n";
            }

            varDecayActivity();
            claDecayActivity();

        }else{
            // NO CONFLICT

            /* Increase decision level. We must now call
               newDecisionLevel() immediately after propagate() in
               order to store the changes to backtrackable data made
               by propagation. Otherwise, we might restart next and
               cancelUntil(0) will overwrite our changes. This only
               happens if we restart immediately after learning a unit
               clause.

               This is a new requirement compared to stock minisat
             */
            newDecisionLevel();


            if (nof_conflicts >= 0 && conflictC >= nof_conflicts){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if (*nof_learnts >= 0 && learnts.size()-nAssigns() >= *nof_learnts) {
                // Reduce the set of learnt clauses:
                reduceDB();
                *nof_learnts   *= learntsize_inc;
            }

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True){
                    // Dummy decision level:
                    newDecisionLevel();
                }else if (value(p) == l_False){
                    analyzeFinal(~p, conflict);
                    return l_False;
                }else{
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef){
                // New variable decision:
                decisions++;
                next = pickBranchLit(polarity_mode, random_var_freq);

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            if( trace ) {
              domevent pe = events[toInt(next)];
              cout << "Decision " << decisions << ":";
              if( noevent(pe) ) {
                cout << next;
              } else {
                cout << pe;
              }
              cout << " at level " << decisionLevel() << "\n";
            }

            // enqueue 'next'
            assert(value(next) == l_Undef);
            uncheckedEnqueue(next);
        }
    }
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}


bool Solver::solve(const vec<Lit>& assumps)
{
    model.clear();
    conflict.clear();

    if (!ok) return false;

    assumps.copyTo(assumptions);

    double  nof_conflicts = restart_first;
    double  nof_learnts   = nClauses() * learntsize_factor;
    lbool   status        = l_Undef;

    if (verbosity >= 1){
        reportf("============================[ Search Statistics ]==============================\n");
        reportf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        reportf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        reportf("===============================================================================\n");
    }

    int lubycounter = 0;
    int lubybits = 0;
    int lubymult = 1;

    // Search:
    while (status == l_Undef){
        if (verbosity >= 1)
            reportf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n", (int)conflicts, order_heap.size(), nClauses(), (int)clauses_literals, (int)nof_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progress_estimate*100), fflush(stdout);
        if( !lubybits ) {
          ++lubycounter;
          lubybits = lubycounter ^ (lubycounter-1);
          lubymult = 1;
        }
        //printf("lubymult %d, nof_learnts %f\n", lubymult, nof_learnts);
        nof_conflicts = lubymult * restart_first;
        //nof_conflicts *= restart_inc;
        lubybits >>= 1;
        lubymult <<= 1;
        status = search((int)nof_conflicts, &nof_learnts);
    }

    if (verbosity >= 1)
        reportf("===============================================================================\n");


    if (status == l_True){
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
        cspmodel.growTo(cspvars.size());
        for(int i = 0; i != cspvars.size(); ++i) {
          cspvar_bt& xb = deref<cspvar_bt>(cspvarbt[i]);
          cspmodel[i] = std::make_pair(xb.min, xb.max);
        }
#ifndef NDEBUG
        verifyModel();
#endif
    }else{
        assert(status == l_False);
        if (conflict.size() == 0)
            ok = false;
    }

    cancelUntil(0);
    return status == l_True;
}

//==================================================
// output


ostream& operator<<(ostream& os, cons const& c)
{
  return c.print(os);
}

ostream& operator<<(ostream& os, domevent pevent)
{
  if( noevent(pevent) ) {
    os << "noevent";
    return os;
  }

  const char *op = 0L;
  switch(pevent.type) {
  case domevent::EQ: op = "=="; break;
  case domevent::NEQ: op = "!="; break;
  case domevent::LEQ: op = "<="; break;
  case domevent::GEQ: op = ">="; break;
  case domevent::NONE: op = "#$%#^%"; break;
  }
  os << "x" << pevent.x.id() << ' ' << op << ' '
     << pevent.d;
  return os;
}

ostream& operator<<(ostream& os, Lit p)
{
  os << (sign(p) ? "-" : "") << var(p)+1;
  return os;
}

ostream& operator<<(ostream& os, cspvar x)
{
  os << "x" << x.id();
  return os;
}

//=================================================================================================
// Debug methods:


void Solver::verifyModel()
{
    bool failed = false;
    for (int i = 0; i < clauses.size(); i++){
        assert(clauses[i]->mark() == 0);
        Clause& c = *clauses[i];
        for (int j = 0; j < c.size(); j++)
            if (modelValue(c[j]) == l_True)
                goto next;

        reportf("unsatisfied clause: ");
        printClause(*clauses[i]);
        reportf("\n");
        failed = true;
    next:;
    }

    assert(!failed);

    reportf("Verified %d original clauses.\n", clauses.size());
}


void Solver::checkLiteralCount()
{
    // Check that sizes are calculated correctly:
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (clauses[i]->mark() == 0)
            cnt += clauses[i]->size();

    if ((int)clauses_literals != cnt){
        fprintf(stderr, "literal count: %d, real value = %d\n", (int)clauses_literals, cnt);
        assert((int)clauses_literals == cnt);
    }
}

Clause* cons::wake(Solver&, Lit)
{
  assert(0);
  return 0L;
}

Clause* cons::propagate(Solver&)
{
  assert(0);
  return 0L;
}

void cons::explain(Solver&, Lit, vec<Lit>&)
{
  assert(0);
}

void cons::clone(Solver &other)
{
  assert(0);
}

ostream& cons::print(ostream& os) const
{
  os << "cons@" << this;
  return os;
}

ostream& cons::printstate(Solver&, ostream& os) const
{
  os << "cons@" << this;
  return os;
}
