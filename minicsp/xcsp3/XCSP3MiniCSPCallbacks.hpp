/*=============================================================================
 * parser for CSP instances represented in XCSP3 Format
 *
 * Copyright (c) 2015 xcp3.org (contact <at> xcsp3.org)
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
#ifndef _MINICSP_XCSP3_CALLBAKCS_H
#define _MINICSP_XCSP3_CALLBAKCS_H

#include <map>
#include "XCSP3CoreCallbacks.h"
#include "XCSP3Variable.h"
#include "minicsp/core/cons.hpp"
#include "Tree.hpp"



/**
 * This is an example that prints useful informations of a XCSP3 instance.
 * You need to create your own class and to override functions of the callback.
 * We suggest to make a map between XVariable and your own variables in order to
 * facilitate the constructions of constraints.
 *
 * see main.cc to show declaration of the parser
 *
 */

namespace XCSP3Core {
    using namespace minicsp;

    struct unsupported {
        string name;


        unsupported(string n) : name(n) {}
    };

    class XCSP3MiniCSPCallbacks : public XCSP3CoreCallbacks {


    public:
        Solver &solver;
        map<string, cspvar> tocspvars;
        map<int, cspvar> constants;

        vector<vector<int>> *previousTuples;


        XCSP3MiniCSPCallbacks(Solver &s) : XCSP3CoreCallbacks(), solver(s) {
            recognizeSpecialCountCases = false;
            recognizeNValuesCases = false;
        }


        ~XCSP3MiniCSPCallbacks() {}


        void print_solution() {
            map<string, cspvar>::const_iterator i, b = tocspvars.begin(), e = tocspvars.end();
            cout << "\n<instantiation type='solution'>\n<list>";
            for(i = b ; i != e ; ++i)
                cout << i->first << " ";
            cout << "</list>\n<values>";
            for(i = b ; i != e ; ++i)
                cout << solver.cspModelValue(i->second) << " ";

            cout << "\n</values>\n</instantiation>\n";
        }


        // ---------------------------- XVariable -> minicsp variables -------------------------------

        vector<cspvar> xvars2cspvars(vector<XVariable *> &xvars) {
            vector<cspvar> v(xvars.size());
            for(int i = 0 ; i != xvars.size() ; ++i) {
                v[i] = tocspvars[xvars[i]->id];
            }
            return v;
        }


        cspvar constant(int c) {
            if(constants.find(c) == constants.end())
                constants[c] = solver.newCSPVar(c, c);
            return constants[c];
        }

        // ---------------------------- VARIABLES ------------------------------------------

        void buildVariableInteger(string id, int minValue, int maxValue) override {
            cspvar x = solver.newCSPVar(minValue, maxValue);
            solver.setCSPVarName(x, id);
            tocspvars[id] = x;
        }


        void buildVariableInteger(string id, vector<int> &values) override {
            for(int i = 0 ; i < values.size() - 1 ; i++)
                if(values[i + 1] == values[i])
                    throw unsupported("Probem : domain with identical value: " + id);

            cspvar x = solver.newCSPVar(values[0], values.back());
            int v = values[0];
            for(int i = 1 ; i < values.size() ; i++) {
                v++;
                while(v != values[i])
                    x.remove(solver, v++, NO_REASON);
            }
            solver.setCSPVarName(x, id);
            tocspvars[id] = x;
        }

        // ---------------------------- EXTENSION ------------------------------------------

        void buildConstraintExtension(string id, vector<XVariable *> list, vector<vector<int>> &tuples, bool support, bool hasStar) override {
            if(hasStar)
                throw unsupported("* not supported in extensional constraints");
            previousTuples = &tuples;
            if(support)
                post_positive_table(solver, xvars2cspvars(list), tuples);
            else
                post_negative_table(solver, xvars2cspvars(list), tuples);
        }


        void buildConstraintExtension(string id, XVariable *variable, vector<int> &tuple, bool support, bool hasStar) override {
            vector<XVariable *> list;
            list.push_back(variable);
            vector<vector<int>> tuples;
            tuples.push_back(tuple);
            buildConstraintExtension(id, list, tuples, support, hasStar);
        }


        void buildConstraintExtensionAs(string id, vector<XVariable *> list, bool support, bool hasStar) override {
            buildConstraintExtension(id, list, *previousTuples, support, hasStar);
        }


        // ---------------------------- INTENSION + PRIMITIVES ------------------------------------------

        cspvar postExpression(Node *n, bool isRoot = false);


        void buildConstraintIntension(string id, string expr) override {
            Tree tree(expr);
            postExpression(tree.root, true);
        }

//        void buildConstraintPrimitive(string id, OrderType op, XVariable *x, int k, XVariable *y) override;


        // ---------------------------- LANGUAGES ------------------------------------------

        void buildConstraintRegular(string id, vector<XVariable *> &list, string st, string final, vector<XTransition> &transitions) override {
            // TODO CHECK
            throw unsupported("regular in progress");

            map<string, size_t> states;
            size_t current = 1;
            for(XTransition xt : transitions) {
                if(states.find(xt.from) == states.end())
                    states[xt.from] = current++;
                if(states.find(xt.to) == states.end())
                    states[xt.to] = current++;
            }

            vector<regular::transition> minitransitions;
            for(XTransition xt : transitions) {
                regular::transition t(states[xt.from], xt.val, states[xt.to]);
                minitransitions.push_back(t);
            }
            set<int> finals;
            finals.insert(states[final]);
            regular::automaton aut(minitransitions, states[st], finals);
            post_regular(solver, xvars2cspvars(list), aut);
        }

        // ---------------------------- ALLDIFF ALLEQUAL ------------------------------------------

        void buildConstraintAlldifferent(string id, vector<XVariable *> &list) override {
            post_alldiff(solver, xvars2cspvars(list));
        }


        void buildConstraintAllEqual(string id, vector<XVariable *> &list) override {
            vector<cspvar> vars = xvars2cspvars(list);
            for(int i = 0 ; i < vars.size() - 1 ; i++)
                post_eq(solver, vars[i], vars[i + 1], 0);
        }

        // ---------------------------- ORDERED ------------------------------------------

        void buildConstraintOrdered(string id, vector<XVariable *> &list, OrderType order) override {
            vector<cspvar> vars = xvars2cspvars(list);
            for(int i = 0 ; i < vars.size() - 1 ; i++) {
                if(order == LE)
                    post_leq(solver, vars[i], vars[i + 1], 0);
                if(order == LT)
                    post_less(solver, vars[i], vars[i + 1], 0);
                if(order == GE)
                    post_leq(solver, vars[i + 1], vars[i], 0);
                if(order == GT)
                    post_less(solver, vars[i + 1], vars[i], 0);
            }
        }


        void buildConstraintLex(string id, vector<vector<XVariable *>> &lists, OrderType order) override {
            vector<cspvar> vars1, vars2;
            for(int i = 0 ; i < lists.size() - 1 ; i++) {
                vars1 = xvars2cspvars(lists[i]);
                vars2 = xvars2cspvars(lists[i + 1]);
                if(order == LE)
                    post_lex_leq(solver, vars1, vars2);
                if(order == LT)
                    post_lex_less(solver, vars1, vars2);
                if(order == GE)
                    post_lex_leq(solver, vars2, vars1);
                if(order == GT)
                    post_lex_less(solver, vars2, vars1);
            }
        }


        void buildConstraintLexMatrix(string id, vector<vector<XVariable *>> &matrix, OrderType order) override {
            vector<cspvar> vars1, vars2;
            // lines
            buildConstraintLex(id, matrix, order);

            //columns
            vector<vector<XVariable *>> tmatrix(matrix[0].size());
            for(int i = 0 ; i < tmatrix.size() ; i++) {
                tmatrix[i].reserve(matrix.size());
                for(int j = 0 ; j < matrix.size() ; j++)
                    tmatrix[i][j] = matrix[j][i];
            }
            buildConstraintLex(id, tmatrix, order);
        }


        // ---------------------------- SUM ------------------------------------------

        void postSum(string id, vector<cspvar> &list, vector<int> &coefs, XCondition &xc) {
            xc.val = -xc.val;
            switch(xc.op) {
                case EQ:
                    post_lin_eq(solver, list, coefs, xc.val);
                    break;
                case NE:
                    post_lin_neq(solver, list, coefs, xc.val);
                    break;
                case GE:
                    for(int i = 0 ; i != coefs.size() ; ++i) coefs[i] = -coefs[i];
                    post_lin_leq(solver, list, coefs, xc.val);
                    break;
                case GT:
                    for(int i = 0 ; i != coefs.size() ; ++i) coefs[i] = -coefs[i];
                    post_lin_less(solver, list, coefs, xc.val);
                    break;
                case LE:
                    post_lin_leq(solver, list, coefs, xc.val);
                    break;
                case LT:
                    post_lin_less(solver, list, coefs, xc.val);
                    break;
                default:
                    throw unsupported("this sum is not supported");
            }
        }


        void buildConstraintSum(string id, vector<XVariable *> &list, vector<int> &coeffs, XCondition &xc) override {
            vector<cspvar> variables = xvars2cspvars(list);
            if(xc.operandType == VARIABLE) {
                xc.operandType = INTEGER;
                xc.val = 0;
                variables.push_back(tocspvars[xc.var]);
                coeffs.push_back(-1);
            }
            if(xc.op != IN) {
                postSum(id, variables, coeffs, xc);
                return;
            }
            // Intervals
            xc.op = GE;
            xc.val = xc.min;
            postSum(id, variables, coeffs, xc);
            xc.op = LE;
            xc.val = xc.max;
            postSum(id, variables, coeffs, xc);
        }


        void buildConstraintSum(string id, vector<XVariable *> &list, XCondition &xc) override {
            vector<int> coeffs;
            coeffs.assign(list.size(), 1);
            buildConstraintSum(id, list, coeffs, xc);
        }


        void buildConstraintNValues(string id, vector<XVariable *> &list, XCondition &xc) override {
            if(xc.op != LE)
                throw unsupported("nValues is only supported with atMost");
            vector<cspvar> vars = xvars2cspvars(list);
            cspvar n;
            switch(xc.operandType) {
                case INTEGER :
                    n = constant(xc.val);
                    break;
                case VARIABLE :
                    n = tocspvars[xc.var];
                    break;
                default :
                    throw unsupported("nValues with interval is not yet supported");
            }
            post_atmostnvalue(solver, vars, n);
        }



        // ---------------------------- ELEMENT ------------------------------------------

        void
        buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank, int value) override {
            if(rank != ANY)
                throw unsupported("Basic element is only supported");
            post_element(solver, constant(value), tocspvars[index->id], xvars2cspvars(list), startIndex);
        }


        void
        buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank, XVariable *value) override {
            if(rank != ANY)
                throw unsupported("Basic element is only supported");
            post_element(solver, tocspvars[value->id], tocspvars[index->id], xvars2cspvars(list), startIndex);
        }


    };

    // Post expression for intensional constraint


    cspvar XCSP3MiniCSPCallbacks::postExpression(Node *n, bool root) {
        cspvar rv;
        if(n->type == NT_VARIABLE) {
            assert(!root);
            NodeVariable *nv = (NodeVariable *) n;
            return tocspvars[nv->var];
        }

        if(n->type == NT_CONSTANT) {
            assert(!root);
            NodeConstant *nc = (NodeConstant *) n;
            return constant(nc->val);

        }

        NodeOperator *fn = (NodeOperator *) n;

        if(fn->type == NT_EQ) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_eq(solver, x1, x2, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_eq_re(solver, x1, x2, 0, rv);
            }
        }

        if(fn->type == NT_NE) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_neq(solver, x1, x2, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_neq_re(solver, x1, x2, 0, rv);
            }
        }

        if(fn->type == NT_GE) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_leq(solver, x2, x1, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_geq_re(solver, x1, x2, 0, rv);
            }
        }

        if(fn->type == NT_GT) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_less(solver, x2, x1, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_gt_re(solver, x1, x2, 0, rv);
            }
        }


        if(fn->type == NT_LE) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_leq(solver, x1, x2, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_leq_re(solver, x1, x2, 0, rv);
            }
        }

        if(fn->type == NT_LT) {
            cspvar x1 = postExpression(fn->args[0]);
            cspvar x2 = postExpression(fn->args[1]);
            if(root)
                post_less(solver, x1, x2, 0);
            else {
                rv = solver.newCSPVar(0, 1);
                post_less_re(solver, x1, x2, 0, rv);
            }
        }

        if(fn->type == NT_OR) {
            if(root) {
                vec<Lit> ps;
                for(int i = 0 ; i != fn->args.size() ; ++i) {
                    cspvar arg = postExpression(fn->args[i]);
                    ps.push(arg.r_eq(solver, 0));
                }
                solver.addClause(ps);
            } else {
                vec<Lit> ps;
                rv = solver.newCSPVar(0, 1);
                ps.push(rv.e_eq(solver, 0));
                for(int i = 0 ; i != fn->args.size() ; ++i) {
                    cspvar arg = postExpression(fn->args[i]);

                    vec<Lit> ps1;
                    ps1.push(rv.r_eq(solver, 0));
                    ps1.push(arg.e_eq(solver, 0));
                    solver.addClause(ps1);

                    ps.push(arg.r_eq(solver, 0));
                }
                solver.addClause(ps);
            }
        }

        if(fn->type == NT_AND) {
            if(root) {
                for(int i = 0 ; i != fn->args.size() ; ++i)
                    postExpression(fn->args[i]);
            } else {
                vec<Lit> ps;
                rv = solver.newCSPVar(0, 1);
                ps.push(rv.r_eq(solver, 0));
                for(int i = 0 ; i != fn->args.size() ; ++i) {
                    cspvar arg = postExpression(fn->args[i]);

                    vec<Lit> ps1;
                    ps1.push(rv.e_eq(solver, 0));
                    ps1.push(arg.r_eq(solver, 0));
                    solver.addClause(ps1);

                    ps.push(arg.e_eq(solver, 0));
                }
                solver.addClause(ps);
            }
        }

        if(fn->type == NT_NOT) {
            if(root) {
                cspvar x = postExpression(fn->args[0]);
                DO_OR_THROW(x.assign(solver, 0, NO_REASON));
            } else {
                vec<Lit> ps;
                rv = solver.newCSPVar(0, 1);
                cspvar arg = postExpression(fn->args[0]);
                vec<Lit> ps1, ps2;
                ps1.push(rv.r_eq(solver, 0));
                ps1.push(arg.e_neq(solver, 0));
                ps2.push(rv.r_neq(solver, 0));
                ps2.push(arg.e_eq(solver, 0));
                solver.addClause(ps1);
                solver.addClause(ps2);
            }
        }
        if(fn->type == NT_IFF) {
            if(root) {
                cspvar arg1 = postExpression(fn->args[0]),
                        arg2 = postExpression(fn->args[1]);
                vec<Lit> ps1, ps2;
                ps1.push(arg1.r_eq(solver, 0));
                ps1.push(arg2.e_eq(solver, 0));
                ps2.push(arg1.r_neq(solver, 0));
                ps2.push(arg2.e_neq(solver, 0));
                solver.addClause(ps1);
                solver.addClause(ps2);
            } else {
                vec<Lit> ps;
                rv = solver.newCSPVar(0, 1);
                cspvar arg1 = postExpression(fn->args[0]),
                        arg2 = postExpression(fn->args[1]);
                vec<Lit> ps1, ps2, ps3, ps4;
                ps1.push(rv.r_neq(solver, 0));
                ps1.push(arg1.r_eq(solver, 0));
                ps1.push(arg2.e_eq(solver, 0));
                ps2.push(rv.r_neq(solver, 0));
                ps2.push(arg1.r_neq(solver, 0));
                ps2.push(arg2.e_neq(solver, 0));

                ps3.push(rv.r_eq(solver, 0));
                ps3.push(arg1.r_eq(solver, 0));
                ps3.push(arg2.e_neq(solver, 0));
                ps4.push(rv.r_eq(solver, 0));
                ps4.push(arg1.r_neq(solver, 0));
                ps4.push(arg2.e_eq(solver, 0));
                solver.addClause(ps1);
                solver.addClause(ps2);
                solver.addClause(ps3);
                solver.addClause(ps4);
            }
        }

        if(fn->type == NT_XOR) {
            if(root) {
                cspvar arg1 = postExpression(fn->args[0]),
                        arg2 = postExpression(fn->args[1]);
                vec<Lit> ps1, ps2;
                ps1.push(arg1.r_eq(solver, 0));
                ps1.push(arg2.e_neq(solver, 0));
                ps2.push(arg1.r_neq(solver, 0));
                ps2.push(arg2.e_eq(solver, 0));
                solver.addClause(ps1);
                solver.addClause(ps2);
            } else {
                vec<Lit> ps;
                rv = solver.newCSPVar(0, 1);
                cspvar arg1 = postExpression(fn->args[0]),
                        arg2 = postExpression(fn->args[1]);
                vec<Lit> ps1, ps2, ps3, ps4;
                ps1.push(rv.r_neq(solver, 0));
                ps1.push(arg1.r_eq(solver, 0));
                ps1.push(arg2.e_neq(solver, 0));
                ps2.push(rv.r_neq(solver, 0));
                ps2.push(arg1.r_neq(solver, 0));
                ps2.push(arg2.e_eq(solver, 0));

                ps3.push(rv.r_eq(solver, 0));
                ps3.push(arg1.r_eq(solver, 0));
                ps3.push(arg2.e_eq(solver, 0));
                ps4.push(rv.r_eq(solver, 0));
                ps4.push(arg1.r_neq(solver, 0));
                ps4.push(arg2.e_neq(solver, 0));
                solver.addClause(ps1);
                solver.addClause(ps2);
                solver.addClause(ps3);
                solver.addClause(ps4);
            }
        }

        // function stuff
        if(fn->type == NT_NEG) {
            assert(!root);
            cspvar arg = postExpression(fn->args[0]);
            rv = solver.newCSPVar(-arg.max(solver), -arg.min(solver));
            post_neg(solver, arg, rv, 0);
        }

        if(fn->type == NT_ABS) {
            assert(!root);
            cspvar arg = postExpression(fn->args[0]);
            rv = solver.newCSPVar(0, max(abs(arg.min(solver)), abs(arg.max(solver))));
            post_abs(solver, arg, rv, 0);
        }

        if(fn->type == NT_SUB) {
            assert(!root);
            cspvar arg1 = postExpression(fn->args[0]);
            cspvar arg2 = postExpression(fn->args[1]);
            int min = arg1.min(solver) - arg2.max(solver);
            int max = arg1.max(solver) - arg2.min(solver);
            rv = solver.newCSPVar(min, max);
            vector<int> w(3);
            vector<cspvar> v(3);
            w[0] = 1;
            v[0] = rv;
            w[1] = -1;
            v[1] = arg1;
            w[2] = 1;
            v[2] = arg2;
            post_lin_eq(solver, v, w, 0);
        }

        if(fn->type == NT_ADD) {
            assert(!root);
            int min = 0, max = 0;
            vector<int> w;
            vector<cspvar> v;
            for(int q = 0 ; q != fn->args.size() ; ++q) {
                cspvar arg = postExpression(fn->args[q]);
                w.push_back(-1);
                v.push_back(arg);
                min += arg.min(solver);
                max += arg.max(solver);
            }
            rv = solver.newCSPVar(min, max);
            v.push_back(rv);
            w.push_back(1);
            post_lin_eq(solver, v, w, 0);
        }
        if(fn->type == NT_MULT) {
            assert(!root);
            cspvar arg0 = postExpression(fn->args[0]);
            for(int q = 1 ; q != fn->args.size() ; ++q) {
                cspvar arg1 = postExpression(fn->args[q]);
                int minv = min(min(arg0.min(solver) * arg1.min(solver),
                                   arg0.min(solver) * arg1.max(solver)),
                               min(arg0.max(solver) * arg1.min(solver),
                                   arg0.max(solver) * arg1.max(solver))),
                        maxv = max(max(arg0.min(solver) * arg1.min(solver),
                                       arg0.min(solver) * arg1.max(solver)),
                                   max(arg0.max(solver) * arg1.min(solver),
                                       arg0.max(solver) * arg1.max(solver)));
                cspvar res = solver.newCSPVar(minv, maxv);
                post_mult(solver, res, arg0, arg1);
                arg0 = res;
            }
            rv = arg0;
        }

        if(fn->type == NT_MIN) {
            assert(!root);
            cspvar arg0 = postExpression(fn->args[0]);
            for(int q = 1 ; q != fn->args.size() ; ++q) {
                cspvar arg1 = postExpression(fn->args[q]);
                int minv = min(arg0.min(solver), arg1.min(solver)),
                        maxv = min(arg0.max(solver), arg1.max(solver));
                cspvar res = solver.newCSPVar(minv, maxv);
                post_min(solver, res, arg0, arg1);
                arg0 = res;
            }
            rv = arg0;
        }
        if(fn->type == NT_MAX) {
            assert(!root);
            cspvar arg0 = postExpression(fn->args[0]);
            for(int q = 1 ; q != fn->args.size() ; ++q) {
                cspvar arg1 = postExpression(fn->args[q]);
                int minv = max(arg0.min(solver), arg1.min(solver)),
                        maxv = max(arg0.max(solver), arg1.max(solver));
                cspvar res = solver.newCSPVar(minv, maxv);
                post_max(solver, res, arg0, arg1);
                arg0 = res;
            }
            rv = arg0;
        }

        if(fn->type == NT_IF) {
            assert(!root);
            assert(fn->args.size() == 3);
            cspvar argif = postExpression(fn->args[0]);
            cspvar arg1 = postExpression(fn->args[1]);
            cspvar arg2 = postExpression(fn->args[2]);
            int minv = min(arg1.min(solver), arg2.min(solver)),
                    maxv = max(arg1.max(solver), arg2.max(solver));
            rv = solver.newCSPVar(minv, maxv);
            post_eq_re(solver, rv, arg1, 0, argif.e_eq(solver, 1));
            post_eq_re(solver, rv, arg2, 0, argif.e_neq(solver, 1));
        }

        return rv;
    }


}
#endif //COSOCO_XCSP3MiniCSPCallbacks_H
