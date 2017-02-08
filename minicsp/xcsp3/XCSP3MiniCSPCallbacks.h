/*=============================================================================
 * parser for CSP instances represented in XCSP3 Format
 *
 * Copyright (c) 2015 xcp3.org (contact <at> xcsp3.org)
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
#ifndef _MINICSP_XCSP3_CALLBAKCS_H
#define _MINICSP_XCSP3_CALLBAKCS_H

#include <map>
#include "XCSP3CoreCallbacks.h"
#include "XCSP3Variable.h"
#include "minicsp/core/cons.hpp"



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
        Solver &solver;
        map<string, cspvar> tocspvars;
        map<int, cspvar> constants;

        vector<vector<int>> *previousTuples;

    public:
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


        // ---------------------------- VARIABLES ------------------------------------------

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


        void buildVariableInteger(string id, int minValue, int maxValue) override {
            cspvar x = solver.newCSPVar(minValue, maxValue);
            solver.setCSPVarName(x, id);
            tocspvars[id] = x;
        }


        void buildVariableInteger(string id, vector<int> &values) override {
            cspvar x = solver.newCSPVar(values[0], values.back());
            int v = values[0];
            for(int i = 1 ; i < values.back() ; i++) {
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

//        void buildConstraintIntension(string id, string expr) override;

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
                case INTEGER : n = constant(xc.val); break;
                case VARIABLE : n = tocspvars[xc.var];break;
                default : throw unsupported("nValues with interval is not yet supported");
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


}
#endif //COSOCO_XCSP3MiniCSPCallbacks_H
