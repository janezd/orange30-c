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


#include "common.hpp"
#include "associationrulesinducer.px"


class TItemSetNode;

/* These objects are collected in TExampleSets, lists of examples that correspond to a particular tree node.
   'example' is a unique example id (basically its index in the original dataset)
   'weight' is the example's weight. */
class TExWei {
public:
    int example;
    double weight;
    TExWei(const int ex, const double wei)
        : example(ex), weight(wei)
    {}

    TExWei(TExWei const &old)
        : example(old.example), weight(old.weight)
    {}

    TExWei &operator =(TExWei const &old)
    {
        example = old.example;
        weight = old.weight;
        return *this;
    }
};



/* A tree element that corresponds to an attribute value (ie, TItemSetNode has as many
   TlItemSetValues as there are values that appear in itemsets.
   For each value, we have the 'examples' that support it, the sum of their weights
   ('support') and the branch that contains more specialized itemsets. */
class TItemSetValue {
public:
    int value;
    TItemSetNode *branch;

    double support;
    TExampleSet examples;

    // This constructor is called when building the 1-tree
    inline TItemSetValue(int al)
        : value(al),
        branch(NULL),
        support(0.0)
    {}

    // This constructor is called when itemsets are intersected (makePairs ets)
    inline TItemSetValue(int al, const TExampleSet &ex, double asupp)
        : value(al),
        branch(NULL),
        support(asupp),
        examples(ex)
    {}

    inline ~TItemSetValue();

    inline void sumSupport()
    { 
        support = 0; 
        ITERATE(TExampleSet, wi, examples) {
            support += wi->weight;
        }
    }
};


/* TItemSetNode splits itemsets according to the value of attribute 'attrIndex';
each element of 'values' corresponds to an attribute value (not necessarily to all,
but only to those values that appear in itemsets).
Itemsets for which the value is not defined are stored in a subtree in 'nextAttribute'.
This can be seen in TItemSetTree::findSupport that finds a node that corresponds to the
given itemset */
class TItemSetNode {
public:
    int attrIndex;
    TItemSetNode *nextAttribute;
    vector<TItemSetValue> values;

    // This constructor is called by 1-tree builder which initializes all values (and later reduces them)
    inline TItemSetNode(PVariable const &var, int anattri)
    : attrIndex(anattri), 
    nextAttribute(NULL)
    { 
        for(int vi = 0, ve = var->noOfValues(); vi<ve; vi++) {
            values.push_back(TItemSetValue(vi));
        }
    }

    // This constructor is called when extending the tree
    inline TItemSetNode(int anattri)
    : attrIndex(anattri), 
      nextAttribute(NULL) 
    {}

    inline ~TItemSetNode()
    { 
        delete nextAttribute; 
    }
};


// must be defined after TItemsetNode, otherwise delete won't call the destructor
inline TItemSetValue::~TItemSetValue()
{
    delete branch;
}



class TRuleTreeNode {
public:
    int attrIndex;
    int value;
    double support;
    TExampleSet examples;
    TRuleTreeNode *nextAttribute;
    TRuleTreeNode *hasValue;

    inline TRuleTreeNode(int const ai, int const val,
                  double const supp, TExampleSet const &ex)
        : attrIndex(ai),
        value(val),
        support(supp),
        examples(ex),
        nextAttribute(NULL),
        hasValue(NULL)
    {}

    inline ~TRuleTreeNode()
    { 
        delete nextAttribute;
        delete hasValue;
    }
};


void setMatchingExamples(
    PAssociationRule const &rule,
    const TExampleSet &leftSet, const TExampleSet &bothSets)
{
    TIntList *matchLeft = new TIntList();
    rule->matchLeft = PIntList(matchLeft);
    const_ITERATE(TExampleSet, nli, leftSet) {
        matchLeft->push_back((*nli).example);
    }
    TIntList *matchBoth = new TIntList();
    rule->matchBoth = PIntList(matchBoth);
    const_ITERATE(TExampleSet, nri, bothSets) {
        matchBoth->push_back((*nri).example);
    }
}


double computeIntersection(const TExampleSet &set1, const TExampleSet &set2, TExampleSet &intersection)
{
    double isupp = 0.0;
    TExampleSet::const_iterator se1i(set1.begin()), se1e(set1.end());
    TExampleSet::const_iterator se2i(set2.begin()), se2e(set2.end());
    while((se1i != se1e) && (se2i != se2e)) {
        if ((*se1i).example < (*se2i).example) {
            se1i++;
        }
        else if ((*se1i).example > (*se2i).example) {
            se2i++;
        }
        else {
            intersection.push_back(*se1i);
            isupp += (*se1i).weight;
            se1i++;
            se2i++;
        }
    }
    return isupp;
}


/* Searches the tree to find a node that corresponds to itemset 'ex' and returns its support */
double findSupport(
    const TExample &ex,
    TItemSetNode *node,
    TItemSetValue **actualNode = NULL)
{
    // This is initialized just to avoid warnings.
    vector<TItemSetValue>::iterator li = node->values.begin();
    TExample::const_iterator ei(ex.begin()), eei(ex.end());
    int attrIndex = 0;
    for(; ei!=eei; ei++, attrIndex++) {
        // If attribute is in the itemset
        if (!isnan(*ei)) {
            // Search for the attribute in the list linked by 'nextAttribute'
            while (node && (node->attrIndex != attrIndex)) {
                node = node->nextAttribute;
            }
            // this attribute does not appear in any itemset that begins with the
            // attributes that we have already encountered in the example
            if (!node) {
                return 0.0;
            }
            // Search for the value
            for(li = node->values.begin();
                (li != node->values.end()) && (li->value != *ei);
                li++);
            // this attribute value does not appear in any itemset ...
            if (li==node->values.end()) {
                return 0.0;
            }
            // continue if possible
            if (!(*li).branch) {
                break;
            }
            node = (*li).branch;
        }
    }
    // If we are not at the end of example yet, we must make sure no
    // further values appear in the itemset
    if (ei != ex.end()) {
        while((++ei!=ex.end()) && isnan(*ei));
    }
    if (ei == ex.end()) {
        if (actualNode) {
            *actualNode = &*li;
        }
        return (*li).support;
    }
    if (actualNode) {
        *actualNode = NULL;
    }
    return 0;
}


double findSupport(
    const TExample &ex, TRuleTreeNode *node,
    TRuleTreeNode **actualNode = NULL)
{
    TExample::const_iterator ei(ex.begin()), eei(ex.end());
    for(; (ei!=eei) && !isnan(*ei); ei++);
    for(; ei!=eei; ei++, node = node->hasValue) {
        while (node && (node->attrIndex != ei-ex.begin())) {
            node = node->nextAttribute;
        }
        if (!node || (node->value != *ei)) {
            raiseError(PyExc_SystemError,
                "internal error in RuleTree (attribute/value not found)");
        }
        while((++ei!=eei) && !isnan(*ei));
        if (ei==eei) {
            if (actualNode) {
                *actualNode = node;
            }
            return node->support;
        }
    }
    raiseError(PyExc_SystemError,
        "internal error in RuleTree (attribute/value not found)");
    return 0.0; // to make the compiler happy
}


TAssociationRulesInducer::TAssociationRulesInducer(double asupp, double aconf)
: maxItemSets(15000),
  confidence(aconf),
  support(asupp),
  classificationRules(false),
  storeExamples(TExampleTable::DontStore)
{}


PAssociationRules TAssociationRulesInducer::operator()(PExampleTable const &data)
{
    PVariable contvar = data->domain->hasContinuousAttributes();
    if (contvar) {
        raiseError(PyExc_TypeError,
            "cannot induce rules from continuous variables ('%s')",
            contvar->cname());
    }
    TItemSetNode *tree = NULL;
    PAssociationRules rules;
    if (classificationRules && !data->domain->classVar) {
        raiseError(PyExc_ValueError,
            "cannot induce classification rules on classless data");
    }
    try {
        int depth, nOfExamples;
        TDiscDistribution classDist;
        buildTrees(data, tree, depth, nOfExamples, classDist);
        rules = classificationRules ?
            generateClassificationRules(data->domain, tree,
            nOfExamples, classDist)
            : generateRules(data->domain, tree, depth, nOfExamples);
        if (storeExamples != TExampleTable::DontStore) {
            PExampleTable xmpls = (storeExamples == TExampleTable::StoreCopy)
                ? TExampleTable::constructCopy(data)
                : TExampleTable::constructReference(data);
            PITERATE(TAssociationRules, ri, rules) {
                (*ri)->examples = xmpls;
            }
        }
    }
    catch (...) {
        delete tree; 
        throw;
    }
    delete tree;
    return rules;
}

    
    
void TAssociationRulesInducer::buildTrees(PExampleTable const &data,
    TItemSetNode *&tree, int &depth, int &nOfExamples, TDiscDistribution &classDist)
{ 
    double suppN;
    depth = 1;
    for(int totni = 0, ni = buildTree1(data, tree, suppN, nOfExamples, classDist);
            ni;
            ni = buildNext1(tree, ++depth, suppN)) {
        totni += ni;
        if (totni > maxItemSets) {
            raiseError(PyExc_RuntimeError,
                "too many itemsets (%i); increase 'max_item_sets'", totni);
        }
    }
    --depth;
}


// buildTree1: builds the first tree with 1-itemsets
int TAssociationRulesInducer::buildTree1(PExampleTable const &data,
    TItemSetNode *&tree, double &suppN, int &nOfExamples,
    TDiscDistribution &classDist)
{
    tree = NULL;
    if (classificationRules) {
        classDist = TDiscDistribution(data->domain->classVar);
    }
    int index, itemSets = 0;
    TItemSetNode **toChange = &tree;

    // builds an empty tree with all possible 1-itemsets
    TVarList::const_iterator vi(data->domain->variables->begin());
    TVarList::const_iterator const ve(data->domain->variables->end());
    for(index = 0; vi != ve; vi++, index++) {
        *toChange = new TItemSetNode(*vi, index);
        toChange = &((*toChange)->nextAttribute);
    }

    // fills the tree with indices of examples from gen
    index = 0;
    nOfExamples = 0;
    TExampleIterator ei(data->begin());
    for(; ei; ++ei, index++) {
        const double wex = ei.getWeight();
        if (classificationRules) {
            if (isnan(ei.getClass())) {
                continue;
            }
            classDist.add(ei.getClass(), wex);
        }
        nOfExamples += wex;
        TItemSetNode *ip = tree;
        TExample::const_iterator exi(ei->begin());
        TExample::const_iterator const exe(ei->end());
        for(; exi!=exe; exi++, ip = ip->nextAttribute) {
            if (!isnan(*exi)) {
                if (*exi >= ip->values.size()) {
                    raiseError(PyExc_ValueError,
                        "invalid value of attribute '%s'",
                        data->domain->variables->at(exi-ei->begin())->cname());
                }
                ip->values[int(*exi)].examples.push_back(TExWei(index, wex));
            }
        }
    }
    suppN = support * nOfExamples;

    // removes all unsupported itemsets
    itemSets = 0;
    TItemSetNode **ip = &tree;
    while(*ip) {
        // computes sums; li goes through all values, and values that remain go to lue
        vector<TItemSetValue>::iterator lb((*ip)->values.begin()), li(lb), lue(lb);
        vector<TItemSetValue>::iterator const le((*ip)->values.end());
        for(li = lue; li != le; li++) {
            (*li).sumSupport();
            if ((*li).support >= suppN) {
                if (li!=lue) {
                    *lue = *li;
                }
                lue++;
            }
        }
        // no itemsets for this attribute
        if (lue == lb) {
            TItemSetNode *tip = (*ip)->nextAttribute;
            (*ip)->nextAttribute = NULL; // make sure delete doesn't remove the whole chain
            delete *ip; 
            *ip = tip;
        }
        // this attribute has itemset
        // (not necessarily for all values, but 'erase' will cut them
        else {
            (*ip)->values.erase(lue, le);
            itemSets += (*ip)->values.size();
            ip = &((*ip)->nextAttribute);
        }
    }
    return itemSets;
}


/* buildNextTree: uses tree for k-1 itemset, and builds a tree for k-itemset
   by traversing to the depth where k-2 items are defined (buildNext1) and then joining
   all pairs of values descendant to the current position in the tree (makePairs).

   This function descends the tree, recursively calling itself.
   At each level of recursion, an attribute value is (implicitly) added to the itemset
   and k is reduced by one, until it reaches 2.
*/
int TAssociationRulesInducer::buildNext1(
    TItemSetNode *node, int k, double const suppN)
{
    if (k==2) {
        return makePairs(node, suppN);
    }
    // For each value of each attribute...
    int itemSets = 0;
    for(; node; node = node->nextAttribute) {
        ITERATE(vector<TItemSetValue>, li, node->values)  {
            if ((*li).branch) {
                itemSets += buildNext1((*li).branch, k-1, suppN);
            }
        }
    }
    return itemSets;
}


/* This function is called by buildNextTree when it reaches the depth of k-1.
   Nodes at this depth represent k-1-itemsets. Pairs of the remaining attribute
   values are now added if they are supported. 
   
   We only need to check the part of the tree at 'nextAttribute';
   past attributes have been already checked.
   */
int TAssociationRulesInducer::makePairs(TItemSetNode *node, const double suppN)
{
    int itemSets = 0;
    for(TItemSetNode *p1 = node; p1; p1 = p1->nextAttribute) {
        ITERATE(vector<TItemSetValue>, li1, p1->values) {
            TItemSetNode **li1_br = &((*li1).branch);
            for(TItemSetNode *p2 = p1->nextAttribute; p2; p2 = p2->nextAttribute) {
                ITERATE(vector<TItemSetValue>, li2, p2->values) {
                    TExampleSet intersection;
                    double isupp = computeIntersection(
                        (*li1).examples, (*li2).examples, intersection);
                    // support can also be 0, so we have to check intersection size as well
                    if (intersection.size() && (isupp>=suppN)) {
                        if (*li1_br && ((*li1_br)->attrIndex != p2->attrIndex)) {
                            li1_br = &((*li1_br)->nextAttribute);
                        }
                        if (!*li1_br) { // either we need a new attribute or no attributes have been added for p1 so far
                            *li1_br = new TItemSetNode(p2->attrIndex);
                        }
                        (*li1_br)->values.push_back(
                            TItemSetValue((*li2).value, intersection, isupp));
                        itemSets++;
                    }
                }
            }
        }
    }
    return itemSets;
}


PAssociationRules TAssociationRulesInducer::generateClassificationRules(
    PDomain const &dom, TItemSetNode *tree, int const nOfExamples,
    TDiscDistribution const &classDist)
{ 
    PExample left = TExample::constructFree(dom);
    PAssociationRules rules(new TAssociationRules());
    generateClassificationRules1(dom, tree, tree, *left, 0, nOfExamples, rules, nOfExamples, classDist, NULL);
    return rules;
}


void TAssociationRulesInducer::generateClassificationRules1(
    PDomain const &dom, TItemSetNode *root, TItemSetNode *node,
    TExample &left, const int nLeft, const double nAppliesLeft,
    PAssociationRules &rules, const int nOfExamples,
    const TDiscDistribution &classDist, TExampleSet *leftSet)
{ 
    for(; node; node = node->nextAttribute) {
        if (node->nextAttribute) {
            // this isn't the class attributes (since the class attribute is the last one)
            ITERATE(vector<TItemSetValue>, li, node->values)
                if ((*li).branch) {
                    left.setValue(node->attrIndex, (*li).value);
                    generateClassificationRules1(dom, root, (*li).branch,
                        left, nLeft+1, (*li).support, rules, nOfExamples,
                        classDist, &(*li).examples);
                }
                left.setValue(node->attrIndex, undefined_value);
        }
        else {
            // this is the last attribute - but is it the class attribute?
            if (nLeft && (node->attrIndex == dom->attributes->size()))
                ITERATE(vector<TItemSetValue>, li, node->values) {
                    const double &nAppliesBoth = (*li).support;
                    const double aconf =  nAppliesBoth / nAppliesLeft;
                    if (aconf >= confidence) {
                        PExample right = TExample::constructFree(dom);
                        right->setClass(TValue((*li).value));
                        PAssociationRule rule(new TAssociationRule(
                            TExample::constructCopy(left), right,
                            nAppliesLeft, classDist[(*li).value],
                            nAppliesBoth, nOfExamples, nLeft, 1));
                        if (storeExamples != TExampleTable::DontStore) {
                            if (!leftSet) {
                                set<int> allExamplesSet;
                                ITERATE(vector<TItemSetValue>, ri, root->values) {
                                    ITERATE(TExampleSet, ei, (*ri).examples) {
                                        allExamplesSet.insert((*ei).example);
                                    }
                                }
                                TExampleSet allExamples;
                                ITERATE(set<int>, ali, allExamplesSet) {
                                    allExamples.push_back(TExWei(*ali, 1));
                                }
                                setMatchingExamples(
                                    rule, allExamples, (*li).examples);
                            }
                            else {
                                setMatchingExamples(
                                    rule, *leftSet, (*li).examples);
                            }
                        }
                        rules->push_back(rule);
                    }
            }
        }
    }
}


PAssociationRules TAssociationRulesInducer::generateRules(
    PDomain const &dom, TItemSetNode *tree, int const depth, int const nOfExamples)
{ 
    PAssociationRules rules(new TAssociationRules());
    for(int k = 2; k <= depth; k++) {
        PExample example = TExample::constructFree(dom);
        generateRules1(*example, tree, tree, k, k, rules, nOfExamples);
    }
    return rules;
}


void TAssociationRulesInducer::generateRules1(
    TExample &ex, TItemSetNode *root, TItemSetNode *node,
    int k, const int nBoth, PAssociationRules const &rules, const int nOfExamples)
{ 
    /* Descends down the tree, recursively calling itself and adding a value to the
    example (ie itemset) at each call. This goes until k reaches 1. */
    if (k>1) {
        for(; node; node = node->nextAttribute) {
            ITERATE(vector<TItemSetValue>, li, node->values)
                if ((*li).branch) {
                    ex.setValue(node->attrIndex, (*li).value);
                    generateRules1(ex, root, (*li).branch, k-1, nBoth, rules, nOfExamples);
                }
                ex.setValue(node->attrIndex, undefined_value);
        }
    }
    else {
        for(; node; node = node->nextAttribute) {
            ITERATE(vector<TItemSetValue>, li, node->values) {
                ex.setValue(node->attrIndex, (*li).value);

                /* Rule with one item on the right are treated separately.
                Incidentally, these are also the only that are suitable for classification rules */
                find1Rules(ex, root, (*li).support, nBoth, rules, nOfExamples, (*li).examples);

                if (nBoth>2) {
                    TRuleTreeNode *ruleTree = buildTree1FromExample(ex, root);

                    try {
                        PExample example(TExample::constructFree(ex.domain));
                        for(int m = 2;
                            (m <= nBoth-1) && 
                            generateNext1(ruleTree, ruleTree, root, *example, m, ex,
                            (*li).support, rules, nOfExamples, (*li).examples) > 2;
                        m++);
                    }
                    catch (...) {
                        delete ruleTree;
                        throw;
                    }
                    delete ruleTree;
                }
            }
            ex.setValue(node->attrIndex, undefined_value);
        }
    }
}


/* For each value in the itemset, check whether the rule with this value on the right
and all others on the left has enough confidence, and add it if so. */
void TAssociationRulesInducer::find1Rules(TExample &example, TItemSetNode *tree, const double &nAppliesBoth, const int nBoth, PAssociationRules rules, const int nOfExamples, const TExampleSet &bothSets)
{
    PExample left = TExample::constructCopy(example);
    PExample right = TExample::constructFree(example.domain);
    TExample::const_iterator ei(example.begin());
    TExample::const_iterator const ee(example.end());
    TExample::iterator lefti(left->begin()), righti(right->begin());
    for(; ei!=ee; ei++, lefti++, righti++) {
        if (!isnan(*ei)) {
            *lefti = undefined_value;
            *righti = *ei;
            TItemSetValue *nodeLeft;
            const double nAppliesLeft = findSupport(*left, tree, &nodeLeft);
            const double tconf = nAppliesBoth/nAppliesLeft;
            if (tconf >= confidence) {
                const double nAppliesRight = findSupport(*right, tree);
                PAssociationRule rule(new TAssociationRule(
                    TExample::constructCopy(left),
                    TExample::constructCopy(right),
                    nAppliesLeft, nAppliesRight, nAppliesBoth,
                    nOfExamples, nBoth-1, 1));
                if (storeExamples != TExampleTable::DontStore) {
                    setMatchingExamples(rule, nodeLeft->examples, bothSets);
                }
                rules->push_back(rule);
            }
            *righti = undefined_value;
            *lefti = *ei;
        }
    }
}


/* Builds a 1-tree (a list of TRuleTreeNodes linked by nextAttribute) with 1-itemsets
that correspond to values found in example 'ex' */
TRuleTreeNode *TAssociationRulesInducer::buildTree1FromExample(
    TExample const &ex, TItemSetNode *node)
{ 
    TRuleTreeNode *newTree = NULL;
    TRuleTreeNode **toChange = &newTree;
    const_ITERATE(TExample, ei, ex) {
        if  (!isnan(*ei)) {
            while(node && (node->attrIndex != ei-ex.begin())) {
                node = node->nextAttribute;
            }
            _ASSERT(node);

            vector<TItemSetValue>::iterator li(node->values.begin()), le(node->values.end());
            for(; (li != le) && ((*li).value != *ei); li++);
            _ASSERT(li!=le);

            *toChange = new TRuleTreeNode(node->attrIndex, (*li).value, (*li).support, (*li).examples);
            toChange = &((*toChange)->nextAttribute);
        }
    }

    return newTree;
}


/* Extends the tree by one level, recursively calling itself until k is 2.
   (At the beginning, k-1 is the tree depth, so when we have 1-tree, this function is called with k=2.)
   At each recursive call, a value from the example 'wholeEx' is added to 'right'. */
int TAssociationRulesInducer::generateNext1(
    TRuleTreeNode *ruleTree, TRuleTreeNode *node, TItemSetNode *itemsets,
    TExample &right, int k, TExample &wholeEx, double const nAppliesBoth,
    PAssociationRules const &rules, const int nOfExamples,
    const TExampleSet &bothSets)
{
    if (k==2) {
        return generatePairs(ruleTree, node, itemsets, right, wholeEx,
            nAppliesBoth, rules, nOfExamples, bothSets);
    }
    int itemSets = 0;
    for(; node; node = node->nextAttribute) {
        if (node->hasValue) {
            right.setValue(node->attrIndex, node->value);
            itemSets += generateNext1(ruleTree, node->hasValue, itemsets,
                right, k-1, wholeEx, nAppliesBoth, rules, nOfExamples, bothSets);
            right.setValue(node->attrIndex, undefined_value);
        }
    }
    return itemSets;
}


int TAssociationRulesInducer::generatePairs(
    TRuleTreeNode *ruleTree, TRuleTreeNode *node, TItemSetNode *itemsets,
    TExample &right, TExample &wholeEx, double const nAppliesBoth,
    PAssociationRules const &rules, const int nOfExamples,
    const TExampleSet &bothSets)
{
    int itemSets = 0;
    for(TRuleTreeNode *p1 = node; p1; p1 = p1->nextAttribute) {
        right.setValue(p1->attrIndex, p1->value);
        TRuleTreeNode **li1_br = &(p1->hasValue);
        for(TRuleTreeNode *p2 = p1->nextAttribute; p2; p2 = p2->nextAttribute) {
            right.setValue(p2->attrIndex, p2->value);

            PExample left = TExample::constructFree(wholeEx.domain);
            TExample::const_iterator righti(right.begin());
            TExample::const_iterator wi(wholeEx.begin());
            TExample::const_iterator const we(wholeEx.end());
            TExample::iterator lefti(left->begin());
            for(; wi != we; wi++, righti++, lefti++) {
                if (!isnan(*wi) && isnan(*righti)) {
                    *lefti = *wi;
                }
            }

            TItemSetValue *nodeLeft;
            double nAppliesLeft = findSupport(*left, itemsets, &nodeLeft);
            double aconf = nAppliesBoth/nAppliesLeft;
            if (aconf>=confidence) {
                TExampleSet intersection;
                double nAppliesRight = computeIntersection(
                    p1->examples, p2->examples, intersection);
                // Add the item to the tree (confidence can also be 0, so we'd better check the intersection size)
                if (intersection.size()) {
                    *li1_br = new TRuleTreeNode(
                        p2->attrIndex, p2->value, nAppliesRight, intersection);
                    li1_br = &((*li1_br)->nextAttribute);
                    itemSets++;
                }
                PAssociationRule rule(new TAssociationRule(
                    TExample::constructCopy(left), TExample::constructCopy(right),
                    nAppliesLeft, nAppliesRight, nAppliesBoth, nOfExamples));
                if (storeExamples != TExampleTable::DontStore) {
                    setMatchingExamples(rule, nodeLeft->examples, bothSets);
                }
                rules->push_back(rule);
            }
            right.setValue(p2->attrIndex, undefined_value);
        }
        right.setValue(p1->attrIndex, undefined_value);
    }
    return itemSets;
}


PyObject *TAssociationRulesInducer::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable examples;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:AssociationRulesInducer",
        AssociationRulesInducer_call_keywords,
        &PExampleTable::argconverter, &examples)) {
            return NULL;
    }
    return (*this)(examples).getPyObject();
}


void TAssociationRulesInducer::gatherRules(
    TItemSetNode *node,
    vector<pair<int, int> > &itemsSoFar, PyObject *listOfItems)
{
    for(; node; node = node->nextAttribute) {
        itemsSoFar.push_back(make_pair(node->attrIndex, (int)0));
        ITERATE(vector<TItemSetValue>, isi, node->values) {
            itemsSoFar.back().second = (*isi).value;
            PyObject *itemset = PyTuple_New(itemsSoFar.size());
            int el = 0;
            vector<pair<int, int> >::const_iterator sfi(itemsSoFar.begin());
            vector<pair<int, int> >::const_iterator const sfe(itemsSoFar.end());
            for(; sfi != sfe; sfi++, el++) {
                PyObject *vp = PyTuple_New(2);
                PyTuple_SET_ITEM(vp, 0, PyLong_FromLong((*sfi).first));
                PyTuple_SET_ITEM(vp, 1, PyLong_FromLong((*sfi).second));
                PyTuple_SET_ITEM(itemset, el, vp);
            }
            PyObject *examples;
            if (storeExamples != TExampleTable::DontStore) {
                examples = PyList_New((*isi).examples.size());
                Py_ssize_t ele = 0;
                ITERATE(TExampleSet, ei, (*isi).examples) {
                    PyList_SetItem(examples, ele++, PyLong_FromLong(ei->example));
                }
            }
            else {
                examples = Py_None;
                Py_INCREF(Py_None);
            }
            PyObject *rr = PyTuple_New(2);
            PyTuple_SET_ITEM(rr, 0, itemset);
            PyTuple_SET_ITEM(rr, 1, examples);
            PyList_Append(listOfItems, rr);
            Py_DECREF(rr);
            gatherRules((*isi).branch, itemsSoFar, listOfItems);
        }
        itemsSoFar.pop_back();
    }
}


PyObject *TAssociationRulesInducer::getItemsets(PyObject *args, PyObject *kw)
{
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:get_itemsets",
        AssociationRulesInducer_getItemsets_keywords,
        &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    PVariable contvar = data->domain->hasContinuousAttributes();
    if (contvar) {
        return PyErr_Format(PyExc_TypeError,
            "cannot induce rules from continuous variables ('%s')",
            contvar->cname());
    }
    TItemSetNode *tree = NULL;
    PyObject *listOfItemsets = NULL;
    try {
		int depth, nOfExamples;
		TDiscDistribution classDist;
		buildTrees(data, tree, depth, nOfExamples, classDist);
		listOfItemsets = PyList_New(0);
		vector<pair<int, int> > itemsSoFar;
		gatherRules(tree, itemsSoFar, listOfItemsets);
    }
    catch (...) {
    	if (tree)
    		delete tree;
        Py_XDECREF(listOfItemsets);
    	throw;
    }
    delete tree;
    return listOfItemsets;
}
