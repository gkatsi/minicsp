/***********************************************************************[Cons.h]
 *
 * MiniCSP
 *
 * Copyright (c) 2010 George katsirelos
 ******************************************************************************/

#include "Solver.h"
#include <vector>

/* Arithmetic relations */

// v1 <= v2 + c
cons *post_leq(Solver& s, cspvar v1, cspvar v2, int c);
// v1 < v2 + c
cons *post_less(Solver& s, cspvar v1, cspvar v2, int c);

// sum coeff[i]*vars[i] + c <= 0
cons *post_lin_leq(Solver &s, std::vector<cspvar> const& vars,
                   std::vector<int> const &coeff, int c);
cons *post_lin_less(Solver &s, std::vector<cspvar> const& vars,
                    std::vector<int> const &coeff, int c);

// sum coeff[i]*vars[i] + c <= 0 implies b=1. This is NOT flatzinc's
// reified constraint!
cons *post_lin_leq_imp_b_re(Solver &s, std::vector<cspvar> const&vars,
                      std::vector<int> const &coeff,
                      int c, cspvar b);
cons *post_lin_less_imp_b_re(Solver &s, std::vector<cspvar> const&vars,
                       std::vector<int> const &coeff,
                       int c, cspvar b);

// b implies sum coeff[i]*vars[i] + c <= 0. Again, not flatzinc's
// constraint
cons *post_b_imp_lin_leq_re(Solver &s,
                            cspvar b,
                            std::vector<cspvar> const&vars,
                            std::vector<int> const &coeff,
                            int c);
cons *post_b_imp_lin_less_re(Solver &s,
                             cspvar b,
                             std::vector<cspvar> const&vars,
                             std::vector<int> const &coeff,
                             int c);

// this is flatzinc reified linear inequality
cons *post_lin_leq_iff_re(Solver &s, std::vector<cspvar> const& vars,
                          std::vector<int> const& coeff,
                          int c, cspvar b);
cons *post_lin_less_iff_re(Solver &s, std::vector<cspvar> const& vars,
                           std::vector<int> const& coeff,
                           int c, cspvar b);

/* no cons_lin_eq, because it's NP-hard. Just simulate with le and
   ge. */

/* Boolean N-ary linear inequality: Pseudo-Boolean constraints */
cons *post_pb(Solver& s, std::vector<Var> const& vars,
              std::vector<int> const& weights, int lb);
