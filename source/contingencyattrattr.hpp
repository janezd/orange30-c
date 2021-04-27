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


#ifndef __CONTINGENCYATTRATTR_HPP
#define __CONTINGENCYATTRATTR_HPP

#include "contingencyattrattr.ppp"
#include "contingency.hpp"

class TContingencyAttrAttr : public TContingency { 
public:
    __REGISTER_CLASS(ContingencyAttrAttr);

    TContingencyAttrAttr(PVariable const &, PVariable const &);
    TContingencyAttrAttr(PVariable const &, PVariable const &, PExampleTable const &);
    TContingencyAttrAttr(TAttrIdx const, TAttrIdx const, PExampleTable const &);
    void computeFromExampleTable(PExampleTable const &);

    inline virtual double p_attr(TValue const, TValue const) const;
    inline virtual PDistribution p_attrs(TValue const outer) const;
public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(feature, data); compute contingency for the given attribute and class");
    PyObject *py_p_attr(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "([value, ]class_value); return probability of feature value or a probability distribution");
};


double TContingencyAttrAttr::p_attr(TValue const outerValue, TValue const innerValue) const
{ 
    return p(outerValue)->p(innerValue);
}


PDistribution TContingencyAttrAttr::p_attrs(TValue const outerValue) const
{ 
    return p(outerValue);
}


#endif
