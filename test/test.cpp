#include "test.hpp"
#include "solver.hpp"

void assert_clause_exact(Clause *to_test, vec<Lit> const& expected)
{
  assert(expected.size() == to_test->size());
  assert_clause_contains(to_test, expected);
}

void assert_clause_contains(Clause *to_test, vec<Lit> const& expected)
{
  for(int i = 0; i != expected.size(); ++i) {
    bool f = false;
    for(int j = 0; j != to_test->size(); ++j)
      if( expected[i] == (*to_test)[j] ) {
        f = true;
        break;
      }
    assert(f);
  }
}
