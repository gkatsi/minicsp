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
#include "cons.hpp"
#include "Sort.h"
#include <cmath>
#include <vector>

using namespace std;

//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :

    // Parameters: (formerly in 'SearchParams')
    trace(false)
  , learning(true)
  , restarting(true)
  , var_decay(1 / 0.95), clause_decay(1 / 0.999), random_var_freq(0.02)
  , restart_first(32), restart_inc(1.5), learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // More parameters:
    //
  , expensive_ccmin  (true)
  , polarity_mode    (polarity_false)
  , verbosity        (0)
  , phase_saving     (true)
  , allow_clause_dbg (true)
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
{
  consqs.growTo(MAX_PRIORITY+2);
  reset_queue();
  prop_queue = &consqs[0];
}


Solver::~Solver()
{
  for (int i = 0; i < learnts.size(); i++) free(learnts[i]);
  for (int i = 0; i < clauses.size(); i++) free(clauses[i]);
  for (int i = 0; i < conses.size(); ++i) conses[i]->dispose();
  for (int i = 0; i != inactive.size(); ++i) free(inactive[i]);
  for (int i = 0; i != cspvars.size(); ++i) {
    cspvar_fixed & xf = cspvars[i];
    if( xf.omax == xf.omin ) continue;
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
    setevent setnone;
    setevents.push(setnone);
    setevents.push(setnone);

    varnames.push_back(std::string());

    insertVarOrder(v);
    return v;
}

cspvar Solver::newCSPVar(int min, int max)
{
  assert(max - min >= 0 );

  bool unary = false;
  if( max == min ) unary = true;

  cspvar x(cspvars.size());

  cspvars.push();
  cspvarnames.push_back(std::string());

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

  if( !unary ) {
    xf.ps1.growTo(xbt.dsize);
    xf.ps2.growTo(xbt.dsize);
    xf.ps3.growTo(xbt.dsize);
    xf.ps4.growTo(xbt.dsize);
  }

  // (x <= i) => (x <= i+1)
  // (x = i) <=> (x <= i) /\ -(x <= i-1)
  for(int i = 0; i != xbt.dsize; ++i) {
    if( unary ) continue;
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
  if( !unary )
    x.setmax(*this, xf.omax, (Clause*)0L);

  /* Unary vars are also hacky. Immediately set eqi(min) = l_True
   */
  if( unary ) {
    uncheckedEnqueue_np(Lit(xf.firstbool+1), NO_REASON);
    uncheckedEnqueue_np(Lit(xf.firstbool), NO_REASON);
  }

  return x;
}

std::vector<cspvar> Solver::newCSPVarArray(int n, int min, int max)
{
  std::vector<cspvar> rv;
  for(int i = 0; i != n; ++i)
    rv.push_back(newCSPVar(min, max));
  return rv;
}

setvar Solver::newSetVar(int min, int max)
{
  assert(max - min >= 0 );

  bool unary = false;
  if( max == min ) unary = true;

  setvar x(setvars.size());

  setvars.push();
  setvarnames.push_back(std::string());

  setvar_data & xd = setvars.last();

  xd.min = min;
  xd.max = max;
  xd._card = newCSPVar(0, max-min+1);

  // the propositional encoding of the domain
  xd.firstbool = newVar();
  for(int i = min+1; i != max+1; ++i)
    newVar();

  for(int i = xd.min; i <= xd.max; ++i) {
    setevent in(x, setevent::IN, i),
      ex(x, setevent::EX, i);
    setevents[ toInt( Lit(xd.ini(i) ) ) ] = in;
    setevents[ toInt( ~Lit(xd.ini(i) ) ) ] = ex;
  }

  /* post constraint sum_i ini(i) = _card */
  vector<Var> v(max-min+1);
  vector<int> w(max-min+1, 1);
  for(int i = min; i != max+1; ++i)
    v[i-min] = xd.firstbool + i - min;
  post_pb(*this, v, w, 0, xd._card);

  return x;
}

std::vector<setvar> Solver::newSetVarArray(int n, int min, int max)
{
  std::vector<setvar> rv;
  for(int i = 0; i != n; ++i)
    rv.push_back(newSetVar(min, max));
  return rv;
}

void Solver::addClause(vec<Lit>& ps)
{
    assert(decisionLevel() == 0);

    if (!ok)
      throw unsat();
    else{
        // Check if clause is satisfied and remove false/duplicate literals:
        sort(ps);
        Lit p; int i, j;
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
            assert(var(ps[i]) != var_Undef );
            if (value(ps[i]) == l_True || ps[i] == ~p)
                return;
            else if (value(ps[i]) != l_False && ps[i] != p)
                ps[j++] = p = ps[i];
        }
        ps.shrink(i - j);
    }

    if (ps.size() == 0) {
      ok = false;
      throw unsat();
    } else if (ps.size() == 1){
        assert(value(ps[0]) == l_Undef);
        uncheckedEnqueue(ps[0]);
        ok = (propagate() == NULL);
        if( !ok ) throw unsat();
        return;
    }else{
        Clause* c = Clause_new(ps, false);
        clauses.push(c);
        attachClause(*c);
    }
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

void Solver::wake_on_lit(Var v, cons *c, void *advice)
{
  wakes_on_lit[v].push( make_pair(c, advice) );
}

void Solver::wake_on_dom(cspvar x, cons *c, void *advice)
{
  cspvars[x._id].wake_on_dom.push( make_pair(c, advice) );
}

void Solver::wake_on_lb(cspvar x, cons *c, void *advice)
{
  cspvars[x._id].wake_on_lb.push( make_pair(c, advice) );
}

void Solver::wake_on_ub(cspvar x, cons *c, void *advice)
{
  cspvars[x._id].wake_on_ub.push( make_pair(c, advice) );
}

void Solver::wake_on_fix(cspvar x, cons *c, void *advice)
{
  cspvars[x._id].wake_on_fix.push( make_pair(c, advice) );
}

void Solver::ensure_can_schedule(cons *c)
{
  assert( c->priority >= 0 && c->priority <= MAX_PRIORITY );
  if( c->cqidx < 0 ) {
    c->cqidx = consqs.size();
    consqs.push( consqueue(c, c->priority) );
  }
}

void Solver::schedule_on_dom(cspvar x, cons *c)
{
  ensure_can_schedule(c);
  cspvars[x._id].schedule_on_dom.push(c->cqidx);
}

void Solver::schedule_on_lb(cspvar x, cons *c)
{
  ensure_can_schedule(c);
  cspvars[x._id].schedule_on_lb.push(c->cqidx);
}

void Solver::schedule_on_ub(cspvar x, cons *c)
{
  ensure_can_schedule(c);
  cspvars[x._id].schedule_on_ub.push(c->cqidx);
}

void Solver::schedule_on_fix(cspvar x, cons *c)
{
  ensure_can_schedule(c);
  cspvars[x._id].schedule_on_fix.push(c->cqidx);
}

void Solver::setVarName(Var v, std::string const& name)
{
  varnames[v] = name;
}

void Solver::setCSPVarName(cspvar v, std::string const& name)
{
  cspvarnames[v._id] = name;
}

void Solver::setSetVarName(setvar v, std::string const& name)
{
  setvarnames[v._id] = name;
}

std::string const& Solver::getVarName(Var v)
{
  return varnames[v];
}

std::string const& Solver::getCSPVarName(cspvar v)
{
  return cspvarnames[v._id];
}

std::string const& Solver::getSetVarName(setvar v)
{
  return setvarnames[v._id];
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
        memcpy(current_space, backtrackable_space[level],
               backtrackable_size);
    }
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
  if( !allow_clause_dbg ) return;

  if(trace) {
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    cout << "Debugging clause " << print(*this, from)
         << " generated by constraint "
         << cons_state_printer(*this, *c) << "\n";
  }

  Solver s1;
  s1.allow_clause_dbg = false; // to avoid infinite recursion
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
  setevents.copyTo(s1.setevents);

  s1.varnames.resize(nv);
  s1.cspvarnames.resize( cspvarnames.size() );
  s1.setvarnames.resize( setvarnames.size() );

  cspvars.copyTo(s1.cspvars);
  for(int i = 0; i != cspvars.size(); ++i) {
    cspvar x(i);
    btptr p = s1.alloc_backtrackable(sizeof(cspvar_bt));
    s1.cspvarbt.push(p);
    cspvar_bt & xbt = s1.deref<cspvar_bt>(p);
    xbt.min = cspvars[i].omin;
    xbt.max = cspvars[i].omax;
    xbt.dsize = xbt.max - xbt.min + 1;
    if( xbt.min == xbt.max ) {
      s1.uncheckedEnqueue_np(Lit(cspvars[i].firstbool+1), NO_REASON);
      s1.uncheckedEnqueue_np(Lit(cspvars[i].firstbool), NO_REASON);
    } else
      x.setmax(s1, xbt.max, (Clause*)0L);
  }

  setvars.copyTo(s1.setvars);

  s1.propagate();

  s1.trace = trace;
  c->clone(s1);
  for(int i = 0; i != from->size(); ++i) {
    Lit q = (*from)[i];
    if(s1.value(q) == l_True )
      return; // conflict already
    if( s1.value( q ) != l_False )
      s1.uncheckedEnqueue(~q, 0L);
  }
  Clause *confl = s1.propagate();
  if(trace) {
    cout << "clause debugging: constraint is now in state\n"
         << cons_state_printer(s1, *s1.conses[0]) << "\n";
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  }
  assert(confl);
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
        cout << domevent_printer(*this, pevent) << " forced by ";
        if( active_constraint )
          cout << cons_state_printer(*this, *active_constraint);
        else
          cout << " clause " << print(*this, from);
      } else {
        if( active_constraint )
          cout << lit_printer(*this, p) << " forced by "
               << cons_state_printer(*this, *active_constraint);
        else
          cout << lit_printer(*this, p) << " forced by clause "
               << print(*this, from);
      }
      cout << " at level " << decisionLevel() << "\n";
    }

    check_debug_solution(p, from);
    uncheckedEnqueue_np(p, from);

#ifdef EXPENSIVE_INVARIANTS
    if( active_constraint )
      debugclause(from, active_constraint);
#endif

    // update csp var and propagate, if applicable
    domevent const &pevent = events[toInt(p)];
    if( noevent(pevent) ) return;
    cspvar_fixed& xf = cspvars[pevent.x._id];
    cspvar_bt& xb = deref<cspvar_bt>(cspvarbt[pevent.x._id]);
    if( pevent.type == domevent::EQ ) {
      xb.dsize = 1;
      xb.min = xb.max = pevent.d;
      // propagate towards max
      if( value(xf.leqiUnsafe(pevent.d)) != l_True ) {
        uncheckedEnqueue_np( Lit(xf.leqiUnsafe(pevent.d)),
                             xf.ps2[pevent.d-xf.omin] );
        if( value(xf.eqiUnsafe(pevent.d+1)) != l_False)
          uncheckedEnqueue_np( ~Lit(xf.eqiUnsafe(pevent.d+1)),
                               xf.ps3[pevent.d+1-xf.omin] );
        int leq = pevent.d+1;
        while( leq < xf.omax && value(xf.leqiUnsafe(leq)) != l_True ) {
          uncheckedEnqueue_np( Lit(xf.leqiUnsafe(leq)), xf.ps1[leq-1-xf.omin] );
          if( value(xf.eqiUnsafe(leq+1)) != l_False)
            uncheckedEnqueue_np( ~Lit(xf.eqiUnsafe(leq+1)), xf.ps3[leq+1-xf.omin] );
          ++leq;
        }
      }
      // propagate towards min
      if( pevent.d > xf.omin &&
          value(xf.leqiUnsafe(pevent.d-1)) != l_False ) {
        uncheckedEnqueue_np( ~Lit(xf.leqiUnsafe(pevent.d-1)),
                             xf.ps3[pevent.d-xf.omin] );
        if( value(xf.eqiUnsafe(pevent.d-1)) != l_False)
          uncheckedEnqueue_np( ~Lit(xf.eqiUnsafe(pevent.d-1)),
                               xf.ps2[pevent.d-1-xf.omin] );
        int geq = pevent.d-2;
        while( geq >= xf.omin && value(xf.leqiUnsafe(geq)) != l_False ) {
          uncheckedEnqueue_np( ~Lit(xf.leqiUnsafe(geq)), xf.ps1[geq-xf.omin] );
          if( value(xf.eqiUnsafe(geq)) != l_False )
            uncheckedEnqueue_np( ~Lit(xf.eqiUnsafe(geq)), xf.ps2[geq-xf.omin] );
          --geq;
        }
      }
    }

    if( pevent.type == domevent::NEQ ) {
      --xb.dsize;
      if( pevent.d == xb.min ) {
        while( value(xf.eqiUnsafe(xb.min)) == l_False ) {
          assert( value(xf.leqiUnsafe(xb.min)) != l_False &&
                  (xb.min == xf.omin ||
                   value(xf.leqiUnsafe(xb.min-1)) == l_False ));
          uncheckedEnqueue_np( ~Lit( xf.leqiUnsafe(xb.min) ),
                               xf.ps4[xb.min-xf.omin] );
          ++xb.min;
        }
      }
      if( pevent.d == xb.max ) {
        while( value(xf.eqiUnsafe(xb.max)) == l_False ) {
          assert( (xb.max == xf.omax ||
                   value(xf.leqiUnsafe(xb.max)) == l_True) &&
                  value(xf.leqiUnsafe(xb.max-1)) != l_False );
          uncheckedEnqueue_np( Lit( xf.leqiUnsafe(xb.max-1) ),
                               xf.ps4[xb.max-xf.omin] );
          --xb.max;
        }
      }
    }

    if( pevent.type == domevent::GEQ ) {
      int geq = pevent.d-1;
      if( value(xf.eqiUnsafe(geq)) != l_False ) {
        uncheckedEnqueue_np( ~Lit( xf.eqiUnsafe(geq)), xf.ps2[geq-xf.omin] );
        --xb.dsize;
      }
      while(geq > xf.omin &&
            value( xf.leqiUnsafe(geq-1) ) != l_False ) {
        uncheckedEnqueue_np( ~Lit(xf.leqiUnsafe(geq-1)),
                             xf.ps1[geq-1-xf.omin] );
        if( value(xf.eqiUnsafe(geq-1)) != l_False ) {
          uncheckedEnqueue_np( ~Lit( xf.eqiUnsafe(geq-1)), xf.ps2[geq-1-xf.omin] );
          --xb.dsize;
        }
        --geq;
      }
      xb.min = pevent.d;
      while( value(xf.eqiUnsafe(xb.min)) == l_False ) {
        assert( value(xf.leqiUnsafe(xb.min)) != l_False &&
                value(xf.leqiUnsafe(xb.min-1)) == l_False );
        uncheckedEnqueue_np( ~Lit( xf.leqiUnsafe(xb.min) ),
                             xf.ps4[xb.min-xf.omin] );
        ++xb.min;
      }
    }
    if( pevent.type == domevent::LEQ ) {
      int leq = pevent.d+1;
      if( leq <= xf.omax && value(xf.eqiUnsafe(leq)) != l_False ) {
        uncheckedEnqueue_np( ~Lit( xf.eqiUnsafe(leq)), xf.ps3[leq-xf.omin] );
        --xb.dsize;
      }
      while( leq < xf.omax &&
             value( xf.leqiUnsafe(leq) ) != l_True ) {
        uncheckedEnqueue_np( Lit( xf.leqiUnsafe(leq) ), xf.ps1[leq-1-xf.omin] );
        if( value(xf.eqiUnsafe(leq+1)) != l_False ) {
          uncheckedEnqueue_np( ~Lit( xf.eqiUnsafe(leq+1)), xf.ps3[leq+1-xf.omin] );
          --xb.dsize;
        }
        ++leq;
      }
      xb.max = pevent.d;
      while( value(xf.eqiUnsafe(xb.max)) == l_False ) {
        assert( value(xf.leqiUnsafe(xb.max)) == l_True &&
                value(xf.leqiUnsafe(xb.max-1)) != l_False );
        uncheckedEnqueue_np( Lit( xf.leqiUnsafe(xb.max-1) ),
                             xf.ps4[xb.max-xf.omin] );
        --xb.max;
      }
    }

    if( xb.max == xb.min &&
        value( xf.eqiUnsafe(xb.max) ) != l_True )
      uncheckedEnqueue_np( Lit(xf.eqiUnsafe(xb.max)),
                           xf.ps4[xb.max-xf.omin] );

#ifdef INVARIANTS
    for(int i = xf.omin; i != xf.omax; ++i ) {
      if( value(xf.leqiUnsafe(i)) == l_False )
        assert( xb.min > i );
      if( value(xf.leqiUnsafe(i)) == l_True )
        assert( xb.max <= i );
      if( value(xf.eqiUnsafe(i)) == l_False )
        assert( xb.min != i && xb.max != i );
    }
#endif
}

Clause *Solver::enqueueFill(Lit p, vec<Lit>& ps)
{
  if( value(p) == l_True ) return 0L;
  ps.push( p );
  Clause *r = Clause_new(ps, false, p);
  ps.pop();
  addInactiveClause(r);
  if( value(p) == l_False ) return r;
  uncheckedEnqueue( p, r );
  return 0L;
}

/*************************************************************************

  Queue handling

  schedule(c): puts the constraint c into the queue it wants. Note
  this is static, otherwise we would need a virtual function call +
  bringing the cons into the cache

  unschedule(c): remove c from the queue

  reset_queue(): remove all constraints from the queue
 */
void Solver::schedule(int cqidx)
{
  consqueue & cq = consqs[cqidx];
  consqueue & p = consqs[cq.priority+1];
  if( cq.next >= 0 ) return; // already scheduled
  cq.prev = p.prev;
  cq.next = cq.priority+1;
  p.prev = cqidx;
  consqs[cq.prev].next = cqidx;
}

int Solver::first_scheduled()
{
  int cqidx = 0;
  while( cqidx >= 0 && !consqs[cqidx].c )
    cqidx = consqs[cqidx].next;
  if( cqidx >= 0 && consqs[cqidx].c ) return cqidx;
  else return -1;
}

void Solver::unschedule(int cqidx)
{
  consqueue * cq = &consqs[cqidx];
  consqs[cq->next].prev = cq->prev;
  consqs[cq->prev].next = cq->next;
  cq->next = -1;
  cq->prev = -1;
}

void Solver::reset_queue()
{
  int cqidx = 0;
  while( cqidx >= 0 ) {
    int nxt = consqs[cqidx].next;
    consqs[cqidx].next = -1;
    consqs[cqidx].prev = -1;
    cqidx = nxt;
  }
  for(int i = 0; i <= MAX_PRIORITY+1; ++i) {
    if( i <= MAX_PRIORITY )
      consqs[i].next = i+1;
    if( i )
      consqs[i].prev = i-1;
  }
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise NULL.
|
|    Propagation involves both unit propagation and constraint propagation
|    for constraints that wake on changes.
|
|    Additionally, it puts in the constraint queue all propagators that
|    want to be scheduled on changes.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
Clause* Solver::propagate_inner()
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
        vec< wake_stub >& pwakes = wakes_on_lit[var(p)];
        for(wake_stub *ci = &pwakes[0],
              *ciend = ci+pwakes.size();
            ci != ciend; ++ci) {
          cons *con = (*ci).first;
          active_constraint = con;
          confl = con->wake(*this, p, (*ci).second);
          active_constraint = 0L;
          if( confl ) {
            if( trace ) {
              cout << "Constraint "
                   << cons_state_printer(*this, *con) << " failed, "
                   << "clause " << print(*this, confl) << "\n";
            }
            qhead = trail.size();
            break;
          }
        }

        if(confl)
          break;

        domevent const & pe = events[toInt(p)];
        vec< pair<cons*, void*> > *dewakes = 0L;
        vec<int> *desched = 0L;
        if( noevent(pe) ) goto SetPropagation;
        switch(pe.type) {
        case domevent::NEQ:
          dewakes=&(cspvars[pe.x._id].wake_on_dom);
          desched=&(cspvars[pe.x._id].schedule_on_dom);
          break;
        case domevent::EQ:
          dewakes=&(cspvars[pe.x._id].wake_on_fix);
          desched=&(cspvars[pe.x._id].schedule_on_fix);
          break;
        case domevent::LEQ:
          if( pe.x.max(*this) >= pe.d ) {
            dewakes=&(cspvars[pe.x._id].wake_on_ub);
            desched=&(cspvars[pe.x._id].schedule_on_ub);
          }
          break;
        case domevent::GEQ:
          if( pe.x.min(*this) <= pe.d ) {
            dewakes=&(cspvars[pe.x._id].wake_on_lb);
            desched=&(cspvars[pe.x._id].schedule_on_lb);
          }
          break;
        case domevent::NONE: assert(0);
        }
        if( !dewakes ) goto SetPropagation;
        for( wake_stub *ci = &((*dewakes)[0]),
               *ciend = ci+dewakes->size();
             ci != ciend; ++ci) {
          cons *con = (*ci).first;
          active_constraint = con;
          confl = con->wake(*this, p, (*ci).second);
          active_constraint = 0L;
          if( confl ) {
            if( trace ) {
              cout << "Constraint "
                   << cons_state_printer(*this, *con) << " failed, "
                   << "clause @ " << confl << "\n";
            }
            qhead = trail.size();
            break;
          }
        }

        if(confl)
          break;

        for( int *si = &((*desched)[0]),*siend = si+desched->size();
             si != siend; ++si)
          schedule(*si);

    SetPropagation:
        setevent const & se = setevents[toInt(p)];
        if( noevent(se) ) continue;
        vec<wake_stub> *sewakes = 0L;
        vec<int> *sesched = 0L;
        switch(se.type) {
        case setevent::IN:
          sewakes=&(setvars[se.x._id].wake_on_in);
          sesched=&(setvars[se.x._id].schedule_on_in);
          break;
        case setevent::EX:
          sewakes=&(setvars[se.x._id].wake_on_ex);
          sesched=&(setvars[se.x._id].schedule_on_ex);
          break;
        case setevent::NONE: assert(0);
        }
        for( wake_stub *ci = &((*sewakes)[0]),
               *ciend = ci+sewakes->size();
             ci != ciend; ++ci) {
          cons *con = (*ci).first;
          active_constraint = con;
          confl = con->wake(*this, p, (*ci).second);
          active_constraint = 0L;
          if( confl ) {
            if( trace ) {
              cout << "Constraint "
                   << cons_state_printer(*this, *con) << " failed, "
                   << "clause @ " << confl << "\n";
            }
            qhead = trail.size();
            break;
          }
        }

        if( confl )
          break;

        for( int *si = &((*sesched)[0]),*siend = si+sesched->size();
             si != siend; ++si)
          schedule(*si);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}

Clause *Solver::propagate()
{
  Clause *confl = NULL;
  int next = -1;
  do {
    if( next >= 0 ) {
      unschedule(next);
      active_constraint = consqs[next].c;
      confl = consqs[next].c->propagate(*this);
      active_constraint = 0L;
      if( confl ) {
        qhead = trail.size();
        reset_queue();
        return confl;
      }
    }
    if( qhead < trail.size() )
      confl = propagate_inner();
    if( confl ) {
      reset_queue();
      return confl;
    }

    next = first_scheduled();
  } while( qhead < trail.size() || next >= 0);
  return 0L;
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

void Solver::gcInactive()
{
  int i,j;
  for(i = j = 0; i < inactive.size(); ++i) {
    if( !locked(*inactive[i]) )
      free(inactive[i]);
    else
      inactive[j++] = inactive[i];
  }
  inactive.shrink(i - j);
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
        Clause *confl;
        try {
          confl = propagate();
        } catch( unsat ) {
          assert(decisionLevel() == 0);
          ++conflicts;
          return l_False;
        }
        if (confl != NULL){
            // CONFLICT
            conflicts++; conflictC++;
            if (decisionLevel() == 0) return l_False;

            first = false;

            if( !learning ) {
              Lit flip = trail[ trail_lim[ decisionLevel() - 1 ] ];
              cancelUntil( decisionLevel() - 1 );
              uncheckedEnqueue(~flip, 0L);
              continue;
            }

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
                   << " and setting " << lit_printer(*this, learnt_clause[0]);
              if( !noevent(event(learnt_clause[0])) ) {
                cspvar x = event(learnt_clause[0]).x;
                cout << ", " << cspvar_printer(*this, x) << " in "
                     << domain_as_set(*this, x);
              }
              cout << "\n";
              cout << "Conflict clause " << conflicts
                   << ": " << print(*this, reason[var(learnt_clause[0])])
                   << "\n";
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


            if (restarting &&
                nof_conflicts >= 0 && conflictC >= nof_conflicts){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if ( decisionLevel() == 0 || inactive.size() > 10*nAssigns() )
              gcInactive();

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
                cout << lit_printer(*this, next);
              } else {
                cout << domevent_printer(*this, pe);
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
        cspsetmodel.growTo(setvars.size());
        for(int i = 0; i != setvars.size(); ++i) {
          set<int>& lb = cspsetmodel[i].first;
          set<int>& ub = cspsetmodel[i].second;
          lb.clear(); ub.clear();
          for(int j = setvars[i].min; j <= setvars[i].max; ++j) {
            lbool l = value( setvars[i].ini(j) );
            if( l != l_False )
              ub.insert(j);
            if( l == l_True )
              lb.insert(j);
          }
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

void Solver::excludeLast()
{
  vec<Lit> exclude;
  for(int i = 0; i != cspvars.size(); ++i) {
    cspvar_fixed & xf = cspvars[i];
    pair<int, int> pi = cspmodel[i];
    assert( pi.first == pi.second );
    exclude.push( ~Lit(xf.eqi( pi.first )) );
  }
  for(int i = 0; i != setvars.size(); ++i) {
    setvar_data & xd = setvars[i];
    pair< set<int>, set<int> > const& pi = cspsetmodel[i];
    set<int> const& lb = pi.first;
    set<int> const& ub = pi.second;
    for(int j = xd.min; j <= xd.max; ++j) {
      if( lb.find(j) != lb.end() )
        exclude.push( ~Lit(xd.ini( j )));
      else if( ub.find(j) == ub.end() )
        exclude.push( Lit(xd.ini(j)));
    }
  }
  addClause(exclude);
  if( trace ) {
    cout << "added exclude clause (";
    for(int i = 0; i != exclude.size(); ++i)
      cout << lit_printer(*this, exclude[i]) << ' ';
    cout << ")\n";
  }
}

//==================================================
// output

ostream& operator<<(ostream& os, domevent_printer dp)
{
  if( noevent(dp._p) ) {
    os << "noevent";
    return os;
  }

  os << cspvar_printer(dp._s, dp._p.x)
     << ' ' << opstring(dp._p.type) << ' '
     << dp._p.d;
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

Clause* cons::wake(Solver&, Lit, void *)
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

ostream& cons::print(Solver&, ostream& os) const
{
  os << "cons@" << this;
  return os;
}

ostream& cons::printstate(Solver&, ostream& os) const
{
  os << "cons@" << this;
  return os;
}
