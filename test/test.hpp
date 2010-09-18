#ifndef __MINICSP_TEST
#define __MINICSP_TEST

#include "solver.hpp"

void assert_clause_exact(Clause *to_test, vec<Lit> const& expected);
void assert_clause_contains(Clause *to_test, vec<Lit> const& expected);

#define MUST_BE_UNSAT(x) do {                   \
    bool thrown##__LINE__ = false;              \
    try { x; } catch( unsat ) {                 \
      thrown##__LINE__ = true; }                \
    assert(thrown##__LINE__);                   \
  } while(0)

#endif
