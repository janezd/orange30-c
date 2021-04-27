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


#ifndef _ASSOCRULESINDUCER_SPARSE_HPP
#define _ASSOCRULESINDUCER_SPARSE_HPP

#include "associationrule.hpp"
#include "assocrulesinducer-sparse.ppp"

class TSparseExamples;
class TSparseItemsetNode;

class TSparseItemsetTree : public TOrange {							//item node used in TSparseItemsetTree
public:
    __REGISTER_CLASS(SparseItemsetTree);

    PDomain domain; //PR

    TSparseItemsetTree(const TSparseExamples &examples);			//constructor
    ~TSparseItemsetTree();

    int buildLevelOne(vector<long> intDomain);
    long extendNextLevel(int maxDepth, long maxCount);
    bool allowExtend(long itemset[], int iLength);
    long countLeafNodes();
    void considerItemset(long itemset[], int iLength, double weight, int aimLength);
    void considerExamples(TSparseExamples *examples, int aimLength);
    void assignExamples(TSparseItemsetNode *node, long *itemset, long *itemsetend, const int exampleId);
    void assignExamples(TSparseExamples &examples);
    void delLeafSmall(double minSupport);
    PAssociationRules genRules(int maxDepth, double minConf, double nOfExamples, bool storeExamples);
    long getItemsetRules(long itemset[], int iLength, double minConf,
        double nAppliesBoth, double nOfExamples, PAssociationRules rules, bool storeExamples, TSparseItemsetNode *bothNode);

    //private:
    TSparseItemsetNode *root;
};


class TAssociationRulesSparseInducer : public TOrange {
public:
    __REGISTER_CLASS(AssociationRulesSparseInducer);
    NEW_WITH_CALL(AssociationRulesSparseInducer);

    int maxItemSets; //P maximal number of itemsets (increase if you want)

    double confidence; //P required confidence
    double support; //P required support

    int storeExamples; //P(&ExampleTable.Store) 

    TAssociationRulesSparseInducer(double asupp=0.3, double aconf=0, int awei=0);
    TSparseItemsetTree *buildTree(
        PExampleTable const &examples,
        long &i, double &fullWeight);
    PAssociationRules operator()(PExampleTable const &);

    void gatherRules(TSparseItemsetNode *, vector<int> &itemsSoFar,
        PyObject *listOfItems);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data); generate rules from examples");
    PyObject *getItemsets(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(data) -> list of itemsets");

private:
    double nOfExamples;
};


class TItemsetsSparseInducer : public TOrange {
public:
    __REGISTER_CLASS(ItemsetsSparseInducer);
    NEW_WITH_CALL(ItemsetsSparseInducer);

    int maxItemSets; //P maximal number of itemsets (increase if you want)
    double support; //P required support

    int storeExamples; //P(&ExampleTable.Store) 

    TItemsetsSparseInducer(double asupp=0.3, int awei=0);
    PSparseItemsetTree operator()(PExampleTable const &);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data); generate itemsets from examples");
private:
    double nOfExamples;
};


class TItemsetNodeProxy : public TOrange {
public:
    __REGISTER_CLASS(ItemsetNodeProxy);

    TSparseItemsetNode const * const node;
    PSparseItemsetTree tree; //PR tree that this node belongs to

    TItemsetNodeProxy(TSparseItemsetNode const *const, PSparseItemsetTree const &);

    static PyObject *__get__children(OrItemsetNodeProxy *self);
    static PyObject *__get__examples(OrItemsetNodeProxy *self);
    static PyObject *__get__support(OrItemsetNodeProxy *self);
    static PyObject *__get__itemId(OrItemsetNodeProxy *self);
};

#endif
