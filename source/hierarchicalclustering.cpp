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
#include "progress.hpp"

#include "hierarchicalclustering.px"

class TClusterW {
public:
    TClusterW *next; // for cluster, this is next cluster, for elements next element
    TClusterW *left, *right; // subclusters, if left==NULL, this is an element
    int size;
    int elementIndex;
    float height;

    float *distances; // distances to clusters before this one (lower left matrix)
    float minDistance; // minimal distance
    int rawIndexMinDistance; // index of minimal distance
    int nDistances;

    inline TClusterW(int const elIndex, float *adistances, int const anDistances)
    : next(NULL),
      left(NULL),
      right(NULL),
      size(1),
      elementIndex(elIndex),
      height(0.0),
      distances(adistances),
      minDistance(numeric_limits<float>::max()),
      rawIndexMinDistance(-1),
      nDistances(anDistances)
    {
        if (distances) {
            computeMinimalDistance();
        }
    }

    inline void elevate(TClusterW **aright, float const aheight)
    {
        left = new TClusterW(*this);
        right = *aright;

        left->distances = NULL;
        size = left->size + right->size;
        elementIndex = -1;
        height = aheight;

        if (next == right) {
            next = right->next;
        }
        else {
            *aright = right->next;
        }
    }


    inline void computeMinimalDistance()
    {
        float *dp = distances, *minp = dp++;
        for(int i = nDistances; --i; dp++) {
            if ((*dp >= 0) && (*dp < *minp)) {
                minp = dp;
            }
        }
        minDistance = *minp;
        rawIndexMinDistance = minp - distances;
    }

    inline TClusterW **clusterAt(int rawIndex, TClusterW **cluster)
    {
        for(float *dp = distances; rawIndex; dp++)
            if (*dp>=0) {
                cluster = &(*cluster)->next;
                rawIndex--;
            }
            return cluster;
    }

    void clearDistances()
    {
        raiseError(PyExc_NotImplementedError,
            "TClusterW: rewrite clean to adjust indexMinDistance");

        float *dst = distances;
        int i = nDistances;
        for(; nDistances && (*dst >= 0); nDistances--, dst++);
        if (!nDistances)
            return;

        float *src = dst;
        for(src++, nDistances--; nDistances && (*src >= 0); src++)
            if (*src >= 0)
                *dst++ = *src;

        nDistances = dst - distances;
    }
};


void THierarchicalCluster::swap()
{
    const TIntList::iterator beg0 = mapping->begin() + left->first;
    const TIntList::iterator beg1 = mapping->begin() + right->first;
    const TIntList::iterator end1 = mapping->begin() + right->last;
    if ((beg0 > beg1) || (beg1 > end1)) {
        raiseError(PyExc_SystemError,
            "internal inconsistency in clustering structure: "
            "invalid ordering of left's and right's 'first' and 'last'");
    }
    TIntList::iterator li0, li1;
    int *temp = new int [beg1 - beg0], *t;
    for(li0 = beg0, t = temp; li0 != beg1; *t++ = *li0++);
    for(li0 = beg0, li1 = beg1; li1 != end1; *li0++ = *li1++);
    for(t = temp; li0 != end1; *li0++ = *t++);
    delete temp;

    left->recursiveMove(end1 - beg1);
    right->recursiveMove(beg0 - beg1);

    PHierarchicalCluster tbr = left;
    left = right;
    right = tbr;
}


void THierarchicalCluster::recursiveMove(int const offset)
{
    first += offset;
    last += offset;
    if (left) {
        left->recursiveMove(offset);
    }
    if (right) {
        right->recursiveMove(offset);
    }
}


Py_ssize_t THierarchicalCluster::__len__()
{
    return last - first;
}


PyObject *THierarchicalCluster::__item__(Py_ssize_t i)
{
    if (!mapping) {
        return PyErr_Format(PyExc_ValueError,
            "cluster has no mapping to original objects");
    }
    i += first;
    if (i > last) {
        return PyErr_Format(PyExc_IndexError, "index out of range");
    }
	if (i >= mapping->size()) {
		return PyErr_Format(PyExc_SystemError,
            "internal inconsistency in 'HierarchicalCluster' (mapping too short)");
    }
	const int elindex = mapping->at(i);
    PyObject *objs = PyObject_GetAttrString(mapping.borrowPyObject(), "objects");
    if (objs && (objs != Py_None)) {
        return PySequence_GetItem(objs, elindex);
	}
    PyErr_Clear();
	return PyLong_FromLong(elindex);

}


PyObject *THierarchicalCluster::py_swap()
{
    swap();
    Py_RETURN_NONE;
}


THierarchicalClustering::THierarchicalClustering()
: linkage(Single),
  overwriteMatrix(false)
{}


TClusterW **THierarchicalClustering::init(int const dim, float *distanceMatrix)
{
    for(float *ddi = distanceMatrix, *dde = ddi + ((dim-1)*(dim+2))/2; ddi!=dde; ddi++) {
        if (*ddi < 0) {
            int x, y;
            TSymMatrix::index2coordinates(ddi-distanceMatrix, x, y);
            raiseError(PyExc_ValueError,
                "distance matrix contains negative element at (%i, %i)", x, y);
        }
    }
    TClusterW **clusters = new TClusterW *[dim];
    TClusterW **clusteri = clusters;
    *clusters = new TClusterW(0, NULL, 0);
    distanceMatrix++;
    for(int elementIndex = 1, e = dim; elementIndex < e; distanceMatrix += ++elementIndex) {
        TClusterW *newcluster = new TClusterW(elementIndex, distanceMatrix, elementIndex);
        (*clusteri++)->next = newcluster;
        *clusteri = newcluster; 
    }
    return clusters;
}



TClusterW *THierarchicalClustering::merge_SingleLinkage(
    TClusterW **clusters, float *milestones)
{
    float *milestone = milestones;

    int step = 0;
    while((*clusters)->next) {
        if (milestone && (step++ ==*milestone)) {
            (*progressCallback)(*((++milestone)++));
        }
        float minDistance = numeric_limits<float>::max();
        TClusterW *cluster;
        TClusterW **pcluster2;
        TClusterW **tcluster = &((*clusters)->next);
        for(; *tcluster; tcluster = &(*tcluster)->next) {
            if ((*tcluster)->minDistance < minDistance) {
                minDistance = (*tcluster)->minDistance;
                pcluster2 = tcluster;
            }
        }
        TClusterW *const cluster2 = *pcluster2;
        const int rawIndex1 = cluster2->rawIndexMinDistance;
        const int rawIndex2 = cluster2->nDistances;
        TClusterW *const cluster1 = clusters[rawIndex1];
        float *disti1 = cluster1->distances;
        float *disti2 = cluster2->distances;
        if (rawIndex1) { // not root - has no distances...
            const float *minIndex1 = cluster1->distances + cluster1->rawIndexMinDistance;
            for(int ndi = cluster1->nDistances; ndi--; disti1++, disti2++) {
                if (*disti2 < *disti1) { // if one is -1, they both are
                    if ((*disti1 = *disti2) < *minIndex1) {
                        minIndex1 = disti1;
                    }
                }
            }
            cluster1->minDistance = *minIndex1;
            cluster1->rawIndexMinDistance = minIndex1 - cluster1->distances;
        }
        while(*disti2 < 0) {
            disti2++;        // should have at least one more >=0  - the one corresponding to distance to cluster1
        }
        for(cluster = cluster1->next; cluster != cluster2; cluster = cluster->next) {
            while(*++disti2 < 0); // should have more - the one corresponding to cluster
            if (*disti2 < cluster->distances[rawIndex1]) {
                if ((cluster->distances[rawIndex1] = *disti2) < cluster->minDistance) {
                    cluster->minDistance = *disti2;
                    cluster->rawIndexMinDistance = rawIndex1;
                }
            }
        }
        for(cluster = cluster->next; cluster; cluster = cluster->next) {
            if (cluster->distances[rawIndex2] < cluster->distances[rawIndex1]) {
                cluster->distances[rawIndex1] = cluster->distances[rawIndex2];
            }
            // don't nest this in the above if -- both distances can be equal, yet index HAS TO move
            if (rawIndex2 == cluster->rawIndexMinDistance) {
                cluster->rawIndexMinDistance = rawIndex1; // the smallest element got moved
            }
            cluster->distances[rawIndex2] = -1;
        }
        cluster1->elevate(pcluster2, minDistance);
    }
    return *clusters;
}

// Also computes Ward's linkage
TClusterW *THierarchicalClustering::merge_AverageLinkage(TClusterW **clusters, float *milestones)
{
    float *milestone = milestones;
    bool ward = linkage == Ward;

    int step = 0;
    while((*clusters)->next) {
        if (milestone && (step++ ==*milestone)) {
            (*progressCallback)(*((++milestone)++));
        }
        float minDistance = numeric_limits<float>::max();
        TClusterW *cluster;
        TClusterW **pcluster2;
        for(TClusterW **tcluster = &((*clusters)->next); *tcluster; tcluster = &(*tcluster)->next) {
            if ((*tcluster)->minDistance < minDistance) {
                minDistance = (*tcluster)->minDistance;
                pcluster2 = tcluster;
            }
        }
        TClusterW *const cluster2 = *pcluster2;
        const int rawIndex1 = cluster2->rawIndexMinDistance;
        const int rawIndex2 = cluster2->nDistances;
        TClusterW *const cluster1 = clusters[rawIndex1];
        float *disti1 = cluster1->distances;
        float *disti2 = cluster2->distances;
        const int size1 = cluster1->size;
        const int size2 = cluster2->size;
        const int sumsize = size1 + size2;
        cluster = (*clusters)->next;
        if (rawIndex1) { // not root - has no distances...
            const float sizeK = cluster->size;
            *disti1 = ward ? (*disti1 * (size1+sizeK) + *disti2 * (size2+sizeK) - minDistance * sizeK) / (sumsize+sizeK)
                : (*disti1 * size1 + *disti2 * size2) / sumsize;
            const float *minIndex1 = disti1;
            int ndi = cluster1->nDistances-1;
            for(disti1++, disti2++, cluster = cluster->next; ndi--; disti1++, disti2++) {
                if (*disti1 >= 0) {
                    const float sizeK = cluster->size;
                    cluster = cluster->next;
                    *disti1 = ward ? (*disti1 * (size1+sizeK) + *disti2 * (size2+sizeK) - minDistance * sizeK) / (sumsize+sizeK)
                        : (*disti1 * size1         + *disti2 * size2) / sumsize;
                    if (*disti1 < *minIndex1) {
                        minIndex1 = disti1;
                    }
                }
            }
            cluster1->minDistance = *minIndex1;
            cluster1->rawIndexMinDistance = minIndex1 - cluster1->distances;
        }

        while(*disti2 < 0) {
            disti2++;        // should have at least one more >=0  - the one corresponding to distance to cluster1
        }
        for(cluster = cluster1->next; cluster != cluster2; cluster = cluster->next) {
            while(*++disti2 < 0); // should have more - the one corresponding to cluster
            float &distc = cluster->distances[rawIndex1];
            const float sizeK = cluster->size;
            distc = ward ? (distc * (size1+sizeK) + *disti2 * (size2+sizeK) - minDistance * sizeK) / (sumsize+sizeK)
                : (distc * size1         + *disti2 * size2) / sumsize;
            if (distc < cluster->minDistance) {
                cluster->minDistance = distc;
                cluster->rawIndexMinDistance = rawIndex1;
            }
            else if ((distc > cluster->minDistance) && (cluster->rawIndexMinDistance == rawIndex1)) {
                cluster->computeMinimalDistance();
            }
        }

        for(cluster = cluster->next; cluster; cluster = cluster->next) {
            float &distc = cluster->distances[rawIndex1];
            const float sizeK = cluster->size;
            distc = ward ? (distc * (size1+sizeK) + cluster->distances[rawIndex2] * (size2+sizeK) - minDistance * sizeK) / (sumsize+sizeK)
                : (distc * size1         + cluster->distances[rawIndex2] * size2) / sumsize;
            cluster->distances[rawIndex2] = -1;
            if (distc < cluster->minDistance) {
                cluster->minDistance = distc;
                cluster->rawIndexMinDistance = rawIndex1;
            }
            else if (   (distc > cluster->minDistance) && (cluster->rawIndexMinDistance == rawIndex1)
                || (cluster->rawIndexMinDistance == rawIndex2)) {
                    cluster->computeMinimalDistance();
            }
        }
        cluster1->elevate(pcluster2, minDistance);
    }
    return *clusters;
}



TClusterW *THierarchicalClustering::merge_CompleteLinkage(TClusterW **clusters, float *milestones)
{
    float *milestone = milestones;

    int step = 0;
    while((*clusters)->next) {
        if (milestone && (step++ ==*milestone)) {
            (*progressCallback)(*((++milestone)++));
        }
        float minDistance = numeric_limits<float>::max();
        TClusterW *cluster;
        TClusterW **pcluster2;
        for(TClusterW **tcluster = &((*clusters)->next); *tcluster; tcluster = &(*tcluster)->next) {
            if ((*tcluster)->minDistance < minDistance) {
                minDistance = (*tcluster)->minDistance;
                pcluster2 = tcluster;
            }
        }
        TClusterW *const cluster2 = *pcluster2;
        const int rawIndex1 = cluster2->rawIndexMinDistance;
        const int rawIndex2 = cluster2->nDistances;
        TClusterW *const cluster1 = clusters[rawIndex1];
        float *disti1 = cluster1->distances;
        float *disti2 = cluster2->distances;
        if (rawIndex1) { // not root - has no distances...
            if (*disti2 > *disti1) {
                *disti1 = *disti2;
            }
            const float *minIndex1 = disti1;
            int ndi = cluster1->nDistances-1;
            for(disti1++, disti2++; ndi--; disti1++, disti2++) {
                if (*disti1 >= 0) {
                    if (*disti2 > *disti1) { // if one is -1, they both are
                        *disti1 = *disti2;
                    }
                    if (*disti1 < *minIndex1) {
                        minIndex1 = disti1;
                    }
                }
            }
            cluster1->minDistance = *minIndex1;
            cluster1->rawIndexMinDistance = minIndex1 - cluster1->distances;
        }

        while(*disti2 < 0) {
            disti2++;        // should have at least one more >=0  - the one corresponding to distance to cluster1
        }
        for(cluster = cluster1->next; cluster != cluster2; cluster = cluster->next) {
            while(*++disti2 < 0); // should have more - the one corresponding to cluster
            float &distc = cluster->distances[rawIndex1];
            if (*disti2 > distc) {
                distc = *disti2;
                if (cluster->rawIndexMinDistance == rawIndex1) {
                    if (distc <= cluster->minDistance) {
                        cluster->minDistance = distc;
                    }
                    else {
                        cluster->computeMinimalDistance();
                    }
                }
            }
        }
        for(cluster = cluster->next; cluster; cluster = cluster->next) {
            float &distc = cluster->distances[rawIndex1];
            if (cluster->distances[rawIndex2] > distc) {
                distc = cluster->distances[rawIndex2];
            }
            cluster->distances[rawIndex2] = -1;
            if ((cluster->rawIndexMinDistance == rawIndex1) || (cluster->rawIndexMinDistance == rawIndex2)) {
                cluster->computeMinimalDistance();
            }
        }
        cluster1->elevate(pcluster2, minDistance);
    }
    return *clusters;
}


TClusterW *THierarchicalClustering::merge(TClusterW **clusters, float *milestones)
{
    switch(linkage) {
        case Complete: return merge_CompleteLinkage(clusters, milestones);
        case Single: return merge_SingleLinkage(clusters, milestones);
        case Average: 
        case Ward:
        default: return merge_AverageLinkage(clusters, milestones);
  }
}


PHierarchicalCluster THierarchicalClustering::restructure(TClusterW *root)
{
    PIntList elementIndices(new TIntList(root->size));
    TIntList::iterator currentElement(elementIndices->begin());
    int currentIndex = 0;
    return restructure(root, elementIndices, currentElement, currentIndex);
}


PHierarchicalCluster THierarchicalClustering::restructure(
    TClusterW *node, PIntList const &elementIndices,
    TIntList::iterator &currentElement, int &currentIndex)
{
    PHierarchicalCluster cluster;
    if (!node->left) {
        *currentElement++ = node->elementIndex;
        cluster = PHierarchicalCluster(
            new THierarchicalCluster(elementIndices, currentIndex++));
    }
    else {
        PHierarchicalCluster left = 
            restructure(node->left, elementIndices, currentElement, currentIndex);
        PHierarchicalCluster right =
            restructure(node->right, elementIndices, currentElement, currentIndex);
        cluster = PHierarchicalCluster(
            new THierarchicalCluster(
            elementIndices, left, right,
            node->height, left->first, right->last));
    }
    // no need to care about 'distances' - they've been removed during clustering (in 'elevate') :)
    delete node;
    return cluster;
}


PHierarchicalCluster THierarchicalClustering::operator()(
    PSymMatrix const &distanceMatrix)
{
    float *distanceMatrixElements = NULL;
    TClusterW **clusters = NULL, *root;
    float *callbackMilestones = NULL;
    try {
        const int dim = distanceMatrix->dim;
        const int size = ((dim+1)*(dim+2))/2;
        distanceMatrixElements = overwriteMatrix ? distanceMatrix->elements : 
            (float *)memcpy(new float[size], distanceMatrix->elements, size*sizeof(float));

        clusters = init(dim, distanceMatrixElements);
        callbackMilestones = (progressCallback && (distanceMatrix->dim >= 1000)) ?
            progressCallback->milestones(distanceMatrix->dim) : NULL;
        root = merge(clusters, callbackMilestones);
    }
    catch (...) {
        delete clusters;
        delete callbackMilestones;
        delete distanceMatrixElements;
        throw;
    }
    delete clusters;
    delete callbackMilestones;
    delete distanceMatrixElements;
    return restructure(root);
}


PyObject *THierarchicalClustering::__call__(PyObject *args, PyObject *kw)
{
    PSymMatrix distances;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:HierarchicalClustering",
        HierarchicalClustering_call_keywords,
        &PSymMatrix::argconverter, &distances)) {
            return NULL;
    }
    return (*this)(distances).getPyObject();
}




struct m_element {
	THierarchicalCluster * cluster;
	int left;
	int right;

	m_element(THierarchicalCluster * cluster, int left, int right);
	m_element(const m_element & other);

	inline bool operator< (const m_element & other) const;
	inline bool operator== (const m_element & other) const;
}; // cluster joined at left and right index

struct ordering_element {
	THierarchicalCluster * left;
	unsigned int u; // the left most (outer) index of left luster
	unsigned m; // the rightmost (inner) index of left cluster
	THierarchicalCluster * right;
	unsigned int w; // the right most (outer) index of the right cluster
	unsigned int k; // the left most (inner) index of the right cluster

	ordering_element();
	ordering_element(THierarchicalCluster * left, unsigned int u, unsigned m,
			THierarchicalCluster * right, unsigned int w, unsigned int k);
	ordering_element(const ordering_element & other);
};

m_element::m_element(THierarchicalCluster * _cluster, int _left, int _right):
	cluster(_cluster), left(_left), right(_right)
{}

m_element::m_element(const m_element & other):
	cluster(other.cluster), left(other.left), right(other.right)
{}

bool m_element::operator< (const m_element & other) const
{
	if (cluster < other.cluster)
		return true;
	else
		if (cluster == other.cluster)
			if (left < other.left)
				return true;
			else
				if (left == other.left)
					return right < other.right;
				else
					return false;
		else
			return false;
}

bool m_element::operator== (const m_element & other) const
{
	return cluster == other.cluster && left == other.left && right == other.right;
}

struct m_element_hash
{
	inline size_t operator()(const m_element & m) const
	{
		size_t seed = 0;
		hash_combine(seed, (size_t) m.cluster);
		hash_combine(seed, (size_t) m.left);
		hash_combine(seed, (size_t) m.right);
		return seed;
	}

	// more or less taken from boost::hash_combine
	inline void hash_combine(size_t &seed, size_t val) const
	{
		seed ^= val + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
};

ordering_element::ordering_element():
	left(NULL), u(-1), m(-1),
	right(NULL), w(-1), k(-1)
{}

ordering_element::ordering_element(THierarchicalCluster * _left,
		unsigned int _u,
		unsigned _m,
		THierarchicalCluster * _right,
		unsigned int _w,
		unsigned int _k
	): left(_left), u(_u), m(_m),
	   right(_right), w(_w), k(_k)
{}

ordering_element::ordering_element(const ordering_element & other):
	   left(other.left), u(other.u), m(other.m),
	   right(other.right), w(other.w), k(other.k)
{}

#define USE_TR1 1

#if USE_TR1
	#if _MSC_VER
		#define HAVE_TR1_DIR 0
	#else
		#define HAVE_TR1_DIR 1
	#endif
	// Diffrent includes required 
	#if HAVE_TR1_DIR
		#include <tr1/unordered_map>
	#else
		#include <unordered_map>
	#endif
	typedef std::tr1::unordered_map<m_element, double, m_element_hash> join_scores;
	typedef std::tr1::unordered_map<m_element, ordering_element, m_element_hash> cluster_ordering;
#else
	typedef std::map<m_element, double> join_scores;
	typedef std::map<m_element, ordering_element> cluster_ordering;
#endif

// Return the minimum distance between elements in matrix
//
float min_distance(
		TIntList::iterator indices1_begin,
		TIntList::iterator indices1_end,
		TIntList::iterator indices2_begin,
		TIntList::iterator indices2_end,
		TSymMatrix & matrix)
{
//	TIntList::iterator iter1 = indices1.begin(), iter2 = indices2.begin();
	float minimum = std::numeric_limits<float>::infinity();
	TIntList::iterator indices2;
	for (; indices1_begin != indices1_end; indices1_begin++)
		for (indices2 = indices2_begin; indices2 != indices2_end; indices2++){
			minimum = std::min(matrix.getitem(*indices1_begin, *indices2), minimum);
		}
	return minimum;
}

struct CompareByScores
{
	join_scores & scores;
	const THierarchicalCluster & cluster;
	const int & fixed;

	CompareByScores(join_scores & _scores, const THierarchicalCluster & _cluster, const int & _fixed):
		scores(_scores), cluster(_cluster), fixed(_fixed)
	{}
	bool operator() (int lhs, int rhs)
	{
		m_element left((THierarchicalCluster*)&cluster, fixed, lhs);
		m_element right((THierarchicalCluster*)&cluster, fixed, rhs);
		return scores[left] < scores[right];
	}
};


//#include <iostream>
//#include <cassert>

// This needs to be called with all left, right pairs to
// update all scores for cluster.
void partial_opt_ordering(
		THierarchicalCluster & cluster,
		THierarchicalCluster & left,
		THierarchicalCluster & right,
		THierarchicalCluster & left_left,
		THierarchicalCluster & left_right,
		THierarchicalCluster & right_left,
		THierarchicalCluster & right_right,
		TSymMatrix &matrix,
		join_scores & M,
		cluster_ordering & ordering)
{
	int u = 0, w = 0;
	TIntList & mapping = *cluster.mapping;
	for (TIntList::iterator u_iter = mapping.begin() + left_left.first;
			u_iter != mapping.begin() + left_left.last;
			u_iter++)
		for (TIntList::iterator w_iter = mapping.begin() + right_right.first;
				w_iter != mapping.begin() + right_right.last;
				w_iter++)
		{
			u = *u_iter;
			w = *w_iter;
			float curr_min = std::numeric_limits<float>::infinity();
			int curr_k = 0, curr_m = 0;
			float C = min_distance(mapping.begin() + left_right.first,
					mapping.begin() + left_right.last,
					mapping.begin() + right_left.first,
					mapping.begin() + right_left.last,
					matrix);

			vector<int> m_ordered(mapping.begin() + left_right.first,
					mapping.begin() + left_right.last);
			vector<int> k_ordered(mapping.begin() + right_left.first,
					mapping.begin() + right_left.last);

			// TODO: precompute the scores for m and k in an array and use a simpler
			// comparison function
			std::sort(m_ordered.begin(), m_ordered.end(), CompareByScores(M, left, u));
			std::sort(k_ordered.begin(), k_ordered.end(), CompareByScores(M, right, w));


			int k0 = k_ordered.front();
			m_element m_right_k0(&right, w, k0);
			int m = 0, k = 0;
			for (vector<int>::iterator iter_m=m_ordered.begin(); iter_m != m_ordered.end(); iter_m++)
			{
				m = *iter_m;

				m_element m_left(&left, u, m);

				if (M[m_left] + M[m_right_k0] + C >= curr_min){
					break;
				}
				for (vector<int>::iterator iter_k = k_ordered.begin(); iter_k != k_ordered.end(); iter_k++)
				{
					k = *iter_k;
					m_element m_right(&right, w, k);
					if (M[m_left] + M[m_right] + C >= curr_min)
					{
						break;
					}
					float test_val = M[m_left] + M[m_right] + matrix.getitem(m, k);
					if (curr_min > test_val)
					{
						curr_min = test_val;
						curr_k = k;
						curr_m = m;
					}
				}

			}

			M[m_element(&cluster, u, w)] = curr_min;
			M[m_element(&cluster, w, u)] = curr_min;

//			assert(M[m_element(&cluster, u, w)] == M[m_element(&cluster, u, w)]);
//			assert(M[m_element(&cluster, u, w)] == curr_min);

//			assert(ordering.find(m_element(&cluster, u, w)) == ordering.end());
//			assert(ordering.find(m_element(&cluster, w, u)) == ordering.end());

			ordering[m_element(&cluster, u, w)] = ordering_element(&left, u, curr_m, &right, w, curr_k);
			ordering[m_element(&cluster, w, u)] = ordering_element(&right, w, curr_k, &left, u, curr_m);
		}
}

void order_clusters(
		THierarchicalCluster & cluster,
		TSymMatrix &matrix,
		join_scores & M,
		cluster_ordering & ordering,
		TProgressCallback * callback)
{
    // I don't think we should consider ever having clusters with only a single
    // subclusters, but just for the case: when I removed 'branches' and replaced
    // it with 'left' and 'right' I changed if (cluster.branches) with
    // if (cluster.right) throughout this function.
	if (!cluster.right)
	{
		M[m_element(&cluster, cluster.mapping->at(cluster.first), cluster.mapping->at(cluster.first))] = 0.0;
		return;
	}

    PHierarchicalCluster left = cluster.left;
	PHierarchicalCluster right = cluster.right;

	order_clusters(*left, matrix, M, ordering, callback);
	order_clusters(*right, matrix, M, ordering, callback);

	PHierarchicalCluster  left_left = (!left->right) ? left : left->left;
	PHierarchicalCluster  left_right = (!left->right) ? left : left->right;
	PHierarchicalCluster  right_left = (!right->right) ? right : right->left;
	PHierarchicalCluster  right_right = (!right->right) ? right : right->right;

	// 1.)
	partial_opt_ordering(cluster,
			*left, *right,
			*left_left, *left_right,
			*right_left, *right_right,
			matrix, M, ordering);

	if (right->right)
		// 2.) Switch right branches.
		// (if there are no right branches the ordering has already been evaluated in 1.)
		partial_opt_ordering(cluster,
				*left, *right,
				*left_left, *left_right,
				*right_right, *right_left,
				matrix, M, ordering);

	if (left->right)
		// 3.) Switch left branches.
		// (if there are no left branches the ordering has already been evaluated in 1. and 2.)
		partial_opt_ordering(cluster,
				*left, *right,
				*left_right, *left_left,
				*right_left, *right_right,
				matrix, M, ordering);

    if (left->right && right->right)
		// 4.) Switch both branches.
		partial_opt_ordering(cluster,
				*left, *right,
				*left_right, *left_left,
				*right_right, *right_left,
				matrix, M, ordering);

    if (callback)
		// TODO: count the number of already processed nodes.
		(*callback)(0.0);
}

/* Check if TIntList contains element.
 */
bool contains(TIntList::iterator iter_begin, TIntList::iterator iter_end, int element)
{
	return std::find(iter_begin, iter_end, element) != iter_end;
}

void optimal_swap(THierarchicalCluster * cluster, int u, int w, cluster_ordering & ordering)
{
	if (cluster->right)
	{
		assert(ordering.find(m_element(cluster, u, w)) != ordering.end());
		ordering_element ord = ordering[m_element(cluster, u, w)];

		PHierarchicalCluster left_right = (ord.left->right)? ord.left->right : PHierarchicalCluster();
		PHierarchicalCluster right_left = (ord.right->right)? ord.right->left : PHierarchicalCluster();

		TIntList & mapping = *cluster->mapping;
		if (left_right && !contains(mapping.begin() + left_right->first,
				mapping.begin() + left_right->last, ord.m))
		{
			assert(!contains(mapping.begin() + left_right->first,
							 mapping.begin() + left_right->last, ord.m));
			ord.left->swap();
			left_right = ord.left->right;
			assert(contains(mapping.begin() + left_right->first,
							mapping.begin() + left_right->last, ord.m));
		}
		optimal_swap(ord.left, ord.u, ord.m, ordering);

		assert(mapping.at(ord.left->first) == ord.u);
		assert(mapping.at(ord.left->last - 1) == ord.m);

		if (right_left && !contains(mapping.begin() + right_left->first,
				mapping.begin() + right_left->last, ord.k))
		{
			assert(!contains(mapping.begin() + right_left->first,
							 mapping.begin() + right_left->last, ord.k));
			ord.right->swap();
			right_left = ord.right->left;
			assert(contains(mapping.begin() + right_left->first,
							mapping.begin() + right_left->last, ord.k));
		}
		optimal_swap(ord.right, ord.k, ord.w, ordering);

		assert(mapping.at(ord.right->first) == ord.k);
		assert(mapping.at(ord.right->last - 1) == ord.w);

		assert(mapping.at(cluster->first) == ord.u);
		assert(mapping.at(cluster->last - 1) == ord.w);
	}
}

PHierarchicalCluster THierarchicalClusterOrdering::operator() (
		PHierarchicalCluster const &root,
		PSymMatrix const &matrix)
{
	join_scores M; // scores
	cluster_ordering ordering;
	order_clusters(*root, *matrix, M, ordering, progress_callback.borrowPtr());

	int u = 0, w = 0;
	int min_u = 0, min_w = 0;
	float min_score = std::numeric_limits<float>::infinity();
	TIntList & mapping = *root->mapping;

	for (TIntList::iterator u_iter = mapping.begin() + root->left->first;
			u_iter != mapping.begin() + root->left->last;
			u_iter++)
		for (TIntList::iterator w_iter = mapping.begin() + root->right->first;
					w_iter != mapping.begin() + root->right->last;
					w_iter++)
		{
			u = *u_iter; w = *w_iter;
			m_element el(root.borrowPtr(), u, w);
			if (M[el] < min_score)
			{
				min_score = M[el];
				min_u = u;
				min_w = w;
			}
		}
//	std::cout << "Min score "<< min_score << endl;

	optimal_swap(root.borrowPtr(), min_u, min_w, ordering);
	return root;
}


PyObject *THierarchicalClusterOrdering::__call__(PyObject *args, PyObject *kw)
{
    PSymMatrix distances;
    PHierarchicalCluster cluster;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&O&:HierarchicalClusterOrdering",
        HierarchicalClusterOrdering_call_keywords,
        &PHierarchicalCluster::argconverter, &cluster,
        &PSymMatrix::argconverter, &distances)) {
            return NULL;
    }
    return (*this)(cluster, distances).getPyObject();
}
