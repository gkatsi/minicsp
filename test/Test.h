#ifndef __MINICSP_TEST
#define __MINICSP_TEST

#include "Solver.h"

void assert_clause_exact(Clause *to_test, vec<Lit> const& expected);
void assert_clause_contains(Clause *to_test, vec<Lit> const& expected);

#endif
