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


#ifndef __CONTINGENCY_HPP
#define __CONTINGENCY_HPP

#include "contingency.ppp"

/*! /file Base class for contingency matrices. */

typedef vector<PDistribution> TDistributionVector;
typedef map<double, PDistribution> TDistributionMap;

class TContingencyClass;

/*! /file Base class for contingency matrices. */
class TContingency : public TOrange {
public:
    friend class TContingencyClass;
    friend class TContingencyClassAttr;
    friend class TContingencyAttrClass;
    friend class TContingencyAttrAttr;

    __REGISTER_CLASS(Contingency);

    PVariable outerVariable; //PN outer variable
    PVariable innerVariable; //PN inner variable
    PDistribution outerDistribution; //PN marginal distribution for outer variable
    PDistribution innerDistribution; //PN marginal distribution for inner variable
    PDistribution innerDistributionUnknown; //PN condition distribution of inner variable for unknown outer value
    union {
        TDistributionVector *discrete;
        TDistributionMap *continuous;
    };
protected:
    explicit inline TContingency() {};
public:
    TContingency(PVariable const &variable, PVariable const &innervar);
    TContingency(TContingency const &);
    TContingency &operator=(TContingency const &);
    ~TContingency();

    void init(PVariable const &variable, PVariable const &innervar);
    void freeDistribution();
    int traverse_references(visitproc visit, void *arg);
    int clear_references();

    inline int varType() const;
    inline int outerVarType() const;
    inline int innerVarType() const;
    inline bool isDiscrete() const;
    inline ssize_t size() const;

    void add(TValue const outvalue, TValue const invalue, double const p=1);
    void normalize();
    inline PDistribution const &operator [](TValue const i) const;
    PDistribution &operator [](TValue const i);
    PDistribution p(TValue const i) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(outer_variable, inner_variable[, data]); construct an empty congintency for the given pair of variables");
    PyObject *py_add(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(outer_value, inner_value[, weight=1]); increase the contingency table cell by the give weight");
    PyObject *py_normalize() PYARGS(METH_NOARGS, "Normalize all distributions (rows) in the table to sum to 1");
    PyObject *__subscript__(PyObject *index);
    Py_ssize_t __len__() const;
    PyObject *__item__(Py_ssize_t i) const;
    PyObject *__getnewargs__(PyObject *args, PyObject *kw) const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    PyObject *keys() const PYARGS(METH_NOARGS, "(); return the values of the outer variable");
    PyObject *values() const PYARGS(METH_NOARGS, "(); return the distributions of the inner variable");
    PyObject *items() const PYARGS(METH_NOARGS, "(); return the values of the outer variable and the corresponding distributions of the inner");
};


int TContingency::varType() const
{ return outerVariable->varType; }

int TContingency::outerVarType() const
{ return outerVariable->varType; }

int TContingency::innerVarType() const
{ return innerVariable->varType; }

bool TContingency::isDiscrete() const
{ return outerVariable->varType == TVariable::Discrete; }

ssize_t TContingency::size() const
{ return isDiscrete() ? discrete->size() : continuous->size(); }

PDistribution const &TContingency::operator [](TValue const i) const
{ 
    if (isDiscrete()) {
        if (i>=int(discrete->size())) {
            raiseError(PyExc_IndexError, "index %i is out of range", int(i));
        }
        return (*discrete)[int(i)];
    }
    else {
        TDistributionMap::iterator mi = continuous->find(i);
        if (mi==continuous->end()) {
            raiseError(PyExc_KeyError, "value not found");
        }
        return (*mi).second;
    }
}

#endif
