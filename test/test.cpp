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

void assert_num_solutions(Solver &s, int ns)
{
  int numsol = 0;
  bool next;
  do {
    next = s.solve();
    if( next ) {
      ++numsol;
      try {
        s.excludeLast();
      } catch(unsat) {
        next = false;
      }
    }
  } while(next);
  assert(numsol == ns);
}

const char *duplicate_test::what() const throw()
{
  return tname.c_str();
}

void test_container::add(test t, std::string s)
{
  if( tset.find(t) != tset.end() )
    throw duplicate_test( tset[t]);
  tests.push_back(make_pair(s, t));
  tset[t] = s;
}

void test_container::run()
{
  using namespace std;
  for(size_t i = 0; i != tests.size(); ++i) {
    cout << tests[i].first << "..." << flush;
    (*tests[i].second)();
    cout << "OK\n";
  }
}
