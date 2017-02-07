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
    };

    class XCSP3MiniCSPCallbacks : public XCSP3CoreCallbacks {
        Solver &solver;
        map<string, cspvar> vars;
        vector<vector<int>> *previousTuples;
    public:
        XCSP3MiniCSPCallbacks(Solver &s) : XCSP3CoreCallbacks(), solver(s) {}


        void print_solution();


        vector<cspvar> xvars2cspvars(vector<XVariable *> &xvars) {
            vector<cspvar> v(xvars.size());
            for(int i = 0 ; i != xvars.size() ; ++i) {
                v[i] = vars[xvars[i]->id];
            }
            return v;
        }

        // ---------------------------- VARIABLES ------------------------------------------

        vector<cspvar> xvars2cspvars(vector<XVariable *> &xvars) {
            vector<cspvar> v(xvars.size());
            for(int i = 0 ; i != xvars.size() ; ++i) {
                v[i] = vars[xvars[i]->id];
            }
            return v;
        }


        virtual void buildVariableInteger(string id, int minValue, int maxValue) override {
            cspvar x = solver.newCSPVar(minValue, maxValue);
            solver.setCSPVarName(x, id);
            vars[id] = x;
        }


        virtual void buildVariableInteger(string id, vector<int> &values) override {
            cspvar x = solver.newCSPVar(values[0], values.back());
            int v = values[0];
            for(int i = 1 ; i < values.back() ; i++) {
                v++;
                while(v != values[i])
                    x.remove(solver, v++, NO_REASON);
            }
            solver.setCSPVarName(x, id);
            vars[id] = x;
        }

        // ---------------------------- EXTENSION ------------------------------------------

        virtual void buildConstraintExtension(string id, vector<XVariable *> list, vector<vector<int>> &tuples, bool support, bool hasStar) override {
            if(hasStar)
                throw unsupported();
            previousTuples = &tuples;
            if(support)
                post_positive_table(solver, xvars2cspvars(list), tuples);
            else
                post_negative_table(solver, xvars2cspvars(list), tuples);
        }


        virtual void buildConstraintExtension(string id, XVariable *variable, vector<int> &tuple, bool support, bool hasStar) override {
            vector<XVariable *> list{variable};
            vector<vector<int>> &tuples{tuple};
            buildConstraintExtension(id, list, tuples, support, hasStar);
        }


        virtual void buildConstraintExtensionAs(string id, vector<XVariable *> list, bool support, bool hasStar) override {
            buildConstraintExtension(id, list, *previousTuples, support, hasStar);
        }


        // ---------------------------- INTENSION + PRIMITIVES ------------------------------------------

        virtual void buildConstraintIntension(string id, string expr) override;

        virtual void buildConstraintPrimitive(string id, OrderType op, XVariable *x, int k, XVariable *y) override;


        // ---------------------------- LANGUAGES ------------------------------------------

        virtual void
        buildConstraintRegular(string id, vector<XVariable *> &list, string st, string final, vector<XTransition> &transitions) override;

        virtual void buildConstraintMDD(string id, vector<XVariable *> &list, vector<XTransition> &transitions) override;


        // ---------------------------- ALLDIFF ALLEQUAL ------------------------------------------

        virtual void buildConstraintAlldifferent(string id, vector<XVariable *> &list) override {
            post_alldiff(solver, xvars2cspvars(list));
        }


        virtual void buildConstraintAllEqual(string id, vector<XVariable *> &list) override {
            vector<cspvar> vars = xvars2cspvars(list);
            for(int i = 0 ; i < vars.size() - 1 ; i++)
                post_eq(solver, vars[i], vars[i + 1], 0);
        }


        virtual void buildConstraintNotAllEqual(string id, vector<XVariable *> &list) override;

        virtual void buildConstraintOrdered(string id, vector<XVariable *> &list, OrderType order) override;

        virtual void buildConstraintLex(string id, vector<vector<XVariable *>> &lists, OrderType order) override;

        virtual void buildConstraintLexMatrix(string id, vector<vector<XVariable *>> &matrix, OrderType order) override;


        // ---------------------------- SUM ------------------------------------------

        void postSum(string id, vector<cspvar> &list, vector<int> &coefs, XCondition &xc) {
            switch(xc.op) {
                case EQ:
                    post_lin_eq(solver, list, coefs, xc.val);
                    break;
                case NE:
                    post_lin_neq(solver, list, coefs, xc.val);
                    break;
                case GE:
                    for(int i = 0 ; i != coefs.size() ; ++i) coefs[i] = -coefs[i];
                    post_lin_leq(solver, list, coefs, -xc.val);
                    break;
                case GT:
                    for(int i = 0 ; i != coefs.size() ; ++i) coefs[i] = -coefs[i];
                    post_lin_less(solver, list, coefs, -xc.val);
                    break;
                case LE:
                    post_lin_leq(solver, list, coefs, xc.val);
                    break;
                case LT:
                    post_lin_less(solver, list, coefs, xc.val);
                    break;
                default:
                    throw unsupported();
            }
        }


        virtual void buildConstraintSum(string id, vector<XVariable *> &list, vector<int> &coeffs, XCondition &xc) override {
            vector<cspvar> variables = xvars2cspvars(list);
            if(xc.operandType == VARIABLE) {
                xc.operandType = INTEGER;
                xc.val = 0;
                variables.push_back(vars[xc.var]);
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


        virtual void XCSP3MiniCSPCallbacks::buildConstraintSum(string id, vector<XVariable *> &list, XCondition &xc) override {
            vector<int> coeffs;
            coeffs.assign(list.size(), 1);
            buildConstraintSum(id, list, coeffs, xc);
        }


        virtual void buildConstraintAtMost(string id, vector<XVariable *> &list, int value, int k) override;

        virtual void buildConstraintAtLeast(string id, vector<XVariable *> &list, int value, int k) override;

        virtual void buildConstraintExactlyK(string id, vector<XVariable *> &list, int value, int k) override;

        virtual void buildConstraintAmong(string id, vector<XVariable *> &list, vector<int> &values, int k) override;

        virtual void buildConstraintExactlyVariable(string id, vector<XVariable *> &list, int value, XVariable *x) override;

        virtual void buildConstraintCount(string id, vector<XVariable *> &list, vector<int> &values, XCondition &xc) override;

        virtual void buildConstraintCount(string id, vector<XVariable *> &list, vector<XVariable *> &values, XCondition &xc) override;

        virtual void buildConstraintNValues(string id, vector<XVariable *> &list, vector<int> &except, XCondition &xc) override;

        virtual void buildConstraintNValues(string id, vector<XVariable *> &list, XCondition &xc) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector<int> &occurs, bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector<XVariable *> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector<XInterval> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<int> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<XVariable *> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<XInterval> &occurs,
                                                bool closed) override;

        virtual void buildConstraintMinimum(string id, vector<XVariable *> &list, XCondition &xc) override;

        virtual void buildConstraintMinimum(string id, vector<XVariable *> &list, XVariable *index, int startIndex, RankType rank,
                                            XCondition &xc) override;

        virtual void buildConstraintMaximum(string id, vector<XVariable *> &list, XCondition &xc) override;

        virtual void buildConstraintMaximum(string id, vector<XVariable *> &list, XVariable *index, int startIndex, RankType rank,
                                            XCondition &xc) override;

        virtual void buildConstraintElement(string id, vector<XVariable *> &list, int value) override;

        virtual void buildConstraintElement(string id, vector<XVariable *> &list, XVariable *value) override;

        virtual void
        buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank, int value) override;

        virtual void
        buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank, XVariable *value) override;

        virtual void buildConstraintChannel(string id, vector<XVariable *> &list, int startIndex) override;

        virtual void buildConstraintChannel(string id, vector<XVariable *> &list1, int startIndex1, vector<XVariable *> &list2,
                                            int startIndex2) override;

        virtual void buildConstraintChannel(string id, vector<XVariable *> &list, int startIndex, XVariable *value) override;

        virtual void buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector<XInterval> &widths) override;

        virtual void buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector<XInterval> &widths,
                                            vector<vector<int>> &patterns) override;

        virtual void buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<int> &lengths, bool zeroIgnored) override;

        virtual void buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<XVariable *> &lengths, bool zeroIgnored) override;

        virtual void
        buildConstraintNoOverlap(string id, vector<vector<XVariable *>> &origins, vector<vector<int>> &lengths, bool zeroIgnored) override;

        virtual void
        buildConstraintNoOverlap(string id, vector<vector<XVariable *>> &origins, vector<vector<XVariable *>> &lengths, bool zeroIgnored) override;

        virtual void buildConstraintInstantiation(string id, vector<XVariable *> &list, vector<int> &values) override;

        virtual void buildObjectiveMinimizeExpression(string expr) override;

        virtual void buildObjectiveMaximizeExpression(string expr) override;


        virtual void buildObjectiveMinimizeVariable(XVariable *x) override;


        virtual void buildObjectiveMaximizeVariable(XVariable *x) override;


        virtual void buildObjectiveMinimize(ExpressionObjective type, vector<XVariable *> &list, vector<int> &coefs) override;


        virtual void buildObjectiveMaximize(ExpressionObjective type, vector<XVariable *> &list, vector<int> &coefs) override;


        virtual void buildObjectiveMinimize(ExpressionObjective type, vector<XVariable *> &list) override;


        virtual void buildObjectiveMaximize(ExpressionObjective type, vector<XVariable *> &list) override;


    };


}
#endif //COSOCO_XCSP3MiniCSPCallbacks_H
