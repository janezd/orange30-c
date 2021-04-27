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


#ifndef __CONTINGENCYATTRCLASS_HPP
#define __CONTINGENCYATTRCLASS_HPP

#include "contingencyattrclass.ppp"

class TContingencyAttrClass : public TContingencyClass {
public:
    __REGISTER_CLASS(ContingencyAttrClass);

    TContingencyAttrClass(PVariable const &attrVar, PVariable const &classVar);
    TContingencyAttrClass(PExampleTable const &, PVariable const &);
    TContingencyAttrClass(PExampleTable const &, TAttrIdx const);

    inline virtual PVariable const &getClassVar() const;
    inline virtual PVariable const &getAttribute() const;

    inline virtual void add_attrclass(TValue const varValue,
        TValue const classValue, double const p);

    inline virtual double p_class(TValue const varValue,
        TValue const classValue) const;

    inline virtual PDistribution p_classes(TValue const varValue) const;
protected:
    virtual void add_gen(PExampleTable const &gen, TAttrIdx const);
    virtual void add_gen(PExampleTable const &gen);
public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(feature, data); compute contingency for the given attribute and class");
    PyObject *py_p_class(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(value[, class_value]); return probability of class or a probability distribution");
};


PVariable const &TContingencyAttrClass::getClassVar() const
{ return innerVariable; }


PVariable const &TContingencyAttrClass::getAttribute() const
{ return outerVariable; }


void TContingencyAttrClass::add_attrclass(TValue const varValue,
                                          TValue const classValue, double const p)
{ 
    add(varValue, classValue, p);
}


double TContingencyAttrClass::p_class(TValue const varValue, TValue const classValue) const
{ 
    try {
        return p(varValue)->p(classValue); 
    }
    catch (PyException &exc) {
        if (exc.type == PyExc_IndexError) {
            return 0.0;
        }
        else {
            throw;
        }
    }
}

PDistribution TContingencyAttrClass::p_classes(TValue const varValue) const
{ 
    return p(varValue);
}


#endif
