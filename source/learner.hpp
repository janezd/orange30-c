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


#ifndef __LEARNER_HPP
#define __LEARNER_HPP

#include "learner.ppp"

class TExampleTable;
class TClassifier;

/*! Abstract base class for classes that induce classifiers from examples.

    The class has a call operator that accepts the data and returns a
    classifier. Examples can have weights (see #TExampleTable::weights); the
    weights will be used in learning if they are present and the learner
    supports them.

    Prior to version 3.0, the operator had an additional argument for
    specifying the id of meta attribute with weight. With the new mechanism
    for storing weights this is no longer needed.
*/

class TLearner : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Learner);
    /*! Pure virtual class for induction of classifiers from data. */
    virtual PClassifier operator()(PExampleTable const &) =0;

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(examples); induces a classifier from examples");

    PyTypeObject *getReturnType(PyTypeObject *defaultType) const;
};

#endif

