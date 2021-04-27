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
#include "assocrulesinducer-sparse.px"

class TSparseExample{
public:
	double weight;			// weight of thi example
	long *itemset;		// vector storing just items that have some value in original example
	int	length;

	TSparseExample(TExample *const example, double weight);
    ~TSparseExample();
    TSparseExample(const TSparseExample &);
};


class TSparseExamples{
public:
	double fullWeight;					// weight of all examples
	vector<TSparseExample*> transaction;	// vector storing all sparse examples
	PDomain domain;						// domain of original example or exampleGenerator
	vector<long> intDomain;				// domain mapped longint values

	TSparseExamples(PExampleTable const &);
	~TSparseExamples();
};


class TSparseItemsetNode;
typedef map<long, TSparseItemsetNode *> TSparseISubNodes;

class TSparseItemsetNode{							//item node used in TSparseItemsetTree
public:
    double weiSupp;							//support of itemset consisting node and all of its parents
    long value;								//value of this node
    TSparseItemsetNode *parent;					//pointer to parent node
    TSparseISubNodes subNode;				//children items
    vector<int> exampleIds;

    TSparseItemsetNode(long avalue = -1);			//constructor
    ~TSparseItemsetNode();

    TSparseItemsetNode *operator[] (long const avalue);	//directly gets subnode

    TSparseItemsetNode* addNode(long const avalue);		//adds new subnode
    bool hasNode(long avalue);				//returns true if has subnode with given value
};


/****************************************************************************************
TSparseExample
*****************************************************************************************/

TSparseExample::TSparseExample(TExample *const example, double aWeight)
    : weight(aWeight)
{
	length = 0;

    if (example->domain->variables->size()) {
        TExample::const_iterator ei(example->begin());
        TExample::const_iterator const ee(example->end());
        TVarList::const_iterator vi(example->domain->variables->begin());
        for(; ei != ee; ei++, vi++) {
            if (!isnan(*ei) 
                && (((*vi)->varType != TVariable::Continuous) || (*ei > 1e-6))) {
                length++;
            }
        }
        itemset = new long[length];
        int vn = 0;
        long *ii = itemset;
        ei = example->begin();
        vi = example->domain->variables->begin();
        for(; ei != ee; ei++, vi++, vn++) {
            TValue val = example->getValue(*vi);
            if (   !isnan(val)
                && (((*vi)->varType != TVariable::Continuous) || (*ei > 1e-6))) {
                    *ii++ = vn;
            }
        }
    }
    else {
        length = 0;
        ITERATE_METAS(*example, mr)
            length++;
        }
        itemset = new long[length];
        long *ii = itemset;
        ITERATE_METAS(*example, mr)
            *ii++ = mr.id;
        }
        sort(itemset, ii);
    }
}

TSparseExample::~TSparseExample()
{
    delete itemset;
    itemset = NULL;
}


TSparseExample::TSparseExample(const TSparseExample &other)
: weight(other.weight),
  itemset(new long[length]),
  length(other.length)
{
    if (other.itemset) {
        memcpy(itemset, other.itemset, length*sizeof(long));
    }
}

/****************************************************************************************
TSparseExamples
*****************************************************************************************/

TSparseExamples::TSparseExamples(PExampleTable const &examples)
{
    fullWeight = 0.0;
    TSparseExample *sparseExm;
    domain = examples->domain;

    const bool sparseExamples = examples->domain->variables->empty();
    set<long> ids;

    // walk through all examples converting them to sparseExample and
    // add them to transaction
    PEITERATE(example, examples) {
        sparseExm = new TSparseExample(*example, example.getWeight());
        if (sparseExamples) {
            for(long *vi = sparseExm->itemset, le = sparseExm->length; le--; vi++) {
                ids.insert(*vi);
            }
        }
        transaction.push_back(sparseExm);
        fullWeight += sparseExm->weight;
    }

    // walk through all existing attributes in example and add them to intDomain
    if (sparseExamples) {
        intDomain.reserve(ids.size());
        ITERATE(set<long>, si, ids)
            intDomain.push_back(*si);
    }
    else {
        for(int i = 0, e = examples->domain->variables->size(); i!=e; i++)
            intDomain.push_back(i);
    }
}

TSparseExamples::~TSparseExamples()
{
  for(vector<TSparseExample*>::const_iterator bi=transaction.begin(), be=transaction.end(); bi!=be; bi++)
    delete *bi;
}

/****************************************************************************************
TSparseItemsetNode
*****************************************************************************************/

TSparseItemsetNode::TSparseItemsetNode(long avalue) {
	weiSupp = 0.0;
	value = avalue;
};

TSparseItemsetNode::~TSparseItemsetNode()
{
  ITERATE(TSparseISubNodes, ii, subNode)
    delete ii->second;
}

TSparseItemsetNode *TSparseItemsetNode::operator[] (long avalue) {
	return subNode[avalue];
};

//if no subNode with that key exists add new
TSparseItemsetNode* TSparseItemsetNode::addNode(long avalue) {
	if (subNode.find(avalue)==subNode.end()) {
		subNode[avalue] = new TSparseItemsetNode(avalue);
		subNode[avalue]->parent = this;
	}
	//returns new node
	return subNode[avalue];
};

bool TSparseItemsetNode::hasNode(long avalue) {
	return (subNode.find(avalue)!=subNode.end());
};

/****************************************************************************************
TSparseItemsetTree
*****************************************************************************************/

// constructor
TSparseItemsetTree::TSparseItemsetTree(const TSparseExamples &examples){
	root = new TSparseItemsetNode();
	domain = examples.domain;
};

TSparseItemsetTree::~TSparseItemsetTree()
{
  delete root;
}

// generates all itemsets with one element
int TSparseItemsetTree::buildLevelOne(vector<long> intDomain) {
	int count = 0;

	ITERATE(vector<long>,idi,intDomain) {
		root->addNode(*idi);
		count++;
	}

	return count;
};

// generates candiate itemsets of size k from large itemsets of size k-1
long TSparseItemsetTree::extendNextLevel(int maxDepth, long maxCount) {
	typedef pair<TSparseItemsetNode *,int> NodeDepth; //<node,depth>

	long count = 0;
	vector<NodeDepth> nodeQue;

	long *cItemset = new long[maxDepth +1];
	int currDepth;
	TSparseItemsetNode *currNode;

	nodeQue.push_back(NodeDepth(root,0)); // put root in que

	while (!nodeQue.empty()) {			//repeats until que is empty
		currNode = nodeQue.back().first;			// node
		currDepth = nodeQue.back().second;			// depth

		nodeQue.pop_back();

		if (currDepth) cItemset[currDepth - 1] = currNode->value;		// generates candidate itemset

		if (currDepth == maxDepth) 										// we found an instance that can be extended
			for(TSparseISubNodes::iterator iter(++(root->subNode.find(currNode->value))), \
											   iter_end(root->subNode.end()); \
				iter!=iter_end; \
				iter++) {
					cItemset[currDepth] = iter->second->value;

					if (allowExtend(cItemset, currDepth + 1)) {
						currNode->addNode(cItemset[currDepth]);
						count++;
						if (count>maxCount) {
						  delete cItemset;
						  return count;
						}
					}
				}
		else RITERATE(TSparseISubNodes,sni,currNode->subNode)		//adds subnodes to list
			nodeQue.push_back(NodeDepth(sni->second, currDepth + 1));
	}
	delete cItemset;
	return count;
};


// tests if some candidate itemset can be extended to large itemset
bool TSparseItemsetTree::allowExtend(long itemset[], int iLength) {
	typedef pair<int,int> IntPair; // <parent node index, depth>
	typedef pair<TSparseItemsetNode *,IntPair> NodeDepth;

	vector<NodeDepth> nodeQue;

	int currDepth;
	int currPrIndex;								//parent index
	TSparseItemsetNode *currNode;
	int i;

	nodeQue.push_back(NodeDepth(root,IntPair(-1,1))); // put root in que

	while (!nodeQue.empty()) {						//repeats until que is empty
		currNode = nodeQue.back().first;			// node
		currPrIndex = nodeQue.back().second.first;	// parentIndex
		currDepth = nodeQue.back().second.second;	// depth

		nodeQue.pop_back();

		if (currDepth == iLength) continue;			// we found an instance

		for (i = currDepth; i!=currPrIndex; i--)		//go through all posible successors of this node
			if (currNode->hasNode(itemset[i]))
				nodeQue.push_back(NodeDepth((*currNode)[itemset[i]],IntPair(i,currDepth + 1)));
			else return 0;
	}
	return 1;
}


// counts number of leaf nodes not using any recursion
long TSparseItemsetTree::countLeafNodes() {
	long countLeaf = 0;
	vector<TSparseItemsetNode *> nodeQue;
	TSparseItemsetNode *currNode;

	nodeQue.push_back(root);

	while (!nodeQue.empty()) {					//repeats until que is empty
		currNode = nodeQue.back();
		nodeQue.pop_back();

		if (!currNode->subNode.empty()) 		//if node is leaf count++ else count children
			RITERATE(TSparseISubNodes,sni,currNode->subNode)
				nodeQue.push_back(sni->second);
		else countLeaf++;						// node is leaf
	}

	return countLeaf;
};


// counts supports of all aimLength long branches in tree using one example (itemset) data
void TSparseItemsetTree::considerItemset(long itemset[], int iLength, double weight, int aimLength) {
	typedef pair<int,int> IntPair; // <parent node index, depth>
	typedef pair<TSparseItemsetNode *,IntPair> NodeDepth;

	vector<NodeDepth> nodeQue;

	int currDepth;
	int currPrIndex;								//parent index
	TSparseItemsetNode *currNode;
	int i, end = iLength - aimLength;

	nodeQue.push_back(NodeDepth(root,IntPair(-1,0))); // put root in que

	while (!nodeQue.empty()) {						//repeats until que is empty
		currNode = nodeQue.back().first;			// node
		currPrIndex = nodeQue.back().second.first;	// parentIndex
		currDepth = nodeQue.back().second.second;	// depth

		nodeQue.pop_back();

		if (currDepth == aimLength) { currNode->weiSupp += weight; continue;}	// we found an instance
		if (currNode->subNode.empty()) continue;	// if node does't have successors

		for (i = currDepth + end; i!=currPrIndex; i--)		//go through all posible successors of this node
			if (currNode->hasNode(itemset[i]))
				nodeQue.push_back(NodeDepth((*currNode)[itemset[i]],IntPair(i,currDepth + 1)));
	}
};

// counts supports of all aimLength long branches in tree using examples data
void TSparseItemsetTree::considerExamples(TSparseExamples *examples, int aimLength) {
		ITERATE(vector<TSparseExample*>,ei,examples->transaction)
			if (aimLength <= (*ei)->length)
				considerItemset((*ei)->itemset, (*ei)->length, (*ei)->weight, aimLength);
}

void TSparseItemsetTree::assignExamples(TSparseItemsetNode *node, long *itemset, long *itemsetend, const int exampleId)
{
  node->exampleIds.push_back(exampleId);
  if (!node->subNode.empty())
    for(; itemset != itemsetend; itemset++)
      if (node->hasNode(*itemset))
        assignExamples((*node)[*itemset], itemset+1, itemsetend, exampleId);
}

void TSparseItemsetTree::assignExamples(TSparseExamples &examples)
{
  int exampleId = 0;
  ITERATE(vector<TSparseExample*>,ei,examples.transaction)
    assignExamples(root, (*ei)->itemset, (*ei)->itemset+(*ei)->length, exampleId++);
}



// deletes all leaves that have weiSupp smaler than given minSupp;
void TSparseItemsetTree::delLeafSmall(double minSupp) {
	long countLeaf = 0;
	vector<TSparseItemsetNode *> nodeQue;
	TSparseItemsetNode *currNode;

	nodeQue.push_back(root);

	while (!nodeQue.empty()) {			//repeats until que is empty
		currNode = nodeQue.back();
		nodeQue.pop_back();

		if (!currNode->subNode.empty()) 	//if node is not leaf add children else check support
			RITERATE(TSparseISubNodes,sni,currNode->subNode)
				nodeQue.push_back(sni->second);
		else
			if (currNode->weiSupp < minSupp) {
				currNode->parent->subNode.erase(currNode->value);
				delete currNode;
			}
	}
};


// generates all possible association rules from tree using given confidence
PAssociationRules TSparseItemsetTree::genRules(int maxDepth, double minConf, double nOfExamples, bool storeExamples) {
    typedef pair<TSparseItemsetNode *,int> NodeDepth; //<node,depth>

    int count=0;
    vector<NodeDepth> nodeQue;

    PAssociationRules rules(new TAssociationRules());

    long *itemset = new long[maxDepth];
    int currDepth;
    TSparseItemsetNode *currNode;

    nodeQue.push_back(NodeDepth(root,0)); // put root in que

    while (!nodeQue.empty()) {						//repeats until que is empty
        currNode = nodeQue.back().first;			// node
        currDepth = nodeQue.back().second;			// depth

        nodeQue.pop_back();

        if (currDepth) itemset[currDepth - 1] = currNode->value;  // create itemset to check for confidence

        if (currDepth > 1)
            count += getItemsetRules(itemset, currDepth, minConf, currNode->weiSupp, nOfExamples, rules, storeExamples, currNode);	//creates rules from itemsets and adds them to rules

        RITERATE(TSparseISubNodes,sni,currNode->subNode)		//adds subnodes to list
            nodeQue.push_back(NodeDepth(sni->second, currDepth + 1));
    }
    delete itemset;
    return rules;
};

// checks if itemset generates some rules with enough confidence and adds these rules to resultset
long TSparseItemsetTree::getItemsetRules(long itemset[], int iLength, double minConf,
    double nAppliesBoth, double nOfExamples,
    PAssociationRules rules,
    bool storeExamples, TSparseItemsetNode *bothNode) {

        double nAppliesLeft, nAppliesRight;
        long count = 0;
        PAssociationRule rule;
        PExample exLeft=TExample::constructFree(domain);
        PExample exRight=TExample::constructFree(domain);
        const bool sparseRules = domain->variables->empty();

        nAppliesLeft=nAppliesBoth;
        nAppliesRight=nAppliesBoth;

        typedef pair<int,int> IntPair; // <parent node index, depth>
        typedef pair<TSparseItemsetNode *,IntPair> NodeDepth;

        vector<NodeDepth> nodeQue;

        int currDepth, i, j;
        int currPrIndex; //parent index
        TSparseItemsetNode *currNode, *tempNode;

        long *leftItemset = new long[iLength - 1];
        double thisConf;

        nodeQue.push_back(NodeDepth(root,IntPair(-1,0))); // put root in que

        while (!nodeQue.empty()) {			//repeats until que is empty
            currNode = nodeQue.back().first;			// node
            currPrIndex = nodeQue.back().second.first;	// parentIndex
            currDepth = nodeQue.back().second.second;	// depth

            nodeQue.pop_back();

            nAppliesLeft = currNode->weiSupp;			// support of left side
            thisConf = nAppliesBoth/nAppliesLeft;

            if (thisConf >= minConf) {	// if confidence > min confidence do ... else don't folow this branch
                if (currDepth) {
                    leftItemset[currDepth-1] = currNode->value;

                    if (sparseRules) {
                        PExample exLeftS = TExample::constructFree(domain);
                        PExample exRightS = TExample::constructFree(domain);

                        tempNode = root;
                        for(i=0, j=0; (i<currDepth) && (j<iLength); ) {
                            if (itemset[j] < leftItemset[i]) {
                                exRightS->setMeta(itemset[j], TValue(1.0));
                                tempNode = (*tempNode)[itemset[j]];
                                j++;
                            }
                            else {
                                _ASSERT(itemset[j] == leftItemset[i]);
                                exLeftS->setMeta(leftItemset[i], TValue(1.0));
                                i++;
                                j++;
                            }
                        }

                        _ASSERT(i==currDepth);
                        for(; j<iLength; j++) {
                            exRightS->setMeta(itemset[j], TValue(1.0));
                            tempNode = (*tempNode)[itemset[j]];
                        }

                        /*
                        for (i=0; i<currDepth; i++)		//generating left and right example and get support of left side
                        exLeft[leftItemset[i]] = 1.0;

                        tempNode = root;
                        for (i=0; i< iLength; i++)
                        if (   ) {
                        exRight[itemset[i]] = 1.0;
                        tempNode = (*tempNode)[itemset[i]];
                        }
                        */
                        nAppliesRight = tempNode->weiSupp;	//get support of left side

                        //add rules
                        rule = PAssociationRule(new TAssociationRule(exLeftS, exRightS, nAppliesLeft, nAppliesRight, nAppliesBoth, nOfExamples, currDepth, iLength-currDepth));
                        if (storeExamples) {
                            rule->matchLeft = PIntList(new TIntList(currNode->exampleIds));
                            rule->matchBoth = PIntList(new TIntList(bothNode->exampleIds));
                        }

                        rules->push_back(rule);
                        count ++;
                    }
                    else {
                        raiseError(PyExc_NotImplementedError,
                            "sparse association rules from ordinary values are not implemented yet");
                    }
                    /*  				for (i=0; i<currDepth;i++) {		//generating left and right example and get support of left side
                    exLeft[leftItemset[i]].setSpecial(false);
                    exLeft[leftItemset[i]].varType=0;
                    }


                    tempNode = root;
                    for (i=0; i<iLength;i++)
                    if (exLeft[itemset[i]].isSpecial()) {
                    exRight[itemset[i]].setSpecial(false);
                    exRight[itemset[i]].varType=0;
                    tempNode = (*tempNode)[itemset[i]];
                    }

                    nAppliesRight = tempNode->weiSupp;	//get support of left side

                    //add rules
                    rule = mlnew TAssociationRule(mlnew TExample(exLeft), mlnew TExample(exRight), nAppliesLeft, nAppliesRight, nAppliesBoth, nOfExamples, currDepth, iLength-currDepth);

                    if (storeExamples) {
                        rule->matchLeft = new TIntList(currNode->exampleIds);
                        rule->matchBoth = new TIntList(bothNode->exampleIds);
                    }
                    rules->push_back(rule);
                    count ++;

                    for (i=0; i<iLength;i++) {					//deleting left and right example
                    exLeft[itemset[i]].setSpecial(true);
                    exLeft[itemset[i]].varType=1;
                    exRight[itemset[i]].setSpecial(true);
                    exRight[itemset[i]].varType=1;
                    }
                    }
                    */
                }
                if (currDepth < iLength - 1)							//if we are not too deep
                    for (i = iLength - 1; i!=currPrIndex; i--)		//go through all posible successors of this node
                        if (currNode->hasNode(itemset[i]))				//if this node exists among childrens
                            nodeQue.push_back(NodeDepth((*currNode)[itemset[i]],IntPair(i,currDepth + 1)));
            }
        }

        delete leftItemset;
        return count;
};

/****************************************************************************************
TAssociationRulesSparseInducer
*****************************************************************************************/

TAssociationRulesSparseInducer::TAssociationRulesSparseInducer(double asupp, double aconf, int awei)
: maxItemSets(15000),
  confidence(aconf),
  support(asupp),
  nOfExamples(0.0),
  storeExamples(TExampleTable::DontStore)
{}


TSparseItemsetTree *TAssociationRulesSparseInducer::buildTree(
    PExampleTable const &examples, long &i, double &fullWeight)
{
	double nMinSupp;
	long currItemSets, newItemSets;

	// reformat examples in sparseExm for better efficacy
	TSparseExamples sparseExm(examples);

	fullWeight = sparseExm.fullWeight;

	// build first level of tree
	TSparseItemsetTree *tree = new TSparseItemsetTree(sparseExm);
	newItemSets = tree->buildLevelOne(sparseExm.intDomain);

	nMinSupp = support * sparseExm.fullWeight;

	//while it is possible to extend tree repeat...
	for(i=1;newItemSets;i++) {
		tree->considerExamples(&sparseExm,i);
		tree->delLeafSmall(nMinSupp);

		currItemSets = tree->countLeafNodes();

		newItemSets = tree->extendNextLevel(i, maxItemSets - currItemSets);

		//test if tree is too large
		if (newItemSets + currItemSets >= maxItemSets) {
			raiseError(PyExc_RuntimeError, 
                "too many itemsets (%i); increase 'support' or 'maxItemSets'",
                maxItemSets);
			newItemSets = 0;
		}
	}

	if (storeExamples)
	  tree->assignExamples(sparseExm);

	return tree;
}


PAssociationRules TAssociationRulesSparseInducer::operator()(PExampleTable const &data)
{
  long i;
  double fullWeight;
  TSparseItemsetTree *tree = buildTree(data, i, fullWeight);
  PAssociationRules rules = tree->genRules(i, confidence, fullWeight,
      storeExamples != TExampleTable::DontStore);
  delete tree;
  
  if (storeExamples != TExampleTable::DontStore) {
      PExampleTable xmpls = (storeExamples == TExampleTable::StoreCopy)
          ? TExampleTable::constructCopy(data)
          : TExampleTable::constructReference(data);
      PITERATE(TAssociationRules, ri, rules) {
          (*ri)->examples = xmpls;
      }
  }
  return rules;
}




/****************************************************************************************
TItemsetsSparseInducer
*****************************************************************************************/

TItemsetsSparseInducer::TItemsetsSparseInducer(double asupp, int awei)
: maxItemSets(15000),
  support(asupp),
  nOfExamples(0.0),
  storeExamples(false)
{}

PSparseItemsetTree TItemsetsSparseInducer::operator()(PExampleTable const &examples)
{	double nMinSupp;
	long currItemSets, i,newItemSets;

	// reformat examples in sparseExm for better efficacy
	TSparseExamples sparseExm(examples);

	// build first level of tree
	PSparseItemsetTree tree(new TSparseItemsetTree(sparseExm));
	newItemSets = tree->buildLevelOne(sparseExm.intDomain);

	nMinSupp = support * sparseExm.fullWeight;

	//while it is posible to extend tree repeat...
	for(i=1;newItemSets;i++) {
		tree->considerExamples(&sparseExm,i);
		tree->delLeafSmall(nMinSupp);
		currItemSets = tree->countLeafNodes();
		newItemSets = tree->extendNextLevel(i, maxItemSets - currItemSets);
		//test if tree is too large
		if (newItemSets + currItemSets >= maxItemSets) {
            raiseError(PyExc_RuntimeError,
                "too many itemsets (%i); increase 'support' or 'maxItemSets'",
                maxItemSets);
			newItemSets = 0;
		}
	}

  if (storeExamples != TExampleTable::DontStore)
	  tree->assignExamples(sparseExm);

	return tree;
}


PyObject *TAssociationRulesSparseInducer::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable examples;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:AssociationRulesSparseInducer",
        AssociationRulesSparseInducer_call_keywords,
        &PExampleTable::argconverter, &examples)) {
            return NULL;
    }
    return (*this)(examples).getPyObject();
}


void TAssociationRulesSparseInducer::gatherRules(
    TSparseItemsetNode *node, vector<int> &itemsSoFar,
    PyObject *listOfItems)
{
    if (itemsSoFar.size()) {
        PyObject *itemset = PyTuple_New(itemsSoFar.size());
        Py_ssize_t el = 0;
        vector<int>::const_iterator sfi(itemsSoFar.begin()), sfe(itemsSoFar.end());
        for(; sfi != sfe; sfi++, el++) {
            PyTuple_SET_ITEM(itemset, el, PyLong_FromLong(*sfi));
        }
        PyObject *examples;
        if (storeExamples != TExampleTable::DontStore) {
            examples = PyList_New(node->exampleIds.size());
            Py_ssize_t ele = 0;
            ITERATE(vector<int>, ei, node->exampleIds) {
                PyList_SetItem(examples, ele++, PyLong_FromLong(*ei));
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
    }
    itemsSoFar.push_back(0);
    ITERATE(TSparseISubNodes, isi, node->subNode) {
        itemsSoFar.back() = (*isi).first;
        gatherRules((*isi).second, itemsSoFar, listOfItems);
    }
    itemsSoFar.pop_back();
}


PyObject *TAssociationRulesSparseInducer::getItemsets(PyObject *args, PyObject *kw)
{
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:get_itemsets",
        AssociationRulesSparseInducer_getItemsets_keywords,
        &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    TSparseItemsetTree *tree = NULL;
    PyObject *listOfItemsets = NULL;
    try {
        long i;
        double fullWeight;
        tree = buildTree(data, i, fullWeight);
        listOfItemsets = PyList_New(0);
        vector<int> itemsSoFar;
        gatherRules(tree->root, itemsSoFar, listOfItemsets);
    }
    catch (...) {
        if (tree) {
            delete tree;
        }
        Py_XDECREF(listOfItemsets);
    	throw;
    }
    delete tree;
    return listOfItemsets;
}


TItemsetNodeProxy::TItemsetNodeProxy(
    TSparseItemsetNode const *const n, PSparseItemsetTree const &t)
: node(n),
tree(t)
{}

PyObject *TItemsetNodeProxy::__get__children(OrItemsetNodeProxy *self)
{
    try {
        TItemsetNodeProxy &me = self->orange;
        PyObject *children = PyDict_New();
        const_ITERATE(TSparseISubNodes, ci, me.node->subNode) {
            PyDict_SetItem(children,
                PyLong_FromLong(ci->first),
                PyObject_FromNewOrange(new(&OrItemsetNodeProxy_Type)
                    TItemsetNodeProxy(ci->second, me.tree)));
        }
        return children;
    }
    PyCATCH
}

PyObject *TItemsetNodeProxy::__get__examples(OrItemsetNodeProxy *self)
{
    try {
        TSparseItemsetNode const *const &node = self->orange.node;
        PyObject *examples = PyList_New(node->exampleIds.size());
        Py_ssize_t i = 0;
        const_ITERATE(vector<int>, ci, node->exampleIds) {
            PyList_SetItem(examples, i++, PyLong_FromLong(*ci));
        }
        return examples;
    }
    PyCATCH
}

PyObject *TItemsetNodeProxy::__get__support(OrItemsetNodeProxy *self)
{
    try {
        return PyFloat_FromDouble(self->orange.node->weiSupp);
    }
    PyCATCH
}

PyObject *TItemsetNodeProxy::__get__itemId(OrItemsetNodeProxy *self)
{
    try {
        return PyFloat_FromDouble(self->orange.node->value);
    }
    PyCATCH
}


PyObject *TItemsetsSparseInducer::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable examples;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:ItemsetsSparseInducer",
        ItemsetsSparseInducer_call_keywords,
        &PExampleTable::argconverter, &examples)) {
            return NULL;
    }
    PSparseItemsetTree tree((*this)(examples));
    return PyObject_FromNewOrange(new(&OrItemsetNodeProxy_Type)
        TItemsetNodeProxy(tree->root, tree));
}


