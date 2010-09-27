#include "solver.hpp"
#include "cmdline.hpp"

namespace cmdline {
  void parse_solver_options(Solver &s, arglist& args) {
    s.learning = !has_option(args, "--nolearning");
    s.trace = has_option(args, "--trace");
    s.restarting = !has_option(args, "--norestart");

    std::pair<bool, int> has_base_restart =
      has_argoption<int>(args, "--base-restart");
    if( has_base_restart.first )
      s.restart_first = has_base_restart.second;

    std::pair<bool, int> has_verbosity =
      has_argoption<int>(args, "--verbosity");
    if( has_verbosity.first )
      s.verbosity = has_verbosity.second;
  }
}
