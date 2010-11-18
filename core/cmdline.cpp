#include "solver.hpp"
#include "cmdline.hpp"
#include <map>

namespace cmdline {
  using namespace std;
  map<string, BranchHeuristic> varbranch;
  map<string, ValBranchHeuristic> valbranch;

  void fillbranch() {
    varbranch["VSIDS"] = VAR_VSIDS;
    varbranch["lex"] = VAR_LEX;
    varbranch["dom"] = VAR_DOM;
    varbranch["domwdeg"] = VAR_DOMWDEG;

    valbranch["VSIDS"] = VAL_VSIDS;
    valbranch["lex"] = VAL_LEX;
    valbranch["bisect"] = VAL_BISECT;
  }

  void parse_solver_options(Solver &s, arglist& args) {
    s.learning = !has_option(args, "--nolearning");
    s.trace = has_option(args, "--trace");
#ifdef INVARIANTS
    s.debugclauses = has_option(args, "--debugclauses");
#endif
    s.restarting = !has_option(args, "--norestart");

    pair<bool, int> has_base_restart =
      has_argoption<int>(args, "--base-restart");
    if( has_base_restart.first )
      s.restart_first = has_base_restart.second;

    pair<bool, int> has_verbosity =
      has_argoption<int>(args, "--verbosity");
    if( has_verbosity.first )
      s.verbosity = has_verbosity.second;

    fillbranch();
    pair<bool, string> has_varbranch =
      has_argoption<string>(args, "--varbranch");
    if( has_varbranch.first ) {
      if( varbranch.find(has_varbranch.second) == varbranch.end() ) {
        string msg = "Unknown variable branching heuristic "
          + has_varbranch.second;
        throw cmd_line_error(msg);
      }
      s.varbranch = varbranch[has_varbranch.second];
    }

    pair<bool, string> has_valbranch =
      has_argoption<string>(args, "--valbranch");
    if( has_valbranch.first ) {
      if( valbranch.find(has_valbranch.second) == valbranch.end() ) {
        string msg = "Unknown value branching heuristic "
          + has_valbranch.second;
        throw cmd_line_error(msg);
      }
      s.valbranch = valbranch[has_valbranch.second];
    }
  }
}
