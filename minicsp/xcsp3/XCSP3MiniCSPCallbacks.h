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
        map <string, cspvar> vars;
        vector <vector<int>> *previousTuples;
    public:
        XCSP3MiniCSPCallbacks(Solver &s);

        void print_solution();

        vector<cspvar> xvars2cspvars(vector<XVariable *> &xvars);


        virtual void buildVariableInteger(string id, int minValue, int maxValue) override;

        virtual void buildVariableInteger(string id, vector<int> &values) override;

        virtual void buildConstraintExtension(string id, vector<XVariable *> list, vector <vector<int>> &tuples, bool support, bool hasStar) override;

        virtual void buildConstraintExtension(string id, XVariable *variable, vector<int> &tuples, bool support, bool hasStar) override;

        virtual void buildConstraintExtensionAs(string id, vector<XVariable *> list, bool support, bool hasStar) override;

        virtual void buildConstraintIntension(string id, string expr) override;

        virtual void buildConstraintPrimitive(string id, OrderType op, XVariable *x, int k, XVariable *y) override;


        virtual void
        buildConstraintRegular(string id, vector<XVariable *> &list, string st, string final, vector <XTransition> &transitions) override;

        virtual void buildConstraintMDD(string id, vector<XVariable *> &list, vector <XTransition> &transitions) override;

        virtual void buildConstraintAlldifferent(string id, vector<XVariable *> &list) override;

        virtual void buildConstraintAllEqual(string id, vector<XVariable *> &list) override;

        virtual void buildConstraintNotAllEqual(string id, vector<XVariable *> &list) override;

        virtual void buildConstraintOrdered(string id, vector<XVariable *> &list, OrderType order) override;

        virtual void buildConstraintLex(string id, vector <vector<XVariable *>> &lists, OrderType order) override;

        virtual void buildConstraintLexMatrix(string id, vector <vector<XVariable *>> &matrix, OrderType order) override;

        virtual void buildConstraintSum(string id, vector<XVariable *> &list, vector<int> &coeffs, XCondition &cond) override;

        virtual void buildConstraintSum(string id, vector<XVariable *> &list, XCondition &cond) override;

        void postSum(id, vector<cspvar> &list, vector<int> &coefs, XCondition &xc);

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

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector <XInterval> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<int> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<XVariable *> &occurs,
                                                bool closed) override;

        virtual void buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector <XInterval> &occurs,
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

        virtual void buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector <XInterval> &widths) override;

        virtual void buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector <XInterval> &widths,
                                            vector <vector<int>> &patterns) override;

        virtual void buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<int> &lengths, bool zeroIgnored) override;

        virtual void buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<XVariable *> &lengths, bool zeroIgnored) override;

        virtual void
        buildConstraintNoOverlap(string id, vector <vector<XVariable *>> &origins, vector <vector<int>> &lengths, bool zeroIgnored) override;

        virtual void
        buildConstraintNoOverlap(string id, vector <vector<XVariable *>> &origins, vector <vector<XVariable *>> &lengths, bool zeroIgnored) override;

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

using namespace XCSP3Core;


XCSP3MiniCSPCallbacks::XCSP3MiniCSPCallbacks(Solver &s) : XCSP3CoreCallbacks(), solver(s) { }


vector <cspvar> XCSP3MiniCSPCallbacks::xvars2cspvars(vector < XVariable * > &xvars) {
    vector <cspvar> v(xvars.size());
    for(int i = 0 ; i != xvars.size() ; ++i) {
        v[i] = vars[xvars[i]->id];
    }
    return v;
}


void XCSP3MiniCSPCallbacks::print_solution() {
    //TODO
    /*map<string, cspvar>::const_iterator i, b = vars.begin(), e = vars.end();
    for(i = b; i != e; ++i) {
        if( i != b ) cout << ", ";
        cout << cspvar_printer(_solver, i->second) << "=" << _solver.cspModelValue(i->second);
    }
    cout << "\n";
     */
}


void XCSP3MiniCSPCallbacks::buildVariableInteger(string id, int minValue, int maxValue) {
    cspvar x = solver.newCSPVar(minValue, maxValue);
    solver.setCSPVarName(x, id);
    vars[id] = x;
}


void XCSP3MiniCSPCallbacks::buildVariableInteger(string id, vector<int> &values) {
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


void XCSP3MiniCSPCallbacks::buildConstraintExtension(string id, vector<XVariable *> list, vector <vector<int>> &tuples, bool support, bool hasStar) {
    if(hasStar)
        throw unsupported();
    previousTuples = &tuples;
    if(support)
        post_positive_table(solver, xvars2cspvars(list), tuples);
    else
        post_negative_table(solver, xvars2cspvars(list), tuples);


}


void XCSP3MiniCSPCallbacks::buildConstraintExtension(string id, XVariable *variable, vector<int> &tuple, bool support, bool hasStar) {
    vector<XVariable *> list {variable};
    vector <vector<int>> &tuples {tuple};
    buildConstraintExtension(id,list,tuples,support,hasStar);
}


void XCSP3MiniCSPCallbacks::buildConstraintExtensionAs(string id, vector<XVariable *> list, bool support, bool hasStar) {
    buildConstraintExtension(id,list,*previousTuples,support,hasStar);
}


void XCSP3MiniCSPCallbacks::buildConstraintIntension(string id, string expr) {
    cout << "\n    intension constraint : " << id << " : " << expr << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintPrimitive(string id, OrderType op, XVariable *x, int k, XVariable *y) {
    cout << "\n   intension constraint " << id << ": " << x->id << (k >= 0 ? "+" : "") << k << " op " << y->id << endl;
}


void
XCSP3MiniCSPCallbacks::buildConstraintRegular(string id, vector<XVariable *> &list, string start, string final, vector <XTransition> &transitions) {
    cout << "\n    regular constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        start: " << start << endl;
    cout << "        final: " << final << endl;
    cout << "        transitions: ";
    for(unsigned int i = 0 ; i < (transitions.size() > 4 ? 4 : transitions.size()) ; i++) {
        cout << "(" << transitions[i].from << "," << transitions[i].val << "," << transitions[i].to << ") ";
    }
    if(transitions.size() > 4) cout << "...";
    cout << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintMDD(string id, vector<XVariable *> &list, vector <XTransition> &transitions) {
    cout << "\n    mdd constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        transitions: ";
    for(unsigned int i = 0 ; i < (transitions.size() > 4 ? 4 : transitions.size()) ; i++) {
        cout << "(" << transitions[i].from << "," << transitions[i].val << "," << transitions[i].to << ") ";
    }
    if(transitions.size() > 4) cout << "...";
    cout << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintAlldifferent(string id, vector<XVariable *> &list) {
    post_alldiff(solver, xvars2cspvars(list));
}


void XCSP3MiniCSPCallbacks::buildConstraintAllEqual(string id, vector<XVariable *> &list) {
    vector<cspvar> vars = xvars2cspvars(list);
    for (int i = 0 ; i < vars.size() - 1 ; i++)
        post_eq(solver,vars[i],vars[i+1],0);
}


void XCSP3MiniCSPCallbacks::buildConstraintNotAllEqual(string id, vector<XVariable *> &list) {
    cout << "\n    not allEqual constraint" << id << endl;
    cout << "        ";
    displayList(list);
}


void XCSP3MiniCSPCallbacks::buildConstraintOrdered(string id, vector<XVariable *> &list, OrderType order) {
    cout << "\n    ordered constraint" << endl;
    string sep;
    if(order == LT) sep = " < ";
    if(order == LE) sep = " <= ";
    if(order == GT) sep = " > ";
    if(order == GE) sep = " >= ";
    cout << "        ";
    displayList(list, sep);
}


void XCSP3MiniCSPCallbacks::buildConstraintLex(string id, vector <vector<XVariable *>> &lists, OrderType order) {
    cout << "\n    lex constraint   nb lists: " << lists.size() << endl;
    string sep;
    if(order == LT) sep = " < ";
    if(order == LE) sep = " <= ";
    if(order == GT) sep = " > ";
    if(order == GE) sep = " >= ";
    cout << "        operator: " << sep << endl;
    for(unsigned int i = 0 ; i < lists.size() ; i++) {
        cout << "        list " << i << ": ";
        cout << "        ";
        displayList(lists[i], " ");
    }
}


void XCSP3MiniCSPCallbacks::buildConstraintLexMatrix(string id, vector <vector<XVariable *>> &matrix, OrderType order) {
    cout << "\n    lex matrix constraint   matrix  " << endl;
    string sep;
    if(order == LT) sep = " < ";
    if(order == LE) sep = " <= ";
    if(order == GT) sep = " > ";
    if(order == GE) sep = " >= ";

    for(unsigned int i = 0 ; i < (matrix.size() < 4 ? matrix.size() : 3) ; i++) {
        cout << "        ";
        displayList(matrix[i]);
    }
    cout << "        Order " << sep << endl;
}


void XCSP3MiniCSPCallbacks::postSum(id, vector<cspvar> &list, vector<int> &coefs, XCondition &xc) {
    switch(xc.orderType) {
        case EQ:
            post_lin_eq(solver, list, coefs, xc.val);
            break;
        case NE:
            post_lin_neq(solver, list, coefs, xc.val);
            break;
        case GE:
            for(int i = 0; i != coefs.size(); ++i) coefs[i] = -coefs[i];
            post_lin_leq(solver, list, coefs, -xc.val);
            break;
        case GT:
            for(int i = 0; i != coefs.size(); ++i) coefs[i] = -coefs[i];
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
void XCSP3MiniCSPCallbacks::buildConstraintSum(string id, vector<XVariable *> &list, vector<int> &coeffs, XCondition &xc) {

    int c = 0;
    vector<cspvar> vars = xvars2cspvars(list);

    if(xc.operandType == VARIABLE) {
        xc.operandType = INTEGER;
        xc.val = 0;
        vars.push(vars[xc.var->id]);
        coeffs.push(-1);
    }
    if(xc.op != IN) {
        postSum(id,vars,coeffs,xc);
        return;
    }
    // Intervals
    xc.operandType = GE;
    xc.val = xc.min;
    postSum(id,vars,coeffs,xc);
    xc.operandType = LE;
    xc.val = xc.max;
    postSum(id,vars,coeffs,xc);
}


void XCSP3MiniCSPCallbacks::buildConstraintSum(string id, vector<XVariable *> &list, XCondition &cond) {
    vector<int> coeffs;
    coeffs.assign(list.size(), 1);
    buildConstraintSum(id, list, coeffs, xc);
}


void XCSP3MiniCSPCallbacks::buildConstraintAtMost(string id, vector<XVariable *> &list, int value, int k) {
    cout << "\n    AtMost constraint: val=" << value << " k=" << k << endl;
    cout << "        ";
    displayList(list);
}


void XCSP3MiniCSPCallbacks::buildConstraintAtLeast(string id, vector<XVariable *> &list, int value, int k) {
    cout << "\n    Atleast constraint: val=" << value << " k=" << k << endl;
    cout << "        ";
    displayList(list);
}


void XCSP3MiniCSPCallbacks::buildConstraintExactlyK(string id, vector<XVariable *> &list, int value, int k) {
    cout << "\n    Exactly constraint: val=" << value << " k=" << k << endl;
    cout << "        ";
    displayList(list);
}


void XCSP3MiniCSPCallbacks::buildConstraintAmong(string id, vector<XVariable *> &list, vector<int> &values, int k) {
    cout << "\n    Among constraint: k=" << k << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);

}


void XCSP3MiniCSPCallbacks::buildConstraintExactlyVariable(string id, vector<XVariable *> &list, int value, XVariable *x) {
    cout << "\n    Exactly Variable constraint: val=" << value << " variable=" << *x << endl;
    cout << "        ";
    displayList(list);
}


void XCSP3MiniCSPCallbacks::buildConstraintCount(string id, vector<XVariable *> &list, vector<int> &values, XCondition &xc) {
    cout << "\n    count constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        values: ";
    cout << "        ";
    displayList(values);
    cout << "        condition: " << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintCount(string id, vector<XVariable *> &list, vector<XVariable *> &values, XCondition &xc) {
    cout << "\n    count constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        values: ";
    displayList(values);
    cout << "        condition: " << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintNValues(string id, vector<XVariable *> &list, vector<int> &except, XCondition &xc) {
    cout << "\n    NValues with exceptions constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        exceptions: ";
    displayList(except);
    cout << "        condition:" << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintNValues(string id, vector<XVariable *> &list, XCondition &xc) {
    cout << "\n    NValues  constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        condition:" << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector<int> &occurs, bool closed) {
    cout << "\n    Cardinality constraint (int values, int occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector<XVariable *> &occurs,
                                                       bool closed) {
    cout << "\n    Cardinality constraint (int values, var occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<int> values, vector <XInterval> &occurs,
                                                       bool closed) {
    cout << "\n    Cardinality constraint (int values, interval occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<int> &occurs,
                                                       bool closed) {
    cout << "\n    Cardinality constraint (var values, int occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector<XVariable *> &occurs,
                                                       bool closed) {
    cout << "\n    Cardinality constraint (var values, var occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintCardinality(string id, vector<XVariable *> &list, vector<XVariable *> values, vector <XInterval> &occurs,
                                                       bool closed) {
    cout << "\n    Cardinality constraint (var values, interval occurs)  constraint closed: " << closed << endl;
    cout << "        ";
    displayList(list);
    cout << "        values:";
    displayList(values);
    cout << "        occurs:";
    displayList(occurs);
}


void XCSP3MiniCSPCallbacks::buildConstraintMinimum(string id, vector<XVariable *> &list, XCondition &xc) {
    cout << "\n    minimum  constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        condition: " << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintMinimum(string id, vector<XVariable *> &list, XVariable *index, int startIndex, RankType rank,
                                                   XCondition &xc) {
    cout << "\n    arg_minimum  constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        index:" << *index << endl;
    cout << "        Start index : " << startIndex << endl;
    cout << "        condition: " << xc << endl;

}


void XCSP3MiniCSPCallbacks::buildConstraintMaximum(string id, vector<XVariable *> &list, XCondition &xc) {
    cout << "\n    maximum  constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        condition: " << xc << endl;
}


void
XCSP3MiniCSPCallbacks::buildConstraintMaximum(string id, vector<XVariable *> &list, XVariable *index, int startIndex, RankType rank, XCondition &xc) {
    cout << "\n    arg_maximum  constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        index:" << *index << endl;
    cout << "        Start index : " << startIndex << endl;
    cout << "        condition: " << xc << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintElement(string id, vector<XVariable *> &list, int value) {
    cout << "\n    element constant constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        value: " << value << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintElement(string id, vector<XVariable *> &list, XVariable *value) {
    cout << "\n    element variable constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        value: " << *value << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank, int value) {
    cout << "\n    element constant (with index) constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        value: " << value << endl;
    cout << "        Start index : " << startIndex << endl;
    cout << "        index : " << *index << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintElement(string id, vector<XVariable *> &list, int startIndex, XVariable *index, RankType rank,
                                                   XVariable *value) {
    cout << "\n    element variable (with index) constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        value: " << *value << endl;
    cout << "        Start index : " << startIndex << endl;
    cout << "        index : " << *index << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintChannel(string id, vector<XVariable *> &list, int startIndex) {
    cout << "\n    channel constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        Start index : " << startIndex << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintChannel(string id, vector<XVariable *> &list1, int startIndex1, vector<XVariable *> &list2,
                                                   int startIndex2) {
    cout << "\n    channel constraint" << endl;
    cout << "        list1 ";
    displayList(list1);
    cout << "        list2 ";
    displayList(list2);
}


void XCSP3MiniCSPCallbacks::buildConstraintChannel(string id, vector<XVariable *> &list, int startIndex, XVariable *value) {
    cout << "\n    channel constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        value: " << *value << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector <XInterval> &widths) {
    cout << "\n    stretch constraint" << endl;
    cout << "        ";
    displayList(list);
    cout << "        values :";
    displayList(values);
    cout << "        widths:";
    displayList(widths);
}


void XCSP3MiniCSPCallbacks::buildConstraintStretch(string id, vector<XVariable *> &list, vector<int> &values, vector <XInterval> &widths,
                                                   vector <vector<int>> &patterns) {
    cout << "\n    stretch constraint (with patterns)" << endl;
    cout << "        ";
    displayList(list);
    cout << "        values :";
    displayList(values);
    cout << "        widths:";
    displayList(widths);
    cout << "        patterns";
    for(unsigned int i = 0 ; i < patterns.size() ; i++)
        cout << "(" << patterns[i][0] << "," << patterns[i][1] << ") ";
    cout << endl;
}


void XCSP3MiniCSPCallbacks::buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<int> &lengths, bool zeroIgnored) {
    cout << "\n    nooverlap constraint" << endl;
    cout << "        origins";
    displayList(origins);
    cout << "        lengths";
    displayList(lengths);
}


void XCSP3MiniCSPCallbacks::buildConstraintNoOverlap(string id, vector<XVariable *> &origins, vector<XVariable *> &lengths, bool zeroIgnored) {
    cout << "\n    nooverlap constraint" << endl;
    cout << "        origins:";
    displayList(origins);
    cout << "        lengths";
    displayList(lengths);
}


void
XCSP3MiniCSPCallbacks::buildConstraintNoOverlap(string id, vector <vector<XVariable *>> &origins, vector <vector<int>> &lengths, bool zeroIgnored) {
    cout << "\n    kdim (int lengths) nooverlap constraint" << endl;
    cout << "origins: " << endl;
    for(unsigned int i = 0 ; i < origins.size() ; i++) {
        cout << "        ";
        displayList(origins[i]);
    }
    cout << "lengths: " << endl;
    for(unsigned int i = 0 ; i < origins.size() ; i++) {
        cout << "        ";
        displayList(lengths[i]);
    }

}


void XCSP3MiniCSPCallbacks::buildConstraintNoOverlap(string id, vector <vector<XVariable *>> &origins, vector <vector<XVariable *>> &lengths,
                                                     bool zeroIgnored) {
    cout << "\n    kdim (lenghts vars nooverlap constraint" << endl;
    cout << "origins: " << endl;
    for(unsigned int i = 0 ; i < origins.size() ; i++) {
        cout << "        ";
        displayList(origins[i]);
    }
    cout << "lengths: " << endl;
    for(unsigned int i = 0 ; i < origins.size() ; i++) {
        cout << "        ";
        displayList(lengths[i]);
    }
}


void XCSP3MiniCSPCallbacks::buildConstraintInstantiation(string id, vector<XVariable *> &list, vector<int> &values) {
    cout << "\n    instantiation constraint" << endl;
    cout << "        list:";
    displayList(list);
    cout << "        values:";
    displayList(values);

}


void XCSP3MiniCSPCallbacks::buildObjectiveMinimizeExpression(string expr) {
    cout << "\n    objective: minimize" << expr << endl;
}


void XCSP3MiniCSPCallbacks::buildObjectiveMaximizeExpression(string expr) {
    cout << "\n    objective: maximize" << expr << endl;
}


void XCSP3MiniCSPCallbacks::buildObjectiveMinimizeVariable(XVariable *x) {
    cout << "\n    objective: minimize variable " << x << endl;
}


void XCSP3MiniCSPCallbacks::buildObjectiveMaximizeVariable(XVariable *x) {
    cout << "\n    objective: maximize variable " << x << endl;
}


void XCSP3MiniCSPCallbacks::buildObjectiveMinimize(ExpressionObjective type, vector<XVariable *> &list, vector<int> &coefs) {
    XCSP3CoreCallbacks::buildObjectiveMinimize(type, list, coefs);
}


void XCSP3MiniCSPCallbacks::buildObjectiveMaximize(ExpressionObjective type, vector<XVariable *> &list, vector<int> &coefs) {
    XCSP3CoreCallbacks::buildObjectiveMaximize(type, list, coefs);
}


void XCSP3MiniCSPCallbacks::buildObjectiveMinimize(ExpressionObjective type, vector<XVariable *> &list) {
    XCSP3CoreCallbacks::buildObjectiveMinimize(type, list);
}


void XCSP3MiniCSPCallbacks::buildObjectiveMaximize(ExpressionObjective type, vector<XVariable *> &list) {
    XCSP3CoreCallbacks::buildObjectiveMaximize(type, list);
}


#endif //COSOCO_XCSP3MiniCSPCallbacks_H
