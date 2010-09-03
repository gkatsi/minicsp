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

// v1 != v2 + c
void post_neq(Solver& s, cspvar v1, cspvar v2, int c);

// v1 <= v2 + c
void post_leq(Solver& s, cspvar v1, cspvar v2, int c);
// v1 < v2 + c
void post_less(Solver& s, cspvar v1, cspvar v2, int c);

// sum coeff[i]*vars[i] + c <= 0
void post_lin_leq(Solver &s, std::vector<cspvar> const& vars,
                   std::vector<int> const &coeff, int c);
void post_lin_less(Solver &s, std::vector<cspvar> const& vars,
                    std::vector<int> const &coeff, int c);

// sum coeff[i]*vars[i] + c <= 0 implies b=1. This is NOT flatzinc's
// reified constraint!
void post_lin_leq_imp_b_re(Solver &s, std::vector<cspvar> const&vars,
                      std::vector<int> const &coeff,
                      int c, cspvar b);
void post_lin_less_imp_b_re(Solver &s, std::vector<cspvar> const&vars,
                       std::vector<int> const &coeff,
                       int c, cspvar b);

// b implies sum coeff[i]*vars[i] + c <= 0. Again, not flatzinc's
// constraint
void post_b_imp_lin_leq_re(Solver &s,
                            cspvar b,
                            std::vector<cspvar> const&vars,
                            std::vector<int> const &coeff,
                            int c);
void post_b_imp_lin_less_re(Solver &s,
                             cspvar b,
                             std::vector<cspvar> const&vars,
                             std::vector<int> const &coeff,
                             int c);

// this is flatzinc reified linear inequality
void post_lin_leq_iff_re(Solver &s, std::vector<cspvar> const& vars,
                          std::vector<int> const& coeff,
                          int c, cspvar b);
void post_lin_less_iff_re(Solver &s, std::vector<cspvar> const& vars,
                           std::vector<int> const& coeff,
                           int c, cspvar b);

/* no cons_lin_eq, because it's NP-hard. Just simulate with le and
   ge. */

/* Boolean N-ary linear inequality: Pseudo-Boolean constraints */
void post_pb(Solver& s, std::vector<Var> const& vars,
              std::vector<int> const& weights, int lb);

// alldiff
void post_alldiff(Solver &s, std::vector<cspvar> const& vars);

