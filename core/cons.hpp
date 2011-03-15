/***********************************************************************[Cons.h]
 *
 * MiniCSP
 *
 * Copyright (c) 2010 George katsirelos
 ******************************************************************************/

#include "solver.hpp"
#include <vector>

/* Arithmetic relations */

// v1 == v2 + c
void post_eq(Solver& s, cspvar v1, cspvar v2, int c);

/* x == -y + c */
void post_neg(Solver &s, cspvar x, cspvar y, int c);

// v1 != v2 + c
void post_neq(Solver& s, cspvar v1, cspvar v2, int c);

// v1 <= v2 + c
void post_leq(Solver& s, cspvar v1, cspvar v2, int c);
// v1 < v2 + c
void post_less(Solver& s, cspvar v1, cspvar v2, int c);

/* v1 == v2 + c <=> b */
void post_eq_re(Solver &s, cspvar v1, cspvar v2, int c, cspvar b);
void post_eq_re(Solver &s, cspvar v1, cspvar v2, int c, Lit b);

/* v1 != v2 + c <=> b */
void post_neq_re(Solver &s, cspvar v1, cspvar v2, int c, cspvar b);
void post_neq_re(Solver &s, cspvar v1, cspvar v2, int c, Lit b);

/* v1 <= v2 + c <=> b */
void post_leq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b);
void post_leq_re(Solver &s, cspvar x, cspvar y, int c, Lit b);
/* v1 < v2 + c <=> b */
void post_less_re(Solver &s, cspvar x, cspvar y, int c, cspvar b);
void post_less_re(Solver &s, cspvar x, cspvar y, int c, Lit b);
/* v1 >= v2 + c <=> b */
void post_geq_re(Solver &s, cspvar x, cspvar y, int c, cspvar b);
void post_geq_re(Solver &s, cspvar x, cspvar y, int c, Lit b);
/* v1 > v2 + c <=> b */
void post_gt_re(Solver &s, cspvar x, cspvar y, int c, cspvar b);
void post_gt_re(Solver &s, cspvar x, cspvar y, int c, Lit b);

/* |x| = y + c */
void post_abs(Solver& s, cspvar v1, cspvar v2, int c);

// sum coeff[i]*vars[i] + c <= 0
void post_lin_leq(Solver &s, std::vector<cspvar> const& vars,
                   std::vector<int> const &coeff, int c);
void post_lin_less(Solver &s, std::vector<cspvar> const& vars,
                    std::vector<int> const &coeff, int c);

// sum coeff[i]*vars[i] + c <= 0 implies b=1. This is NOT flatzinc's
// reified constraint!
void post_lin_leq_right_imp_re(Solver &s, std::vector<cspvar> const&vars,
                               std::vector<int> const &coeff,
                               int c, cspvar b);
void post_lin_less_right_imp_re(Solver &s, std::vector<cspvar> const&vars,
                                std::vector<int> const &coeff,
                                int c, cspvar b);

// b implies sum coeff[i]*vars[i] + c <= 0. Again, not flatzinc's
// constraint
void post_lin_leq_left_imp_re(Solver &s,
                              std::vector<cspvar> const&vars,
                              std::vector<int> const &coeff,
                              int c,
                              cspvar b);
void post_lin_less_left_imp_re(Solver &s,
                               std::vector<cspvar> const&vars,
                               std::vector<int> const &coeff,
                               int c,
                               cspvar b);

// this is flatzinc reified linear inequality
void post_lin_leq_iff_re(Solver &s, std::vector<cspvar> const& vars,
                          std::vector<int> const& coeff,
                          int c, cspvar b);
void post_lin_less_iff_re(Solver &s, std::vector<cspvar> const& vars,
                           std::vector<int> const& coeff,
                           int c, cspvar b);

/* linear equality: just <= and >=. It is np-hard to propagate anyway,
   although in the future it might be worth implementing the gac
   propagator for small coefficients */

// sum coeff[i]*vars[i] + c = 0
void post_lin_eq(Solver &s,
                 std::vector<cspvar> const& vars,
                 std::vector<int> const& coeff,
                 int c);

// sum coeff[i]*vars[i] + c = 0 implies b = 1
void post_lin_eq_right_imp_re(Solver &s,
                              std::vector<cspvar> const& vars,
                              std::vector<int> const& coeff,
                              int c,
                              cspvar b);
// b = 1 implies sum coeff[i]*vars[i] + c = 0
void post_lin_eq_left_imp_re(Solver &s,
                             std::vector<cspvar> const& vars,
                             std::vector<int> const& coeff,
                             int c,
                             cspvar b);
// b=1 iff sum coeff[i]*vars[i] + c = 0
void post_lin_eq_iff_re(Solver &s,
                        std::vector<cspvar> const& vars,
                        std::vector<int> const& coeff,
                        int c,
                        cspvar b);

/* linear inequality: L != 0 */
void post_lin_neq(Solver &s,
                  std::vector<cspvar> const& vars,
                  std::vector<int> const& coeff,
                  int c);

/* L != 0 => b = 1 */
void post_lin_neq_right_imp_re(Solver &s,
                               std::vector<cspvar> const& vars,
                               std::vector<int> const& coeff,
                               int c,
                               cspvar b);

/* b = 1 => L != 0 */
void post_lin_neq_left_imp_re(Solver &s,
                              std::vector<cspvar> const& vars,
                              std::vector<int> const& coeff,
                              int c,
                              cspvar b);

/* b = 1 <=> L != 0 */
void post_lin_neq_iff_re(Solver &s,
                         std::vector<cspvar> const& vars,
                         std::vector<int> const& coeff,
                         int c,
                         cspvar b);

/* Boolean N-ary linear inequality: Pseudo-Boolean constraints */
void post_pb(Solver& s, std::vector<Var> const& vars,
              std::vector<int> const& weights, int lb);
void post_pb(Solver& s, std::vector<cspvar> const& vars,
              std::vector<int> const& weights, int lb);

// reified versions
void post_pb_right_imp_re(Solver& s, std::vector<cspvar> const& vars,
                          std::vector<int> const& weights, int lb,
                          cspvar b);
void post_pb_left_imp_re(Solver& s, std::vector<cspvar> const& vars,
                         std::vector<int> const& weights, int lb,
                         cspvar b);
void post_pb_iff_re(Solver& s, std::vector<cspvar> const& vars,
                    std::vector<int> const& weights, int lb,
                    cspvar b);

/* Pseudo Boolean constraint with a non-Boolean var on the RHS:

   sum w[i]*v[i] + c = rhs

   where v[i] are all Boolean
 */
void post_pb(Solver& s, std::vector<Var> const& vars,
             std::vector<int> const& weights, int c, cspvar rhs);
void post_pb(Solver& s, std::vector<cspvar> const& vars,
             std::vector<int> const& weights, int c, cspvar rhs);

/* x = y*z */
void post_mult(Solver& s, cspvar x, cspvar y, cspvar z);

/* x = y/z */
void post_div(Solver& s, cspvar x, cspvar y, cspvar z);

/* x = min(y,z) */
void post_min(Solver &s, cspvar x, cspvar y, cspvar z);
/* x = max(y,z) */
void post_max(Solver &s, cspvar x, cspvar y, cspvar z);

/* Element: R = X[I-offset]

   offset is 0 by default for normal 0-based indexing, but the
   flatzinc frontend uses 1 for its own 1-based indexing */
void post_element(Solver &s, cspvar R, cspvar I,
                  std::vector<cspvar> const& X, int offset=0);

// alldiff. if gac is false it behaves like a clique of != and also
// detects gac disentailment, otherwise enforces gac like usual
void post_alldiff(Solver &s, std::vector<cspvar> const& vars, bool gac = true);

// atmostnvalue. The number of distinct values taken by the vector x
// is at most N
void post_atmostnvalue(Solver &s, std::vector<cspvar> const& x, cspvar N);

/* Regular
 *
 * the sequence x[0]...x[n] is a string accepted by the automaton (nfa
 * or dfa, does not matter) with transition function d, initial state
 * q0 and accepting states F. The alphabet S is implicitly defined by
 * the transition function.
 *
 * Since we can do NFA, we accept non-deterministic transition
 * functions but there is no way to specify e-transitions, so the nfa
 * has to be converted to e-free.
 *
 * Following the minizinc specification, state 0 is implicitly a
 * rejecting sink.
 */
namespace regular {
  struct transition {
    size_t q0;
    int s;
    size_t q1;
    transition(size_t pq0, int ps, size_t pq1) : q0(pq0), s(ps), q1(pq1) {}
  };

  struct automaton {
    std::vector<transition> d;
    int q0;
    std::set<int> F;

    automaton(std::vector<transition> const& pd, int pq0,
              std::set<int> pF) : d(pd), q0(pq0), F(pF) {}
  };
}

void post_regular(Solver& s, std::vector<cspvar> const& x,
                  regular::automaton const& aut,
                  bool gac = true);

/* Cumulative constraint: holds if a set of tasks 0..n-1 is scheduled
   so that task i starts as s[i], has duration d[i] and requires r[i]
   of a given resource, and the resource requirements of all tasks
   running at any given time is less than c. */
void post_cumulative(Solver& s, std::vector<cspvar> const& start,
                     std::vector<cspvar> const& dur,
                     std::vector<cspvar> const& req,
                     cspvar cap);
