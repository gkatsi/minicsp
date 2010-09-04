/* SEND+MORE=MONEY */

#include <iostream>
#include <vector>
#include "solver.hpp"
#include "cons.hpp"

using namespace std;

int main()
{
  Solver s;
  vector<cspvar> x = s.newCSPVarArray(8, 0, 9);
  cspvar S = x[0];
  cspvar E = x[1];
  cspvar N = x[2];
  cspvar D = x[3];
  cspvar M = x[4];
  cspvar O = x[5];
  cspvar R = x[6];
  cspvar Y = x[7];

  vector<cspvar> v;
  vector<int> c;
  v.push_back(S); c.push_back(1000);
  v.push_back(E); c.push_back(100);
  v.push_back(N); c.push_back(10);
  v.push_back(D); c.push_back(1);
  v.push_back(M); c.push_back(1000);
  v.push_back(O); c.push_back(100);
  v.push_back(R); c.push_back(10);
  v.push_back(E); c.push_back(1);
  v.push_back(M); c.push_back(-10000);
  v.push_back(O); c.push_back(-1000);
  v.push_back(N); c.push_back(-100);
  v.push_back(E); c.push_back(-10);
  v.push_back(Y); c.push_back(-1);

  vector<int> c1(c);
  for(size_t i = 0; i != c1.size(); ++i) c1[i] = -c1[i];

  post_lin_leq(s, v, c, 0);
  post_lin_leq(s, v, c1, 0);

  s.solve();
}
