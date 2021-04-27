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


#ifndef __HIERARCHICALCLUSTERING_HPP
#define __HIERARCHICALCLUSTERING_HPP

#include "common.hpp"
#include "progress.hpp"
#include "symmatrix.hpp"
#include "hierarchicalclustering.ppp"


class THierarchicalCluster : public TOrange {
public:
    __REGISTER_CLASS(HierarchicalCluster);
    NEW_NOARGS;

    PHierarchicalCluster left; //P left subcluster
    PHierarchicalCluster right; //P right subcluster
    float height;    //P height
    PIntList mapping; //P indices to the list of all elements in the clustering
    int first;        //P the index into 'elements' to the first element of the cluster
    int last;         //P the index into 'elements' to the one after the last element of the cluster

    inline THierarchicalCluster();

    inline THierarchicalCluster(
        PIntList const &els,
        int const elementIndex);

    inline THierarchicalCluster(
        PIntList const &els,
        PHierarchicalCluster const &, PHierarchicalCluster const &,
        float const h, int const f, int const l);

    void swap();

protected:
    void recursiveMove(int const offset);

public:
    Py_ssize_t __len__();
    PyObject *__item__(Py_ssize_t i);
    PyObject *py_swap() PYARGS(METH_NOARGS, "(); swap the left and the right cluster");
};



class TClusterW;
class TProgressCallback;

class THierarchicalClustering : public TOrange {
public:
    __REGISTER_CLASS(HierarchicalClustering);
    NEW_WITH_CALL(HierarchicalClustering);

    enum Linkage {Single, Average, Complete, Ward} PYCLASSCONSTANTS;
    int linkage; //P(&Linkage) linkage
    bool overwriteMatrix; //P if true (default is false) it will use (and destroy) the distance matrix given as argument

    PProgressCallback progressCallback; //P progress callback function

    THierarchicalClustering();

    PHierarchicalCluster operator()(PSymMatrix const &);

    TClusterW **init(int const dim, float *distanceMatrix);
    TClusterW *merge(TClusterW **clusters, float *callbackMilestones);
    PHierarchicalCluster restructure(TClusterW *);
    PHierarchicalCluster restructure(
        TClusterW *,
        PIntList const &elementIndices,
        TIntList::iterator &currentElement,
        int &currentIndex);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(distances); cluster data");

private:
    TClusterW *merge_CompleteLinkage(TClusterW **, float *callbackMilestones);
    TClusterW *merge_SingleLinkage(TClusterW **, float *callbackMilestones);
    TClusterW *merge_AverageLinkage(TClusterW **, float *callbackMilestones);
    // Average linkage also computes Ward's linkage
};


class THierarchicalClusterOrdering: public TOrange {
public:
	__REGISTER_CLASS(HierarchicalClusterOrdering);
    NEW_WITH_CALL(HierarchicalClusterOrdering);

	PProgressCallback progress_callback; //P progress callback function
	PHierarchicalCluster operator() (
        PHierarchicalCluster const &root, PSymMatrix const &matrix);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(clustering, distances); order sub clusters");
};


THierarchicalCluster::THierarchicalCluster()
: height(0.0),
  first(0),
  last(0)
{}


THierarchicalCluster::THierarchicalCluster(
    PIntList const &els, int const elementIndex)
: height(0.0),
  mapping(els),
  first(elementIndex),
  last(elementIndex+1)
{}


THierarchicalCluster::THierarchicalCluster(
    PIntList const &els,
    PHierarchicalCluster const &aLeft, PHierarchicalCluster const &aRight,
    float const h, int const f, int const l)
: left(aLeft), right(aRight),
  height(h),
  mapping(els),
  first(f),
  last(l)
{}


#endif
