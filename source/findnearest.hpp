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

#ifndef __FINDNEAREST_HPP
#define __FINDNEAREST_HPP

#include "findnearest.ppp"

class TFindNearest: public TOrange {
public:
    __REGISTER_CLASS(FindNearest);

    TMetaId distanceID; //P id of meta attribute where the distance should be stored (0 = no storing)
    bool includeSame; //P tells whether to include examples that are same as the reference example
    PExamplesDistance distance; //P metrics
    PExampleTable examples; //P a list of stored examples

    TFindNearest();
    TFindNearest(
        PExampleTable const &,
        PExamplesDistance const &,
        TMetaId const distId=0,
        bool const includeSame = true);

    virtual PExampleTable operator()(
        TExample const *const,
        double const k = 0.0,
        bool const needsClass=false,
        TMetaId const overrideDistanceID=ILLEGAL_INT) const;

    NEW_NOARGS;
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(example[, k, needs_class, distanceID]) -> table; return sorted table of k (or all) nearest examples");

    template<class THeap>
    PExampleTable examplesFromHeap(THeap const &, TMetaId const distanceID) const;
};

class TFindNearestConstructor : public TOrange {
public:
    __REGISTER_CLASS(FindNearestConstructor);

    PExamplesDistanceConstructor distanceConstructor; //P metrics
    bool includeSame; //P tells whether to include examples that are same as the reference example

    TFindNearestConstructor(
        PExamplesDistanceConstructor const & =PExamplesDistanceConstructor(),
        bool const includeSame = true);

    virtual PFindNearest operator()(
        PExampleTable const &,
        TMetaId const distanceID=0) const;

    NEW_WITH_CALL(FindNearestConstructor);
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(examples[, distanceID]) -> table; return an instance of FindNearest");
};

#define TFindNearest_BruteForce TFindNearest
#define TFindNearestConstructor_BruteForce TFindNearestConstructor

PYMODULECONSTANT(FindNearest_BruteForce, (PyObject *)(&OrFindNearest_Type))
PYMODULECONSTANT(FindNearestConstructor_BruteForce, (PyObject *)(&OrFindNearestConstructor_Type))

#endif
