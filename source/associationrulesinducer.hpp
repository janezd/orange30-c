/*
    This file is part of Orange.
    	
    Copyright 1996-2010 Faculty of Computer and Information Science, University of Ljubljana
    Contact: janez.demsar@fri.uni-lj.si

    Orange is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Orange is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Orange.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _ASSOCIATIONRULESINDUCER_HPP
#define _ASSOCIATIONRULESINDUCER_HPP

#include "associationrule.hpp"
#include "associationrulesinducer.ppp"

class TExWei;
/* This is a set of examples used to list the examples that support a particular tree node */
typedef vector<TExWei> TExampleSet;

class TItemSetNode;
class TRuleTreeNode;


class TAssociationRulesInducer : public TOrange {
public:
    __REGISTER_CLASS(AssociationRulesInducer);
    NEW_WITH_CALL(AssociationRulesInducer);

    int maxItemSets; //P maximal number of itemsets (increase if you want)

    double confidence; //P required confidence
    double support; //P required support
    bool classificationRules; //P if true, rules will have the class and only the class attribute on the right-hand side
    int storeExamples; //P(&ExampleTable.Store) if true, each rule is going to have tables with references to examples which match its left side or both sides

public:

    TAssociationRulesInducer(double const asupp=0.3, double const aconf=0.5);
    PAssociationRules operator()(PExampleTable const &);

    void buildTrees(PExampleTable const &, TItemSetNode *&,
        int &depth, int &nOfExamples, TDiscDistribution &);

    int  buildTree1(PExampleTable const &, TItemSetNode *&,
        double &suppN, int &nOfExamples, TDiscDistribution &);

    int  buildNext1(TItemSetNode *, int k, const double suppN);
    int  makePairs (TItemSetNode *, const double suppN);

    PAssociationRules generateClassificationRules(PDomain const &,
        TItemSetNode *tree, const int nOfExamples, const TDiscDistribution &);

    void generateClassificationRules1(PDomain const &,
        TItemSetNode *root, TItemSetNode *node,
        TExample &left, const int nLeft, const double nAppliesLeft,
        PAssociationRules &, const int nOfExamples, const TDiscDistribution &,
        TExampleSet *leftSet);

    PAssociationRules generateRules(PDomain const &,
        TItemSetNode *, const int depth, const int nOfExamples);

    void generateRules1(TExample &, TItemSetNode *root, TItemSetNode *node,
        int k, int oldk, PAssociationRules const &, const int nOfExamples);

    void find1Rules(TExample &, TItemSetNode *, const double &support,
        int oldk, PAssociationRules, const int nOfExamples,
        const TExampleSet &bothSets);

    TRuleTreeNode *buildTree1FromExample(TExample const &, TItemSetNode *);

    int generateNext1(TRuleTreeNode *ruleTree, TRuleTreeNode *node,
        TItemSetNode *itemsetsTree, TExample &right, int k, TExample &whole,
        double const support, PAssociationRules const &, const int nOfExamples,
        const TExampleSet &bothSets);

    int generatePairs(TRuleTreeNode *ruleTree, TRuleTreeNode *node,
        TItemSetNode *itemsetsTree, TExample &right, TExample &whole,
        const double support, PAssociationRules const &, const int nOfExamples,
        const TExampleSet &bothSets);


    void gatherRules(TItemSetNode *, vector<pair<int, int> > &itemsSoFar,
                     PyObject *listOfItems);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data); generate rules from examples");
    PyObject *getItemsets(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(data) -> list of itemsets");
};


#endif
