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

#include "common.hpp"
#include "contingencyclass.px"

TContingencyClass::TContingencyClass(PVariable const &outer, PVariable const &inner)
: TContingency(outer, inner)
{}


double TContingencyClass::p_attr(TValue const, TValue const) const
{ 
    raiseError(PyExc_TypeError, "cannot compute p(value|class)"); 
    return 0.0;
}


double TContingencyClass::p_class(TValue const, TValue const) const
{
    raiseError(PyExc_TypeError, "cannot compute p(class|value)");
    return 0.0;
}

PDistribution TContingencyClass::p_attrs(TValue const) const
{ 
    raiseError(PyExc_TypeError, "cannot compute p(.|class)"); 
    return PDistribution();
}


PDistribution TContingencyClass::p_classes(TValue const) const
{ 
    raiseError(PyExc_TypeError, "cannot compute p(class|.)");
    return PDistribution();
}


void TContingencyClass::computeFromExampleTable(PVariable const &outer,
                                                PVariable const &inner,
                                                PExampleTable const &gen,
                                                TAttrIdx const attrNo)
{
    freeDistribution();
    init(outer, inner);
    if (attrNo == ILLEGAL_INT) {
        add_gen(gen);
    }
    else {
        add_gen(gen, attrNo);
    }
}


PyObject *TContingencyClass::py_add_var_class(PyObject *args, PyObject *kw)
{
    PyObject *pyvalue, *pyclass;
    double weight = 1.0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|d:add", ContingencyClass_add_var_class_keywords,
        &pyvalue, &pyclass, &weight)) {
            return NULL;
    }
    const TValue val = getAttribute()->py2val(pyvalue);
    const TValue cval = getClassVar()->py2val(pyclass);
    add_attrclass(val, cval, weight);
    Py_RETURN_NONE;
}


PyObject *TContingencyClass::__get__classVar(OrContingencyClass *self)
{
    return ((TContingencyClass &)self->orange).getClassVar().toPython();
}

PyObject *TContingencyClass::__get__variable(OrContingencyClass *self)
{
    return ((TContingencyClass &)self->orange).getAttribute().toPython();
}
