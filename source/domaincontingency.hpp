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


#ifndef __DOMAINCONTINGENCY_HPP
#define __DOMAINCONTINGENCY_HPP

#include "domaincontingency.ppp"
#include "contingency.hpp"

class TDomainContingency : public TOrange {
public:
    VECTOR_INTERFACE(PContingencyClass, contingencies)
    __REGISTER_CLASS(DomainContingency)

    PDistribution classes; //P distribution of class values
    bool classIsOuter; //P tells whether the class is the outer variable
  
    TDomainContingency(bool const acout=false);

    TDomainContingency(
        PExampleTable const &, bool const classOut=false);

    TDomainContingency(
        PExampleTable const &,
        vector<bool> const &, bool const classOut=false);

    virtual void computeFromExampleTable(
        PExampleTable const &,
        vector<bool> const * =NULL, PDomain const &newDomain=PDomain());

    void normalize();

    int traverse_references(visitproc visit, void *arg);
    int clear_references();

    TAttrIdx getItemIndex(PyObject *index) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(data[, class_outer, skip_discrete=False, skip_continuous=False]); construct contingencies from data");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    Py_ssize_t __len__() const;
    PyObject *__item__(Py_ssize_t index) const;
    PyObject *__subscript__(PyObject *index) const;
    PyObject *py_normalize() PYARGS(METH_NOARGS, "(); normalizes all distribution within all contingencies");
};

#endif
