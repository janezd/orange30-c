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


#ifndef __CONTINGENCYCLASS_HPP
#define __CONTINGENCYCLASS_HPP

#include "contingencyclass.ppp"
#include "contingency.hpp"

class TContingencyClass : public TContingency {
public:
    __REGISTER_ABSTRACT_CLASS(ContingencyClass);
protected:
    explicit inline TContingencyClass() {};
public:
    TContingencyClass(
        PVariable const &outer, PVariable const &inner);

    void computeFromExampleTable(
        PVariable const &outer, PVariable const &inner,
        PExampleTable const &, TAttrIdx const);

    virtual PVariable const &getClassVar() const =0;
    virtual PVariable const &getAttribute() const =0;

    virtual void add_attrclass(TValue const varValue, TValue const classValue,
        double const p) = 0;
    virtual double p_class(TValue const varValue, TValue const classValue) const;
    virtual double p_attr(TValue const varValue, TValue const classValue) const;
    virtual PDistribution p_classes(TValue const varValue) const;
    virtual PDistribution p_attrs(TValue const classValue) const;
protected:
    virtual void add_gen(PExampleTable const &gen, TAttrIdx const) = 0;
    virtual void add_gen(PExampleTable const &gen) = 0;
public:
    PyObject *py_add_var_class(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(variable_value, class_value[, weight=1]); increase the contingency table cell by the give weight");
    static PyObject *__get__classVar(OrContingencyClass *self);
    static PyObject *__get__variable(OrContingencyClass *self);
};

#endif
