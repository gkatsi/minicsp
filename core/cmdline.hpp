#ifndef __CMDLINE_HPP
#define __CMDLINE_HPP

#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <stdexcept>
#include <list>
#include <string>
#include <algorithm>

class Solver;

namespace cmdline {
  typedef std::list< std::string > arglist;

  // exceptions
  struct cmd_line_error : public std::runtime_error {
    cmd_line_error(const std::string& msg) : runtime_error(msg) { }
  };

  template<typename T>
  inline std::pair<bool, T> has_argoption(arglist& args,
                                          const std::string& option) {
    T t = T();
    arglist::iterator i = std::find(args.begin(), args.end(), option);
    if( i != args.end() ) {
      arglist::iterator arg = boost::next(i);
      if( arg == args.end() ) {
        std::string message = option + " needs an argument";
        throw cmd_line_error(message);
      }
      try {
        t = boost::lexical_cast<T>(*arg);
      } catch (...) {
        std::string message = *arg + " is not a suitable argument for " + option;
        throw cmd_line_error(message);
      }
      args.erase(i, boost::next(arg));
      return std::make_pair(true, t);
    } else
      return std::make_pair(false, t);
  }

  inline
  bool has_option(arglist& args, const std::string& option)
  {
    arglist::iterator i = find(args.begin(), args.end(), option);
    if( i != args.end() ) {
      args.erase(i);
      return true;
    } else
      return false;
  }

  void parse_solver_options(Solver &s, arglist& args);

} // namespace cmdline

#endif
