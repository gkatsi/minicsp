/*=============================================================================
 * XCSP frontend for minicsp
 *
 * Copyright (c) 2010 George Katsirelos (gkatsi@gmail.com)
 * Copyright (c) 2008 Olivier ROUSSEL (olivier.roussel <at> cril.univ-artois.fr)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *=============================================================================
 */

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <iomanip>

#include "solver.hpp"
#include "cmdline.hpp"
#include "utils.hpp"

#include "XMLParser_libxml2.hh"
#include "minicsp_callback.hpp"

using namespace CSPXMLParser;

using namespace std;

int main(int argc, char *argv[])
{
  list<string> args(argv+1, argv+argc);

  if(args.empty()) {
    cout << "usage: minicsp-xcsp [options] input.xml";
    return 1;
  }

  Solver s;

  cmdline::parse_solver_options(s, args);
  bool stat = cmdline::has_option(args, "--stat");
  bool maint = cmdline::has_option(args, "--maint");
  setup_signal_handlers(&s);

  double cpu_time = cpuTime();

  minicsp_callback cb(s);

  XMLParser_libxml2<> parser(cb);
  parser.setPreferredExpressionRepresentation(TREE);
  if( maint ) // do not catch exceptions
    parser.parse(args.back().c_str());
  else try {
      parser.parse(args.back().c_str());
    } catch (unsat& e) {
      cout << setw(5) << setfill('=') << '='
           << "UNSATISFIABLE" << setw(5) << '=' << "\n";
      return 0;
    } catch (unsupported& e) {
      cout << "\nERROR: unsupported instance\n";
      return 1;
    }
  double parse_time = cpuTime() - cpu_time;
  cout << parse_time " s to parse instance\n";

  bool findall = cmdline::has_option(args, "--all");

  bool sat = false, next;
  do {
    next = s.solve();
    sat = sat || next;
    if( next ) {
      cout << "solution\n";
    }
    next = findall;
  } while(next);

  if( sat )
    cout << "sat\n";
  else
    cout << "unsat\n";

  if( stat ) {
    printStats(s, "%% ");
    reportf("%sParse time            : %g s\n", "%% ", parse_time);
  }

  return 0;
}
