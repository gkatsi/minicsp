/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Copyright:
 *     Guido Tack, 2007
 *
 *  Last modified:
 *     $Date: 2010-05-11 12:33:38 +0200 (Tue, 11 May 2010) $ by $Author: tack $
 *     $Revision: 10940 $
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "flatzinc.hpp"
#include "registry.hpp"

#include <vector>
#include <string>
#include <set>
using namespace std;

namespace FlatZinc {
  inline
  set<int> setrange(int min, int max)
  {
    set<int> rv;
    for(int i = min; i <= max; ++i)
      rv.insert(i);
    return rv;
  }

  set<int> vs2is(IntVarSpec* vs) {
    if (vs->assigned) {
      return setrange(vs->i,vs->i);
    }
    if (vs->domain()) {
      AST::SetLit* sl = vs->domain.some();
      if (sl->interval) {
        return setrange(sl->min, sl->max);
      } else {
        set<int> rv;
        for (int i=sl->s.size(); i--;)
          rv.insert(sl->s[i]);
        return rv;
      }
    }
    return setrange(-1000, 1000);
  }

  int vs2bsl(BoolVarSpec* bs) {
    if (bs->assigned) {
      return bs->i;
    }
    if (bs->domain()) {
      AST::SetLit* sl = bs->domain.some();
      assert(sl->interval);
      return std::min(1, std::max(0, sl->min));
    }
    return 0;
  }

  int vs2bsh(BoolVarSpec* bs) {
    if (bs->assigned) {
      return bs->i;
    }
    if (bs->domain()) {
      AST::SetLit* sl = bs->domain.some();
      assert(sl->interval);
      return std::max(0, std::min(1, sl->max));
    }
    return 1;
  }

  int ann2ivarsel(AST::Node* ann) {
    if (/*AST::Atom* s =*/ dynamic_cast<AST::Atom*>(ann)) {
      // if (s->id == "input_order")
      //   return TieBreakVarBranch<IntVarBranch>(INT_VAR_NONE);
    }
    std::cerr << "Warning, ignored search annotation: ";
    ann->print(std::cerr);
    std::cerr << std::endl;
    return 0;
  }

  int ann2ivalsel(AST::Node* ann) {
    if (/*AST::Atom* s =*/ dynamic_cast<AST::Atom*>(ann)) {
      // if (s->id == "indomain_min")
      //   return INT_VAL_MIN;
    }
    std::cerr << "Warning, ignored search annotation: ";
    ann->print(std::cerr);
    std::cerr << std::endl;
    return 0;
  }

  int ann2asnivalsel(AST::Node* ann) {
    if (/*AST::Atom* s =*/ dynamic_cast<AST::Atom*>(ann)) {
      // if (s->id == "indomain_min")
      //   return INT_ASSIGN_MIN;
    }
    std::cerr << "Warning, ignored search annotation: ";
    ann->print(std::cerr);
    std::cerr << std::endl;
    return 0;
  }


  FlatZincModel::FlatZincModel(Solver &s)
    : solver(s),
      intVarCount(-1), boolVarCount(-1), setVarCount(-1), _optVar(-1),
      _solveAnnotations(NULL) {}

  void
  FlatZincModel::init(int intVars, int boolVars, int setvars) {
    intVarCount = 0;
    iv = IntVarArray(intVars);
    iv_introduced = std::vector<bool>(intVars);
    iv_boolalias = std::vector<int>(intVars);
    boolVarCount = 0;
    bv = BoolVarArray(boolVars);
    bv_introduced = std::vector<bool>(boolVars);
  }

  void
  FlatZincModel::newIntVar(IntVarSpec* vs) {
    if (vs->alias) {
      iv[intVarCount++] = iv[vs->i];
    } else {
      set<int> domain = vs2is(vs);
      cspvar x = solver.newCSPVar(*domain.begin(), *domain.rbegin());
      iv[intVarCount++] = x;
      int prev = *domain.begin();
      for(set<int>::const_iterator i = domain.begin(), end = domain.end();
          i != end; ++i) {
        if( *i > prev+1 ) {
          for(int q = prev+1; q != *i; ++q)
            x.remove(solver, q, NO_REASON);
        }
        prev = *i;
      }
    }
    iv_introduced[intVarCount-1] = vs->introduced;
    iv_boolalias[intVarCount-1] = -1;
  }

  void
  FlatZincModel::newSetVar(SetVarSpec* vs) {
    throw FlatZinc::Error("minicsp", "set variables not supported");
  }

  void
  FlatZincModel::aliasBool2Int(int iv, int bv) {
    iv_boolalias[iv] = bv;
  }
  int
  FlatZincModel::aliasBool2Int(int iv) {
    return iv_boolalias[iv];
  }

  void
  FlatZincModel::newBoolVar(BoolVarSpec* vs) {
    if (vs->alias) {
      bv[boolVarCount++] = bv[vs->i];
    } else {
      bv[boolVarCount++] = solver.newCSPVar(0, 1);
    }
    bv_introduced[boolVarCount-1] = vs->introduced;
  }

  void
  FlatZincModel::postConstraint(const ConExpr& ce, AST::Node* ann) {
    try {
      registry().post(solver, *this, ce, ann);
    } catch (unsat& e) {
      throw FlatZinc::Error("unsat", e.what());
    } catch (AST::TypeError& e) {
      throw FlatZinc::Error("Type error", e.what());
    }
  }

  void flattenAnnotations(AST::Array* ann, std::vector<AST::Node*>& out) {
      for (unsigned int i=0; i<ann->a.size(); i++) {
        if (ann->a[i]->isCall("seq_search")) {
          AST::Call* c = ann->a[i]->getCall();
          if (c->args->isArray())
            flattenAnnotations(c->args->getArray(), out);
          else
            out.push_back(c->args);
        } else {
          out.push_back(ann->a[i]);
        }
      }
  }

  void
  FlatZincModel::createBranchers(AST::Node* ann, bool ignoreUnknown,
                                 std::ostream& err) {
    if (ann) {
      err << "Warning, ignored search annotation: ";
      ann->print(err);
      err << std::endl;
    }
  }

  AST::Array*
  FlatZincModel::solveAnnotations(void) const {
    return _solveAnnotations;
  }

  void
  FlatZincModel::solve(AST::Array* ann) {
    _method = SAT;
    _solveAnnotations = ann;
  }

  void
  FlatZincModel::minimize(int var, AST::Array* ann) {
    _method = MIN;
    _optVar = var;
    _solveAnnotations = ann;
    // Branch on optimization variable to ensure that it is given a value.
    AST::Array* args = new AST::Array(4);
    args->a[0] = new AST::Array(new AST::IntVar(_optVar));
    args->a[1] = new AST::Atom("input_order");
    args->a[2] = new AST::Atom("indomain_min");
    args->a[3] = new AST::Atom("complete");
    AST::Call* c = new AST::Call("int_search", args);
    if (!ann)
      ann = new AST::Array(c);
    else
      ann->a.push_back(c);
  }

  void
  FlatZincModel::maximize(int var, AST::Array* ann) {
    _method = MAX;
    _optVar = var;
    _solveAnnotations = ann;
    // Branch on optimization variable to ensure that it is given a value.
    AST::Array* args = new AST::Array(4);
    args->a[0] = new AST::Array(new AST::IntVar(_optVar));
    args->a[1] = new AST::Atom("input_order");
    args->a[2] = new AST::Atom("indomain_min");
    args->a[3] = new AST::Atom("complete");
    AST::Call* c = new AST::Call("int_search", args);
    if (!ann)
      ann = new AST::Array(c);
    else
      ann->a.push_back(c);
  }

  FlatZincModel::~FlatZincModel(void) {
    delete _solveAnnotations;
  }

  void
  FlatZincModel::run(std::ostream& out, const Printer& p) {
    assert(0);
    // solve it
    switch (_method) {
    case MIN:
    case MAX:
      break;
    case SAT:
      break;
    }
  }

  void
  FlatZincModel::constrain() {
    int opt = solver.cspModelValue(iv[_optVar]);
    if (_method == MIN) {
      iv[_optVar].setmax( solver, opt-1, NO_REASON );
    } else if (_method == MAX) {
      iv[_optVar].setmax( solver, opt+1, NO_REASON );
    }
  }

  FlatZincModel::Meth
  FlatZincModel::method(void) const {
    return _method;
  }

  int
  FlatZincModel::optVar(void) const {
    return _optVar;
  }

  void
  FlatZincModel::print(std::ostream& out, const Printer& p) const {
    p.print(out, solver, iv, bv);
  }

  void
  Printer::init(AST::Array* output) {
    _output = output;
  }

  void
  Printer::printElem(std::ostream& out,
                     Solver& solver,
                       AST::Node* ai,
                       const IntVarArray& iv,
                       const BoolVarArray& bv
                       ) const {
    int k;
    if (ai->isInt(k)) {
      out << k;
    } else if (ai->isIntVar()) {
      pair<int, int> v = solver.cspModelRange(iv[ai->getIntVar()]);
      if( v.first == v.second )
        out << v.first;
      else
        out << v.first << ".." << v.second;
    } else if (ai->isBoolVar()) {
      if (bv[ai->getBoolVar()].min(solver) == 1) {
        out << "true";
      } else if (bv[ai->getBoolVar()].max(solver) == 0) {
        out << "false";
      } else {
        out << "false..true";
      }
    } else if (ai->isBool()) {
      out << (ai->getBool() ? "true" : "false");
    } else if (ai->isSet()) {
      AST::SetLit* s = ai->getSet();
      if (s->interval) {
        out << s->min << ".." << s->max;
      } else {
        out << "{";
        for (unsigned int i=0; i<s->s.size(); i++) {
          out << s->s[i] << (i < s->s.size()-1 ? ", " : "}");
        }
      }
    } else if (ai->isString()) {
      std::string s = ai->getString();
      for (unsigned int i=0; i<s.size(); i++) {
        if (s[i] == '\\' && i<s.size()-1) {
          switch (s[i+1]) {
          case 'n': out << "\n"; break;
          case '\\': out << "\\"; break;
          case 't': out << "\t"; break;
          default: out << "\\" << s[i+1];
          }
          i++;
        } else {
          out << s[i];
        }
      }
    }
  }

  void
  Printer::print(std::ostream& out,
                 Solver& solver,
                 const IntVarArray& iv,
                 const BoolVarArray& bv) const {
    if (_output == NULL)
      return;
    for (unsigned int i=0; i< _output->a.size(); i++) {
      AST::Node* ai = _output->a[i];
      if (ai->isArray()) {
        AST::Array* aia = ai->getArray();
        int size = aia->a.size();
        out << "[";
        for (int j=0; j<size; j++) {
          printElem(out,solver, aia->a[j],iv,bv);
          if (j<size-1)
            out << ", ";
        }
        out << "]";
      } else {
        printElem(out,solver,ai,iv,bv);
      }
    }
  }

  Printer::~Printer(void) {
    delete _output;
  }

}

// STATISTICS: flatzinc-any
