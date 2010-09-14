/****************************************************************************************[Solver.h]
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

#ifndef Solver_h
#define Solver_h

#include <vector>
#include <cstdio>
#include <cstring>

#include "Vec.h"
#include "Heap.h"
#include "Alg.h"

#include "solvertypes.hpp"


//=================================================================================================
// Solver -- the main class:

class Solver;
/* a completely opaque reference type. can only be used with Solver::deref */
class btptr {
  size_t offset;
  friend class Solver;
};

class Solver {
public:

    // Constructor/Destructor:
    //
    Solver();
    ~Solver();

    // Problem specification:
    //
    Var     newVar    (bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    cspvar  newCSPVar (int min, int max);                       // Add a CSP (multi-valued) var with the given lower and upper bounds
    std::vector<cspvar> newCSPVarArray(int n, int min, int max);// Add a number of identical CSP vars
    bool    addClause (vec<Lit>& ps);                           // Add a clause to the solver. NOTE! 'ps' may be shrunk by this method!
    bool    addConstraint(cons *c);                             // Add a constraint. For transfer of ownership only, everything else in the constructor

    /* Waking means that the constraint is called immediately when we
       process the corresponding literal in the assignment
       stack. Scheduling means that when we process the literal, the
       constraint is placed in a queue and then called when the
       assignment stack has been processed. Similar to the difference
       between a demon and a propagator in ilog. Unlike gecode
       advisors, a constraint can prune when it wakes. */
    void    wake_on_lit(Var, cons *c);                          // Wake this constraint when the Boolear Var is fixed
    void    wake_on_dom(cspvar, cons*c);                        // Wake this constraint when a value of cspvar is pruned
    void    wake_on_lb(cspvar, cons*c);                         // Wake this constraint when the lb of cspvar is changed
    void    wake_on_ub(cspvar, cons*c);                         // Wake this constraint when the ub of cspvar is changed
    void    wake_on_fix(cspvar, cons*c);                        // Wake this constraint when the cspvar is assigned

    void    schedule_on_dom(cspvar, cons*c);                    // Schedule this constraint when a value of cspvar is pruned
    void    schedule_on_lb(cspvar, cons*c);                     // Schedule this constraint when the lb of cspvar is changed
    void    schedule_on_ub(cspvar, cons*c);                     // Schedule this constraint when the ub of cspvar is changed
    void    schedule_on_fix(cspvar, cons*c);                    // Schedule this constraint when the cspvar is assigned

    btptr   alloc_backtrackable(unsigned size);                 // allocate memory to be automatically restored to its previous contents on backtracking
    void*   get(btptr p);                                       // get direct pointer to  backtrackable mem. for temporary use only
    template<typename T>
    T&      deref(btptr p);                                     // dereference backtrackable mem. for temporary use only

    // event information
    domevent event(Lit p);                                      // get the event associated with a literal, if any

    // Solving:
    //
    bool    simplify     ();                        // Removes already satisfied clauses.
    bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    bool    solve        ();                        // Search without assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state

    // Explaining
    void    addInactiveClause(Clause *c);           // add a clause that will not be propagated, just as a reason or a conflict clause

    // Variable mode:
    //
    void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool   value      (Var x) const;       // The current value of a variable.
    lbool   value      (Lit p) const;       // The current value of a literal.
    lbool   modelValue (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
  std::pair<int,int>  cspModelRange(cspvar x) const; // Range in last model
  int     cspModelValue(cspvar x) const;     // Assigned value in last model

    int     nAssigns   ()      const;       // The current number of assigned literals.
    int     nClauses   ()      const;       // The current number of original clauses.
    int     nLearnts   ()      const;       // The current number of learnt clauses.
    int     nVars      ()      const;       // The current number of variables.

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;                   // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;                // If problem is unsatisfiable (possibly under assumptions),
                                        // this vector represent the final conflict clause expressed in the assumptions.
    vec<std::pair<int, int> > cspmodel; // for a satisfiable problem, the lb and ub of every csp variable. easier to
                                        // access than through model, but strictly speaking redundant

    // Mode of operation:
    //
    bool      trace;              // if true, will output all prunings
    double    var_decay;          // Inverse of the variable activity decay factor.                                            (default 1 / 0.95)
    double    clause_decay;       // Inverse of the clause activity decay factor.                                              (1 / 0.999)
    double    random_var_freq;    // The frequency with which the decision heuristic tries to choose a random variable.        (default 0.02)
    int       restart_first;      // The initial restart limit.                                                                (default 100)
    double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)
    bool      expensive_ccmin;    // Controls conflict clause minimization.                                                    (default TRUE)
    int       polarity_mode;      // Controls which polarity the decision heuristic chooses. See enum below for allowed modes. (default polarity_false)
    int       verbosity;          // Verbosity level. 0=silent, 1=some progress report                                         (default 0)
    bool      phase_saving;

    enum { polarity_true = 0, polarity_false = 1, polarity_user = 2, polarity_rnd = 3 };

    // Statistics: (read-only member variable)
    //
    uint64_t starts, decisions, rnd_decisions, propagations, conflicts;
    uint64_t clauses_literals, learnts_literals, max_literals, tot_literals;

public:
    // Interface to propagators
    void     debugclause      (Clause *from, cons *c);                                 // check whether clause *from causes a failure given constraint c
    void     uncheckedEnqueue (Lit p, Clause* from = NULL);                            // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, Clause* from = NULL);                            // Test if fact 'p' contradicts current state, enqueue otherwise.
    Clause*  propagate        ();                                                      // Perform unit propagation. Returns possibly conflicting clause.

    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.

    int cspvarmax(cspvar x);              // get the current max of x
    int cspvarmin(cspvar x);              // get the current min of x
    int cspvardsize(cspvar x);            // get the current domain size of x (which may be smaller than max - min + 1)
    Var cspvareqi(cspvar x, int d);       // get the propositional var representing x = d
    Var cspvarleqi(cspvar x, int d);      // get the propositional var representing x <= d

protected:

    // Helper structures:
    //
    struct VarOrderLt {
        const vec<double>&  activity;
        bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const vec<double>&  act) : activity(act) { }
    };

    friend class VarFilter;
    struct VarFilter {
        const Solver& s;
        VarFilter(const Solver& _s) : s(_s) {}
        bool operator()(Var v) const { return toLbool(s.assigns[v]) == l_Undef && s.decision_var[v]; }
    };

    // Solver state:
    //
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vec<Clause*>        clauses;          // List of problem clauses.
    vec<Clause*>        learnts;          // List of learnt clauses.
    vec<Clause*>        inactive;         // List of non-propagating clauses.
    vec<cons*>          conses;           // List of problem constraints.
    double              cla_inc;          // Amount to bump next clause with.
    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.

    vec<vec<Clause*> >  watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<vec<cons*> >    wakes_on_lit;     // 'wakes_on_lit[var(lit)]' is a list of csp constraints that wake when var is set

    vec<char>           assigns;          // The current assignments (lbool:s stored as char:s).
    vec<char>           polarity;         // The preferred polarity of each variable.
    vec<char>           decision_var;     // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<Clause*>        reason;           // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    vec<int>            level;            // 'level[var]' contains the level at which the assignment was made.
    vec<lbool>          phase;            // backjumped over as this polarity
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
    double              random_seed;      // Used by the random variable selection.
    double              progress_estimate;// Set by 'search()'.
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.

    // csp stuff
    vec<cspvar_fixed>   cspvars;             // the fixed data for each cspvar
    vec<btptr>          cspvarbt;            // the backtrackable data for each var
    vec<domevent>       events;              // the csp event that a literal corresponds to
    size_t              backtrackable_size;  // How much we need to copy
    size_t              backtrackable_cap;   // How much backtrackable memory is allocated
    vec<void*>          backtrackable_space; // per-level copies of backtrackable data
    void*               current_space;       // All backtrackable data are pointers into this

    cons*               active_constraint;   // the constraint currently propagating.

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char>           seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;

    // Main internal methods:
    //
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    (int polarity_mode, double random_var_freq);             // Return the next decision variable.
    void     analyze          (Clause* confl, vec<Lit>& out_learnt, int& out_btlevel); // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool     litRedundant     (Lit p, uint32_t abstract_levels);                       // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts, double * nof_learnts);               // Search for a given number of conflicts.
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    void     removeSatisfied  (vec<Clause*>& cs);                                      // Shrink 'cs' to contain only non-satisfied clauses.
    void     uncheckedEnqueue_np(Lit p, Clause *from);                                 // uncheckedEnqueue with no CSP propagation

    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void     attachClause     (Clause& c);             // Attach a clause to watcher lists.
    void     detachClause     (Clause& c);             // Detach a clause to watcher lists.
    void     removeClause     (Clause& c);             // Detach and free a clause.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

    // Misc:
    //
    int      decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
    double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...

    // Debug:
    void     printLit         (Lit l);
    template<class C>
    void     printClause      (const C& c);
    void     verifyModel      ();
    void     checkLiteralCount();

    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
        return (int)(drand(seed) * size); }
};


//=================================================================================================
// Implementation of inline methods:

inline
void* Solver::get(btptr p)
{
  return ((char*)current_space)+p.offset;
}

template <typename T>
inline
T& Solver::deref(btptr p)
{
  return *(T*)get(p);
}

inline
domevent Solver::event(Lit p)
{
  if( p != lit_Undef ) return events[toInt(p)];
  else return domevent();
}

inline void Solver::insertVarOrder(Var x) {
    if (!order_heap.inHeap(x) && decision_var[x]) order_heap.insert(x); }

inline void Solver::varDecayActivity() { var_inc *= var_decay; }
inline void Solver::varBumpActivity(Var v) {
    if ( (activity[v] += var_inc) > 1e100 ) {
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100; }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v); }

inline void Solver::claDecayActivity() { cla_inc *= clause_decay; }
inline void Solver::claBumpActivity (Clause& c) {
        if ( (c.activity() += cla_inc) > 1e20 ) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                learnts[i]->activity() *= 1e-20;
            cla_inc *= 1e-20; } }

inline bool     Solver::enqueue         (Lit p, Clause* from)   { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::locked          (const Clause& c) const { return reason[var(c[0])] == &c && value(c[0]) == l_True; }

inline void     Solver::newDecisionLevel()  {
  trail_lim.push(trail.size());
  int dlvl = decisionLevel();
  while( backtrackable_space.size() < decisionLevel() )
    backtrackable_space.push(0L);
  if( !backtrackable_space[dlvl-1] ) {
    backtrackable_space[dlvl-1] = malloc(backtrackable_size);
    if(! backtrackable_space[dlvl-1] )
      throw std::bad_alloc();
  }
  std::memcpy(backtrackable_space[dlvl-1], current_space,
              backtrackable_size);
}

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level[x] & 31); }
inline lbool    Solver::value         (Var x) const   { return toLbool(assigns[x]); }
inline lbool    Solver::value         (Lit p) const   { return toLbool(assigns[var(p)]) ^ sign(p); }
inline lbool    Solver::modelValue    (Lit p) const   { return model[var(p)] ^ sign(p); }
inline int      Solver::nAssigns      ()      const   { return trail.size(); }
inline int      Solver::nClauses      ()      const   { return clauses.size(); }
inline int      Solver::nLearnts      ()      const   { return learnts.size(); }
inline int      Solver::nVars         ()      const   { return assigns.size(); }
inline void     Solver::setPolarity   (Var v, bool b) { polarity    [v] = (char)b; }
inline void     Solver::setDecisionVar(Var v, bool b) { decision_var[v] = (char)b; if (b) { insertVarOrder(v); } }
inline bool     Solver::solve         ()              { vec<Lit> tmp; return solve(tmp); }
inline bool     Solver::okay          ()      const   { return ok; }



//=================================================================================================
// Debug + etc:


#define reportf(format, args...) ( fflush(stdout), fprintf(stderr, format, ## args), fflush(stderr) )

static inline void logLit(FILE* f, Lit l)
{
    fprintf(f, "%sx%d", sign(l) ? "~" : "", var(l)+1);
}

static inline void logLits(FILE* f, const vec<Lit>& ls)
{
    fprintf(f, "[ ");
    if (ls.size() > 0){
        logLit(f, ls[0]);
        for (int i = 1; i < ls.size(); i++){
            fprintf(f, ", ");
            logLit(f, ls[i]);
        }
    }
    fprintf(f, "] ");
}

static inline const char* showBool(bool b) { return b ? "true" : "false"; }


// Just like 'assert()' but expression will be evaluated in the release version as well.
static inline void check(bool expr) { assert(expr); }


inline void Solver::printLit(Lit l)
{
    reportf("%s%d:%c", sign(l) ? "-" : "", var(l)+1, value(l) == l_True ? '1' : (value(l) == l_False ? '0' : 'X'));
}


template<class C>
inline void Solver::printClause(const C& c)
{
    for (int i = 0; i < c.size(); i++){
        printLit(c[i]);
        fprintf(stderr, " ");
    }
}

inline std::pair<int, int> Solver::cspModelRange(cspvar x) const
{
  return cspmodel[x._id];
}

inline int Solver::cspModelValue(cspvar x) const
{
  int id = x._id;
  if( cspmodel[id].first != cspmodel[id].second )
    throw unassigned_var(*this, x);
  return cspmodel[id].first;
}


//==================================================
// cspvar inlines

inline int Solver::cspvarmax(cspvar x)
{
  return deref<cspvar_bt>(cspvarbt[x._id]).max;
}

inline int Solver::cspvarmin(cspvar x)
{
  return deref<cspvar_bt>(cspvarbt[x._id]).min;
}

inline int Solver::cspvardsize(cspvar x)
{
  return deref<cspvar_bt>(cspvarbt[x._id]).dsize;
}

inline Var Solver::cspvareqi(cspvar x, int d)
{
  return cspvars[x._id].eqi(d);
}

inline Var Solver::cspvarleqi(cspvar x, int d)
{
  return cspvars[x._id].leqi(d);
}

inline bool cspvar::indomain(Solver &s, int d) const
{
  return s.value( eqi(s, d) ) != l_False;
}

inline int cspvar::min(Solver &s) const
{
  return s.cspvarmin(*this);
}

inline int cspvar::max(Solver &s) const
{
  return s.cspvarmax(*this);
}

inline int cspvar::domsize(Solver &s) const
{
  return s.cspvardsize(*this);
}

inline Var cspvar::eqi(Solver &s, int d) const
{
  return s.cspvareqi(*this, d);
}

inline Var cspvar::leqi(Solver &s, int d) const
{
  return s.cspvarleqi(*this, d);
}

inline Lit cspvar::r_geq(Solver &s, int d) const
{
  return Lit( leqi(s, d-1) );
}

inline Lit cspvar::r_leq(Solver &s, int d) const
{
  return ~Lit( leqi(s, d) );
}

inline Lit cspvar::r_eq(Solver &s, int d) const
{
  return ~Lit( eqi(s, d) );
}

inline Lit cspvar::r_neq(Solver &s, int d) const
{
  return Lit( eqi(s, d) );
}

inline Lit cspvar::e_geq(Solver &s, int d) const
{
  return ~Lit( leqi(s, d-1) );
}

inline Lit cspvar::e_leq(Solver &s, int d) const
{
  return Lit( leqi(s, d) );
}

inline Lit cspvar::e_eq(Solver &s, int d) const
{
  return Lit( eqi(s, d) );
}

inline Lit cspvar::e_neq(Solver &s, int d) const
{
  return ~Lit( eqi(s, d) );
}

inline Clause *cspvar::remove(Solver &s, int d, Clause *c)
{
  Var xd = eqi(s, d);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_False ) return 0L;
  if( s.value(xd) == l_True ) return c;
  s.uncheckedEnqueue( ~Lit(xd), c);
  return 0L;
}

inline Clause *cspvar::remove(Solver &s, int d, vec<Lit> &ps)
{
  Var xd = eqi(s, d);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_False ) return 0L;
  Clause *r = Clause_new(ps);
  s.addInactiveClause(r);
  if( s.value(xd) == l_True ) return r;
  s.uncheckedEnqueue( ~Lit(xd), r);
  return 0L;
}

inline Clause *cspvar::setmin(Solver &s, int d, Clause *c)
{
  Var xd = leqi(s, d-1);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_False ) return 0L;
  if( s.value(xd) == l_True ) return c;
  s.uncheckedEnqueue( ~Lit(xd), c);
  return 0L;
}

inline Clause *cspvar::setmin(Solver &s, int d, vec<Lit> &ps)
{
  Var xd = leqi(s, d-1);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_False ) return 0L;
  Clause *r = Clause_new(ps);
  s.addInactiveClause(r);
  if( s.value(xd) == l_True ) return r;
  s.uncheckedEnqueue( ~Lit(xd), r);
  return 0L;
}

inline Clause *cspvar::setmax(Solver &s, int d, Clause *c)
{
  Var xd = leqi(s, d);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_True ) return 0L;
  if( s.value(xd) == l_False ) return c;
  s.uncheckedEnqueue( Lit(xd), c);
  return 0L;
}

inline Clause *cspvar::setmax(Solver &s, int d, vec<Lit> &ps)
{
  Var xd = leqi(s, d);
  if( xd == var_Undef ) return 0L;
  if( s.value(xd) == l_True ) return 0L;
  Clause *r = Clause_new(ps);
  s.addInactiveClause(r);
  if( s.value(xd) == l_False ) return r;
  s.uncheckedEnqueue( Lit(xd), r);
  return 0L;
}

inline Clause *cspvar::assign(Solver &s, int d, Clause *c)
{
  Var xd = eqi(s, d);
  assert( xd != var_Undef );
  if( s.value(xd) == l_True ) return 0L;
  if( s.value(xd) == l_False ) return c;
  s.uncheckedEnqueue( Lit(xd), c );
  return 0L;
}

inline Clause *cspvar::assign(Solver &s, int d, vec<Lit> &ps)
{
  Var xd = eqi(s, d);
  assert( xd != var_Undef );
  if( s.value(xd) == l_True ) return 0L;
  Clause *r = Clause_new(ps);
  s.addInactiveClause(r);
  if( s.value(xd) == l_False ) return r;
  s.uncheckedEnqueue( Lit(xd), r );
  return 0L;
}

inline bool operator==(cspvar x1, cspvar x2)
{
  return x1.id() == x2.id();
}

//==================================================
// a trick to avoid branching
// returns a1 if w >= 0, otherwise a2

inline
int select(int w, int a1, int a2)
{
  int mask = w >> (sizeof(int)*8-1);
  return - (~mask*a1) - (mask*a2);
}

//==================================================
// exception stuff

inline
const char *unassigned_var::what() const throw()
{
  const char s[] = "expected var %s to be assigned";
  static const int l = strlen(s);
  static char exc[2*strlen(s)+1];
  snprintf(exc, 2*l, s, _x.id());
  return exc;
}

//=================================================================================================
#endif
