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

/* Boolean N-ary linear inequality: Pseudo-Boolean constraints */
cons *post_pb(Solver& s, std::vector<Var> const& vars,
              std::vector<int> const& weights, int lb);
