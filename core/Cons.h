/***********************************************************************[Cons.h]
 *
 * MiniCSP
 *
 * Copyright (c) 2010 George katsirelos
 ******************************************************************************/

#include "Solver.h"
#include <vector>

/* Boolean N-ary linear inequality: Pseudo-Boolean constraints */
cons *post_pb(Solver& s, std::vector<Var> const& vars,
              std::vector<int> const& weights, int lb);
