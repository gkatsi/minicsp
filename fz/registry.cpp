/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Guido Tack <tack@gecode.org>
 *
 *  Contributing authors:
 *     Mikael Lagerkvist <lagerkvist@gmail.com>
 *
 *  Copyright:
 *     Guido Tack, 2007
 *     Mikael Lagerkvist, 2009
 *
 *  Last modified:
 *     $Date: 2010-07-21 11:42:47 +0200 (Wed, 21 Jul 2010) $ by $Author: tack $
 *     $Revision: 11243 $
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

#include "registry.hpp"
#include "solver.hpp"
#include "flatzinc.hpp"
#include "cons.hpp"
#include <vector>
#include <set>

using namespace std;

namespace FlatZinc {

  Registry& registry(void) {
    static Registry r;
    return r;
  }

  void
  Registry::post(Solver& s, FlatZincModel &m,
                 const ConExpr& ce, AST::Node* ann) {
    std::map<std::string,poster>::iterator i = r.find(ce.id);
    if (i == r.end()) {
      throw FlatZinc::Error("Registry",
        std::string("Constraint ")+ce.id+" not found");
    }
    i->second(s, m, ce, ann);
  }

  void
  Registry::add(const std::string& id, poster p) {
    r[id] = p;
  }

  namespace {

    int ann2icl(AST::Node* ann) {
      // if (ann) {
      //   if (ann->hasAtom("val"))
      //     return ICL_VAL;
      //   if (ann->hasAtom("domain"))
      //     return ICL_DOM;
      //   if (ann->hasAtom("bounds") ||
      //       ann->hasAtom("boundsR") ||
      //       ann->hasAtom("boundsD") ||
      //       ann->hasAtom("boundsZ"))
      //     return ICL_BND;
      // }
      return 0;
    }

    inline vector<int> arg2intargs(AST::Node* arg, int offset = 0) {
      AST::Array* a = arg->getArray();
      vector<int> ia(a->a.size()+offset);
      for (int i=offset; i--;)
        ia[i] = 0;
      for (int i=a->a.size(); i--;)
        ia[i+offset] = a->a[i]->getInt();
      return ia;
    }

    inline vector<int> arg2boolargs(AST::Node* arg, int offset = 0) {
      AST::Array* a = arg->getArray();
      vector<int> ia(a->a.size()+offset);
      for (int i=offset; i--;)
        ia[i] = 0;
      for (int i=a->a.size(); i--;)
        ia[i+offset] = a->a[i]->getBool();
      return ia;
    }

    inline
    set<int> setrange(int min, int max)
    {
      set<int> rv;
      for(int i = min; i <= max; ++i)
        rv.insert(i);
      return rv;
    }

    inline set<int> arg2intset(Solver& s, AST::Node* n) {
      AST::SetLit* sl = n->getSet();
      set<int> d;
      if (sl->interval) {
        d = setrange(sl->min, sl->max);
      } else {
        for (int i=sl->s.size(); i--; )
          d.insert(sl->s[i]);
      }
      return d;
    }

    inline vector<set<int> > arg2intsetargs(Solver& s,
                                            AST::Node* arg, int offset = 0) {
      AST::Array* a = arg->getArray();
      if (a->a.size() == 0) {
        vector<set<int> > emptyIa(0);
        return emptyIa;
      }
      vector<set<int> > ia(a->a.size()+offset);
      for (int i=a->a.size(); i--;) {
        ia[i+offset] = arg2intset(s, a->a[i]);
      }
      return ia;
    }

    inline vector<cspvar> arg2intvarargs(Solver& s,
                                         FlatZincModel& m,
                                         AST::Node* arg,
                                         int offset = 0) {
      AST::Array* a = arg->getArray();
      if (a->a.size() == 0) {
        vector<cspvar> emptyIa;
        return emptyIa;
      }
      vector<cspvar> ia(a->a.size()+offset);
      for (int i=offset; i--;) {
        if( m.constants.find(0) == m.constants.end() )
          m.constants[0] = s.newCSPVar(0, 0);
        ia[i] = m.constants[0];
      }
      for (int i=a->a.size(); i--;) {
        if (a->a[i]->isIntVar()) {
          ia[i+offset] = m.iv[a->a[i]->getIntVar()];
        } else {
          int value = a->a[i]->getInt();
          if( m.constants.find(value) == m.constants.end() )
            m.constants[value] = s.newCSPVar(value, value);
          cspvar iv = m.constants[value];
          ia[i+offset] = iv;
        }
      }
      return ia;
    }

    inline vector<cspvar> arg2boolvarargs(Solver& s,
                                          FlatZincModel& m,
                                          AST::Node* arg,
                                          int offset = 0, int siv=-1) {
      AST::Array* a = arg->getArray();
      if (a->a.size() == 0) {
        vector<cspvar> emptyIa;
        return emptyIa;
      }
      vector<cspvar> ia(a->a.size()+offset-(siv==-1?0:1));
      for (int i=offset; i--;) {
        ia[i] = s.newCSPVar(0, 0);
      }
      for (int i=0; i<static_cast<int>(a->a.size()); i++) {
        if (i==siv)
          continue;
        if (a->a[i]->isBool()) {
          bool value = a->a[i]->getBool();
          cspvar iv = s.newCSPVar(value, value);
          ia[offset++] = iv;
        } else if (a->a[i]->isIntVar() &&
                   m.aliasBool2Int(a->a[i]->getIntVar()) != -1) {
          ia[offset++] = m.bv[m.aliasBool2Int(a->a[i]->getIntVar())];
        } else {
          ia[offset++] = m.bv[a->a[i]->getBoolVar()];
        }
      }
      return ia;
    }

    cspvar getBoolVar(Solver& s,
                      FlatZincModel& m,
                      AST::Node* n) {
      cspvar x0;
      if (n->isBool()) {
        cspvar& constvar = n->getBool() ? m.vartrue : m.varfalse;
        if( !constvar.valid() )
          constvar = s.newCSPVar(n->getBool(), n->getBool());
        x0 = constvar;
      }
      else {
        x0 = m.bv[n->getBoolVar()];
      }
      return x0;
    }

    cspvar getIntVar(Solver& s,
                     FlatZincModel& m,
                     AST::Node* n) {
      cspvar x0;
      if (n->isIntVar()) {
        x0 = m.iv[n->getIntVar()];
      } else {
        int v = n->getInt();
        if( m.constants.find(v) == m.constants.end() )
          m.constants[v] = s.newCSPVar(v, v);
        x0 = m.constants[v];
      }
      return x0;
    }

    bool isBoolArray(FlatZincModel& m, AST::Node* b) {
      AST::Array* a = b->getArray();
      if (a->a.size() == 0)
        return true;
      for (int i=a->a.size(); i--;) {
        if (a->a[i]->isBoolVar() || a->a[i]->isBool())
          continue;
        if ( !a->a[i]->isIntVar() )
          return false;
        if( m.aliasBool2Int(a->a[i]->getIntVar()) == -1)
          return false;
      }
      return true;
    }

    void p_alldifferent(Solver& s, FlatZincModel &m,
                        const ConExpr& ce, AST::Node* ann) {
      vector<cspvar> va = arg2intvarargs(s, m, ce[0]);
      post_alldiff(s, va);
    }

    void p_int_eq(Solver& s, FlatZincModel &m,
                  const ConExpr& ce, AST::Node* ann) {
      if (ce[0]->isIntVar()) {
        if (ce[1]->isIntVar()) {
          post_eq(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0);
        } else {
          getIntVar(s, m, ce[0]).assign(s, ce[1]->getInt(), NO_REASON);
        }
      } else {
        getIntVar(s, m, ce[1]).assign(s, ce[0]->getInt(), NO_REASON);
      }
    }

    void p_int_neq(Solver& s, FlatZincModel &m,
                   const ConExpr& ce, AST::Node* ann) {
      if (ce[0]->isIntVar()) {
        if (ce[1]->isIntVar()) {
          post_neq(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0);
        } else {
          getIntVar(s, m, ce[0]).remove(s, ce[1]->getInt(), NO_REASON);
        }
      } else {
        getIntVar(s, m, ce[1]).remove(s, ce[0]->getInt(), NO_REASON);
      }
    }

    // ce[0] <= ce[1] + c
    void p_int_leq_c(Solver& s, FlatZincModel &m,
                     AST::Node* ce0, AST::Node* ce1, int c,
                     AST::Node* ann) {
      if (ce0->isIntVar()) {
        if (ce1->isIntVar()) {
          post_leq(s, getIntVar(s, m, ce0), getIntVar(s, m, ce1), c);
        } else {
          getIntVar(s, m, ce0).setmax(s, ce1->getInt()+c, NO_REASON);
        }
      } else {
        getIntVar(s, m, ce1).setmin(s, ce0->getInt()-c, NO_REASON);
      }
    }

    void p_int_geq(Solver& s, FlatZincModel &m,
                   const ConExpr& ce, AST::Node* ann) {
      p_int_leq_c(s, m, ce[1], ce[0], 0, ann);
    }
    void p_int_gt(Solver& s, FlatZincModel &m,
                  const ConExpr& ce, AST::Node* ann) {
      p_int_leq_c(s, m, ce[1], ce[0], -1, ann);
    }
    void p_int_leq(Solver& s, FlatZincModel &m,
                   const ConExpr& ce, AST::Node* ann) {
      p_int_leq_c(s, m, ce[0], ce[1], 0, ann);
    }
    void p_int_lt(Solver& s, FlatZincModel &m,
                  const ConExpr& ce, AST::Node* ann) {
      p_int_leq_c(s, m, ce[0], ce[1], -1, ann);
    }

    /* Comparisons */
    void p_int_eq_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_eq_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                 getBoolVar(s, m, ce[2]));
    }
    void p_int_ne_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_neq_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                  getBoolVar(s, m, ce[2]));
    }
    void p_int_ge_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_geq_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                  getBoolVar(s, m, ce[2]));
    }
    void p_int_gt_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_gt_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                 getBoolVar(s, m, ce[2]));
    }
    void p_int_le_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_leq_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                  getBoolVar(s, m, ce[2]));
    }
    void p_int_lt_reif(Solver& s, FlatZincModel& m,
                       const ConExpr& ce, AST::Node* ann) {
      post_less_re(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0,
                  getBoolVar(s, m, ce[2]));
    }

    /* linear (in-)equations */
    void p_int_lin(Solver& s, FlatZincModel& m,
                   domevent::event_type op, bool strict, bool reif,
                   const ConExpr& ce,
                   AST::Node* ann) {
      vector<int> ia = arg2intargs(ce[0]);
      if (isBoolArray(m,ce[1])) {
        int c = ce[2]->getInt();
        vector<cspvar> iv = arg2boolvarargs(s, m, ce[1]);
        switch(op) {
        case domevent::LEQ:
          for(size_t i = 0; i != ia.size(); ++i)
            ia[i] = -ia[i];
          c = -c;
          // continue on to GEQ
        case domevent::GEQ:
          if( strict ) {
            ++c;
          }
          if( !reif )
            post_pb(s, iv, ia, c);
          else {
            cspvar b = getBoolVar(s, m, ce[3]);
            post_pb_iff_re(s, iv, ia, c, b);
          }
          break;
        case domevent::EQ:
          assert(!strict);
          // pseudo-boolean equality
          break;
        case domevent::NEQ:
          assert(!strict);
          // pseudo-boolean inequality
          break;
        case domevent::NONE: assert(0); break;
        }
      } else {
        vector<cspvar> iv = arg2intvarargs(s, m, ce[1]);
        int c = ce[2]->getInt();
        switch(op) {
        case domevent::GEQ:
          for(size_t i = 0; i != ia.size(); ++i)
            ia[i] = -ia[i];
          c = -c;
          // continue on to LEQ
        case domevent::LEQ:
          if( strict ) {
            ++c;
          }
          if( !reif )
            post_lin_leq(s, iv, ia, -c);
          else {
            cspvar b = getBoolVar(s, m, ce[3]);
            post_lin_leq_iff_re(s, iv, ia, -c, b);
          }
          break;
        case domevent::EQ:
          assert(!strict);
          if( !reif )
            post_lin_eq(s, iv, ia, -c);
          else {
            cspvar b = getBoolVar(s, m, ce[3]);
            post_lin_eq_iff_re(s, iv, ia, -c, b);
          }
          break;
        case domevent::NEQ:
          assert(!strict);
          if( !reif )
            post_lin_neq(s, iv, ia, -c);
          else {
            cspvar b = getBoolVar(s, m, ce[3]);
            post_lin_neq_iff_re(s, iv, ia, -c, b);
          }
          break;
        case domevent::NONE: assert(0); break;
        }
      }
    }

    void p_int_lin_eq(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::EQ, false, false, ce, ann);
    }
    void p_int_lin_eq_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::EQ, false, true, ce, ann);
    }
    void p_int_lin_ne(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::NEQ, false, false, ce, ann);
    }
    void p_int_lin_ne_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::NEQ, false, true, ce, ann);
    }

    void p_int_lin_le(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::LEQ, false, false, ce, ann);
    }
    void p_int_lin_le_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::LEQ, false, true, ce, ann);
    }
    void p_int_lin_lt(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::LEQ, true, false, ce, ann);
    }
    void p_int_lin_lt_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::LEQ, true, true, ce, ann);
    }
    void p_int_lin_ge(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::GEQ, false, false, ce, ann);
    }
    void p_int_lin_ge_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::GEQ, false, true, ce, ann);
    }
    void p_int_lin_gt(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::GEQ, true, false, ce, ann);
    }
    void p_int_lin_gt_reif(Solver& s, FlatZincModel& m,
                           const ConExpr& ce, AST::Node* ann) {
      p_int_lin(s, m, domevent::GEQ, true, true, ce, ann);
    }

    /* arithmetic constraints */

    void p_int_plus(Solver& s, FlatZincModel& m,
                    const ConExpr& ce, AST::Node* ann) {
      if (!ce[0]->isIntVar()) {
        post_eq(s, getIntVar(s, m, ce[1]), getIntVar(s, m, ce[2]),
                -ce[0]->getInt());
      } else if (!ce[1]->isIntVar()) {
        post_eq(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[2]),
                -ce[1]->getInt());
      } else if (!ce[2]->isIntVar()) {
        post_neg(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]),
                 ce[2]->getInt());
      } else {
        vector<cspvar> x(3);
        x[0] = getIntVar(s,m,ce[0]);
        x[1] = getIntVar(s,m,ce[1]);
        x[2] = getIntVar(s,m,ce[2]);
        vector<int> w(3);
        w[0] = 1;
        w[1] = 1;
        w[2] = -1;
        post_lin_eq(s, x, w, 0);
      }
    }

    void p_int_minus(Solver& s, FlatZincModel& m,
                     const ConExpr& ce, AST::Node* ann) {
      if (!ce[0]->isIntVar()) {
        post_neg(s, getIntVar(s, m, ce[2]), getIntVar(s, m, ce[1]),
                 ce[0]->getInt());
      } else if (!ce[1]->isIntVar()) {
        post_eq(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[2]),
                ce[1]->getInt());
      } else if (!ce[2]->isIntVar()) {
        post_eq(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]),
                ce[2]->getInt());
      } else {
        vector<cspvar> x(3);
        x[0] = getIntVar(s,m,ce[0]);
        x[1] = getIntVar(s,m,ce[1]);
        x[2] = getIntVar(s,m,ce[2]);
        vector<int> w(3);
        w[0] = 1;
        w[1] = -1;
        w[2] = -1;
        post_lin_eq(s, x, w, 0);
      }
    }

    void p_int_abs(Solver& s, FlatZincModel& m,
                     const ConExpr& ce, AST::Node* ann) {
      if( !ce[0]->isIntVar()) {
        cspvar y = getIntVar(s, m, ce[1]);
        y.assign(s, abs(ce[0]->getInt()), NO_REASON);
      } else if( !ce[1]->isIntVar()) {
        cspvar x = getIntVar(s, m, ce[0]);
        int y = ce[1]->getInt();
        if( y < 0 ) throw unsat();
        if( x.min(s) <= -y )
          x.setmin(s, -y, NO_REASON);
        else
          x.assign(s, y, NO_REASON);
        if( x.max(s) >= y)
          x.setmax(s, y, NO_REASON);
        else
          x.assign(s, -y, NO_REASON);
        for(int i = x.min(s)+1; i < x.max(s); ++i)
          x.remove(s, i, NO_REASON);
      } else {
        post_abs(s, getIntVar(s, m, ce[0]), getIntVar(s, m, ce[1]), 0);
      }
    }

    void p_int_times(Solver& s, FlatZincModel& m,
                     const ConExpr& ce, AST::Node* ann) {
      cspvar x0 = getIntVar(s, m, ce[0]);
      cspvar x1 = getIntVar(s, m, ce[1]);
      cspvar x2 = getIntVar(s, m, ce[2]);
      post_mult(s, x1, x2, x0); // note the order
    }
    void p_int_div(Solver& s, FlatZincModel& m,
                   const ConExpr& ce, AST::Node* ann) {
      cspvar x0 = getIntVar(s, m, ce[0]);
      cspvar x1 = getIntVar(s, m, ce[1]);
      cspvar x2 = getIntVar(s, m, ce[2]);
      post_div(s, x1, x2, x0); // note the order
    }

    void p_int_negate(Solver& s, FlatZincModel& m,
                      const ConExpr& ce, AST::Node* ann) {
      if( !ce[0]->isIntVar() ) {
        if( !ce[1]->isIntVar() ) {
          if( ce[0]->getInt() != - ce[1]->getInt() )
            throw unsat();
          return;
        }
        cspvar x1 = getIntVar(s, m, ce[1]);
        x1.assign(s, -ce[0]->getInt(), NO_REASON);
      } else if( !ce[1]->isIntVar() ) {
        cspvar x0 = getIntVar(s, m, ce[1]);
        x0.assign(s, -ce[1]->getInt(), NO_REASON);
      } else {
        cspvar x0 = getIntVar(s, m, ce[0]);
        cspvar x1 = getIntVar(s, m, ce[1]);
        post_neg(s, x0, x1, 0);
      }
    }

    void p_int_min(Solver& s, FlatZincModel& m,
                   const ConExpr& ce, AST::Node* ann) {
      cspvar x0 = getIntVar(s, m, ce[0]);
      cspvar x1 = getIntVar(s, m, ce[1]);
      cspvar x2 = getIntVar(s, m, ce[2]);
      post_min(s, x2, x0, x1);
    }
    void p_int_max(Solver& s, FlatZincModel& m,
                   const ConExpr& ce, AST::Node* ann) {
      cspvar x0 = getIntVar(s, m, ce[0]);
      cspvar x1 = getIntVar(s, m, ce[1]);
      cspvar x2 = getIntVar(s, m, ce[2]);
      post_max(s, x2, x0, x1);
    }

#if 0
    void p_int_mod(Solver& s, const ConExpr& ce, AST::Node* ann) {
      IntVar x0 = getIntVar(s, ce[0]);
      IntVar x1 = getIntVar(s, ce[1]);
      IntVar x2 = getIntVar(s, ce[2]);
      mod(s,x0,x1,x2, ann2icl(ann));
    }

    /* Boolean constraints */
    void p_bool_CMP(Solver& s, IntRelType irt, const ConExpr& ce,
                   AST::Node* ann) {
      rel(s, getBoolVar(s, ce[0]), irt, getBoolVar(s, ce[1]),
          ann2icl(ann));
    }
    void p_bool_CMP_reif(Solver& s, IntRelType irt, const ConExpr& ce,
                   AST::Node* ann) {
      rel(s, getBoolVar(s, ce[0]), irt, getBoolVar(s, ce[1]),
          getBoolVar(s, ce[2]), ann2icl(ann));
    }
    void p_bool_eq(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_EQ, ce, ann);
    }
    void p_bool_eq_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_EQ, ce, ann);
    }
    void p_bool_ne(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_NQ, ce, ann);
    }
    void p_bool_ne_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_NQ, ce, ann);
    }
    void p_bool_ge(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_GQ, ce, ann);
    }
    void p_bool_ge_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_GQ, ce, ann);
    }
    void p_bool_le(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_LQ, ce, ann);
    }
    void p_bool_le_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_LQ, ce, ann);
    }
    void p_bool_gt(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_GR, ce, ann);
    }
    void p_bool_gt_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_GR, ce, ann);
    }
    void p_bool_lt(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP(s, IRT_LE, ce, ann);
    }
    void p_bool_lt_reif(Solver& s, const ConExpr& ce, AST::Node* ann) {
      p_bool_CMP_reif(s, IRT_LE, ce, ann);
    }

#define BOOL_OP(op) \
    BoolVar b0 = getBoolVar(s, ce[0]); \
    BoolVar b1 = getBoolVar(s, ce[1]); \
    if (ce[2]->isBool()) { \
      rel(s, b0, op, b1, ce[2]->getBool(), ann2icl(ann)); \
    } else { \
      rel(s, b0, op, b1, s.bv[ce[2]->getBoolVar()], ann2icl(ann)); \
    }

#define BOOL_ARRAY_OP(op) \
    BoolVarArgs bv = arg2boolvarargs(s, ce[0]); \
    if (ce[1]->isBool()) { \
      rel(s, op, bv, ce[1]->getBool(), ann2icl(ann)); \
    } else { \
      rel(s, op, bv, s.bv[ce[1]->getBoolVar()], ann2icl(ann)); \
    }

    void p_bool_or(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BOOL_OP(BOT_OR);
    }
    void p_bool_and(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BOOL_OP(BOT_AND);
    }
    void p_array_bool_and(Solver& s, const ConExpr& ce, AST::Node* ann)
    {
      BOOL_ARRAY_OP(BOT_AND);
    }
    void p_array_bool_or(Solver& s, const ConExpr& ce, AST::Node* ann)
    {
      BOOL_ARRAY_OP(BOT_OR);
    }
    void p_array_bool_clause(Solver& s, const ConExpr& ce,
                             AST::Node* ann) {
      BoolVarArgs bvp = arg2boolvarargs(s, ce[0]);
      BoolVarArgs bvn = arg2boolvarargs(s, ce[1]);
      clause(s, BOT_OR, bvp, bvn, 1, ann2icl(ann));
    }
    void p_array_bool_clause_reif(Solver& s, const ConExpr& ce,
                             AST::Node* ann) {
      BoolVarArgs bvp = arg2boolvarargs(s, ce[0]);
      BoolVarArgs bvn = arg2boolvarargs(s, ce[1]);
      BoolVar b0 = getBoolVar(s, ce[2]);
      clause(s, BOT_OR, bvp, bvn, b0, ann2icl(ann));
    }
    void p_bool_xor(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BOOL_OP(BOT_XOR);
    }
    void p_bool_l_imp(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BoolVar b0 = getBoolVar(s, ce[0]);
      BoolVar b1 = getBoolVar(s, ce[1]);
      if (ce[2]->isBool()) {
        rel(s, b1, BOT_IMP, b0, ce[2]->getBool(), ann2icl(ann));
      } else {
        rel(s, b1, BOT_IMP, b0, s.bv[ce[2]->getBoolVar()], ann2icl(ann));
      }
    }
    void p_bool_r_imp(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BOOL_OP(BOT_IMP);
    }
    void p_bool_not(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BoolVar x0 = getBoolVar(s, ce[0]);
      BoolVar x1 = getBoolVar(s, ce[1]);
      rel(s, x0, BOT_XOR, x1, 1, ann2icl(ann));
    }

    /* element constraints */
    void p_array_int_element(Solver& s, const ConExpr& ce,
                                 AST::Node* ann) {
      bool isConstant = true;
      AST::Array* a = ce[1]->getArray();
      for (int i=a->a.size(); i--;) {
        if (!a->a[i]->isInt()) {
          isConstant = false;
          break;
        }
      }
      IntVar selector = getIntVar(s, ce[0]);
      rel(s, selector > 0);
      if (isConstant) {
        IntArgs ia = arg2intargs(ce[1], 1);
        element(s, ia, selector, getIntVar(s, ce[2]), ann2icl(ann));
      } else {
        IntVarArgs iv = arg2intvarargs(s, ce[1], 1);
        element(s, iv, selector, getIntVar(s, ce[2]), ann2icl(ann));
      }
    }
    void p_array_bool_element(Solver& s, const ConExpr& ce,
                                  AST::Node* ann) {
      bool isConstant = true;
      AST::Array* a = ce[1]->getArray();
      for (int i=a->a.size(); i--;) {
        if (!a->a[i]->isBool()) {
          isConstant = false;
          break;
        }
      }
      IntVar selector = getIntVar(s, ce[0]);
      rel(s, selector > 0);
      if (isConstant) {
        IntArgs ia = arg2boolargs(ce[1], 1);
        element(s, ia, selector, getBoolVar(s, ce[2]), ann2icl(ann));
      } else {
        BoolVarArgs iv = arg2boolvarargs(s, ce[1], 1);
        element(s, iv, selector, getBoolVar(s, ce[2]), ann2icl(ann));
      }
    }

    /* coercion constraints */
    void p_bool2int(Solver& s, const ConExpr& ce, AST::Node* ann) {
      BoolVar x0 = getBoolVar(s, ce[0]);
      IntVar x1 = getIntVar(s, ce[1]);
      if (ce[0]->isBoolVar() && ce[1]->isIntVar()) {
        s.aliasBool2Int(ce[1]->getIntVar(), ce[0]->getBoolVar());
      }
      channel(s, x0, x1, ann2icl(ann));
    }

    void p_int_in(Solver& s, const ConExpr& ce, AST::Node *) {
      IntSet d = arg2intset(s,ce[1]);
      if (ce[0]->isBoolVar()) {
        IntSetRanges dr(d);
        Iter::Ranges::Singleton sr(0,1);
        Iter::Ranges::Inter<IntSetRanges,Iter::Ranges::Singleton> i(dr,sr);
        IntSet d01(i);
        if (d01.size() == 0) {
          s.fail();
        } else {
          rel(s, getBoolVar(s, ce[0]), IRT_GQ, d01.min());
          rel(s, getBoolVar(s, ce[0]), IRT_LQ, d01.max());
        }
      } else {
        dom(s, getIntVar(s, ce[0]), d);
      }
    }
#endif

    class IntPoster {
    public:
      IntPoster(void) {
        registry().add("int_eq", &p_int_eq);
        registry().add("int_ne", &p_int_neq);
        registry().add("int_ge", &p_int_geq);
        registry().add("int_gt", &p_int_gt);
        registry().add("int_le", &p_int_leq);
        registry().add("int_lt", &p_int_lt);
        registry().add("int_eq_reif", &p_int_eq_reif);
        registry().add("int_ne_reif", &p_int_ne_reif);
        registry().add("int_ge_reif", &p_int_ge_reif);
        registry().add("int_gt_reif", &p_int_gt_reif);
        registry().add("int_le_reif", &p_int_le_reif);
        registry().add("int_lt_reif", &p_int_lt_reif);
        registry().add("int_lin_le", &p_int_lin_le);
        registry().add("int_lin_le_reif", &p_int_lin_le_reif);
        registry().add("int_lin_lt", &p_int_lin_lt);
        registry().add("int_lin_lt_reif", &p_int_lin_lt_reif);
        registry().add("int_lin_ge", &p_int_lin_ge);
        registry().add("int_lin_ge_reif", &p_int_lin_ge_reif);
        registry().add("int_lin_gt", &p_int_lin_gt);
        registry().add("int_lin_gt_reif", &p_int_lin_gt_reif);
        registry().add("int_plus", &p_int_plus);
        registry().add("int_minus", &p_int_minus);
        registry().add("int_abs", &p_int_abs);
        registry().add("int_times", &p_int_times);
        registry().add("int_div", &p_int_div);
        registry().add("int_negate", &p_int_negate);
        registry().add("int_min", &p_int_min);
        registry().add("int_max", &p_int_max);
      }
    };
    IntPoster __int_poster;
  }
}
