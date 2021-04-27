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


// to include Python.h before STL defines a template set (doesn't work with VC 6.0)

#include "common.hpp"
#include "examplesdistance.hpp"
#include "findnearest.px"


class TNNRec {
public:
  int idx;
  double dist;
  long randoff;

  inline TNNRec(int const anidx, double const adist, long const roff)
  : idx(anidx),
  dist(adist),
  randoff(roff)
  {}

  inline bool operator <(const TNNRec &other) const
  { 
      return (dist==other.dist) ? (randoff<other.randoff) : (dist<other.dist);
  }
};


class TNNRecR {
public:
  int idx;
  double dist;
  long randoff;

  inline TNNRecR(int const anidx, double const adist, long const roff)
  : idx(anidx),
  dist(adist),
  randoff(roff) 
  {}

  inline TNNRecR &operator = (TNNRecR const &old)
  { 
      idx = old.idx;
      dist = old.dist; 
      randoff = old.randoff;
      return *this;
  }

  inline bool operator <(const TNNRecR &other) const
  { 
      return (dist==other.dist) ? (randoff>other.randoff) : (dist>other.dist);
  }
};


class TNNRecW : public TNNRecR {
public:
  double weight;

  inline TNNRecW(int const anidx, double const adist,
                 long const roff, double const wei)
  : TNNRecR(anidx, adist, roff),
    weight(wei)
  {}
};


TFindNearest::TFindNearest()
: distanceID(0),
includeSame(true)
{}


TFindNearest::TFindNearest(PExampleTable const &anex,
                           PExamplesDistance const &adist,
                           TMetaId const anID,
                           bool const is)
: distanceID(anID),
  includeSame(is),
  distance(adist),
  examples(anex)
{}


template<class THeap>
PExampleTable TFindNearest::examplesFromHeap(THeap const &NN,
                                             TMetaId const distanceID) const
{
    PExampleTable res = TExampleTable::constructEmptyReference(examples);
    res->reserveRefs(NN.size());
    const_ITERATE(typename THeap, nni, NN) {
        PExample ex = examples->at(nni->idx);
        if (distanceID) {
            ex->setMeta(distanceID, nni->dist);
        }
        res->push_back(ex);
    }
    return res;
}


PExampleTable TFindNearest::operator()(TExample const *const e,
                                       double const k,
                                       bool const needsClass,
                                       TMetaId const aspecDistanceID) const
{ 
    checkProperty(examples);
    checkProperty(distance);
    TMetaId specDistanceID = aspecDistanceID != ILLEGAL_INT 
        ? aspecDistanceID : distanceID;
    PExample wex = e->convertedTo(examples->domain);
    TExample const *const tex = wex.borrowPtr();
    TRandomGenerator rgen(e->checkSum());
    const bool reallyNeedsClass = needsClass && bool(examples->domain->classVar);
    if (k <= 0.0) {
        vector<TNNRec> NN;
        NN.reserve(examples->size());
        PEITERATE(ei, examples) {
            if (!(reallyNeedsClass && isnan(ei->getClass()))) {
                double const dist = (*distance)(tex, *ei);
                if (includeSame || (dist>0.0)) {
                    NN.push_back(TNNRec(ei.index(), dist, rgen.randlong()));
                }
            }
        }
        sort(NN.begin(), NN.end());
        return examplesFromHeap(NN, specDistanceID);
    }
    else if (!examples->hasWeights()) {
        vector<TNNRecR> NN;
        NN.reserve(k+1);
        TExampleTable::iterator ei(examples->begin());
        for(; (NN.size() < k) && ei; ++ei) {
             if (!(reallyNeedsClass && isnan(ei->getClass()))) {
                const double dist = (*distance)(tex, *ei);
                if (includeSame || (dist>0.0)) {
                    NN.push_back(TNNRecR(ei.index(), dist, rgen.randlong()));
                }
             }
        }
        make_heap(NN.begin(), NN.end());
        for(; ei; ++ei) {
             if (!(reallyNeedsClass && isnan(ei->getClass()))) {
                const double dist = (*distance)(tex, *ei);
                if (includeSame || (dist>0.0)) {
                    const long roff = rgen.randlong();
                    const TNNRecR &worst = NN.front();
                    if ((dist == worst.dist) ? roff <= worst.randoff : dist < worst.dist) {
                        pop_heap(NN.begin(), NN.end());
                        NN.back() = TNNRecR(ei.index(), dist, roff);
                        push_heap(NN.begin(), NN.end());
                    }
                }
             }
        }
        sort_heap(NN.begin(), NN.end());
        reverse(NN.begin(), NN.end());
        return examplesFromHeap(NN, specDistanceID);
    }
    else {
        vector<TNNRecW> NN;
        double needs = k;
        PEITERATE(ei, examples) {
            if (!(reallyNeedsClass && isnan(ei->getClass()))) {
                const double dist = (*distance)(tex, *ei);
                if (includeSame || (dist>0.0)) {
                    const double wei = ei.getWeight();
                    needs -= wei;
                    NN.push_back(TNNRecW(ei.index(), dist, rgen.randlong(), wei));
                    push_heap(NN.begin(), NN.end());
                    if (needs + NN.back().weight <= 0) {
                        needs += NN.back().weight;
                        pop_heap(NN.begin(), NN.end());
                        NN.pop_back();
                    }
                }
            }
        }
        sort_heap(NN.begin(), NN.end());
        reverse(NN.begin(), NN.end());
        return examplesFromHeap(NN, specDistanceID);
    }
}


TFindNearestConstructor::TFindNearestConstructor(
    PExamplesDistanceConstructor const &edist,
    bool const is)
: distanceConstructor(edist),
includeSame(is)
{}


PFindNearest TFindNearestConstructor::operator()(PExampleTable const &gen,
                                                 TMetaId const distanceID) const
{
    PExamplesDistance edist;
    if (distanceConstructor) {
        edist = (*distanceConstructor)(gen);
    }
    else {
        PExamplesDistanceConstructor distconst(
            new TExamplesDistanceConstructor_Euclidean());
        edist = (*distconst)(gen);
    }
    return PFindNearest(
        new TFindNearest(gen, edist, distanceID, includeSame));
}


PyObject *TFindNearestConstructor::__call__(PyObject *args, PyObject *kw) const
{
    PExampleTable examples;
    int distanceID = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "O&|i:FindNearestConstructor", FindNearestConstructor_call_keywords,
        &PExampleTable::argconverter, &examples, &distanceID)) {
            return NULL;
    }
    return (*this)(examples, distanceID).getPyObject();
}


PyObject *TFindNearest::__call__(PyObject *args, PyObject *kw) const
{
    PyObject *pyexample;
    double k = -1;
    int needsClass = 0;
    int specDistanceID = distanceID;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "O|dii:FindNearest", FindNearest_call_keywords,
        &pyexample, &k, &needsClass, &specDistanceID)) {
            return NULL;
    }
    if (!examples) {
        raiseError(PyExc_ValueError, "example table is missing");
    }
    PExample example = TExample::fromDomainAndPyObject(
        examples->domain, pyexample, false);
    return (*this)(example.borrowPtr(), k, needsClass != 0, specDistanceID).getPyObject();
}
