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


#ifndef __CONTINGENCYCLASSATTR_HPP
#define __CONTINGENCYCLASSATTR_HPP

#include "contingencyclassattr.ppp"
#include "contingencyclass.hpp"

class TContingencyClassAttr : public TContingencyClass {
public:
    __REGISTER_CLASS(ContingencyClassAttr);

    TContingencyClassAttr(PVariable const &outer, PVariable const &inner);
    TContingencyClassAttr(PExampleTable const &, TAttrIdx const);
    TContingencyClassAttr(PExampleTable const &, PVariable const &);

    inline virtual PVariable const &getClassVar() const;
    inline virtual PVariable const &getAttribute() const;

    inline virtual void add_attrclass(TValue const varValue,
        TValue const classValue, double const p);
    inline virtual double p_attr(TValue const varValue, TValue const classValue) const;
    inline virtual PDistribution p_attrs(TValue const classValue) const;
protected:
    virtual void add_gen(PExampleTable const &gen, TAttrIdx const);
    virtual void add_gen(PExampleTable const &gen);
public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(feature, data); compute contingency for the given attribute and class");
    PyObject *py_p_attr(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "([value, ]class_value); return probability of feature value or a probability distribution");
};

PVariable const &TContingencyClassAttr::getClassVar() const
{ return outerVariable; }

PVariable const &TContingencyClassAttr::getAttribute() const
{ return innerVariable; }

void TContingencyClassAttr::add_attrclass(TValue const varValue, TValue const classValue,
                                          const double p)
{ 
    add(classValue, varValue, p);
}

double TContingencyClassAttr::p_attr(TValue const varValue, TValue const classValue) const
{ 
    return p(classValue)->p(varValue);
}

PDistribution TContingencyClassAttr::p_attrs(TValue const classValue) const
{ 
    return p(classValue);
}

#endif
