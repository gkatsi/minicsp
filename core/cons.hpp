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

// alldiff
void post_alldiff(Solver &s, std::vector<cspvar> const& vars);

