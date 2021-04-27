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

#ifndef __DOMAINDISTRIBUTIONS_HPP
#define __DOMAINDISTRIBUTIONS_HPP

#include "domaindistributions.ppp"

class TDomainDistributions : public TOrange {
public:
    VECTOR_INTERFACE(PDistribution, distributions)

    __REGISTER_CLASS(DomainDistributions);

    TDomainDistributions();
    TDomainDistributions(TDomainDistributions const &);
    TDomainDistributions(PExampleTable const &,
        bool const skipDiscrete = false, bool const skipContinuous = false);

    void normalize();

    int traverse_references(visitproc visit, void *arg);
    int clear_references();

    TAttrIdx getItemIndex(PyObject *index) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(data[, skip_discrete=False, skip_continuous=False]); construct distributions from data");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    Py_ssize_t __len__() const;
    PyObject *__item__(Py_ssize_t index) const;
    PyObject *__subscript__(PyObject *index) const;
};

#endif
