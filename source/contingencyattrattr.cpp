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
#include "contingencyattrattr.px"


TContingencyAttrAttr::TContingencyAttrAttr(PVariable const &variable,
                                           PVariable const &innervar)
: TContingency(variable, innervar)
{}


TContingencyAttrAttr::TContingencyAttrAttr(PVariable const &variable,
                                           PVariable const &innervar,
                                           PExampleTable const &gen)
: TContingency(variable, innervar)
{ 
    if (gen) {
        computeFromExampleTable(gen);
    }
}


TContingencyAttrAttr::TContingencyAttrAttr(TAttrIdx const var,
                                           TAttrIdx const innervar,
                                           PExampleTable const &gen)
: TContingency(gen->domain->getVar(var), gen->domain->getVar(innervar))
{ 
    computeFromExampleTable(gen);
}


void TContingencyAttrAttr::computeFromExampleTable(PExampleTable const &gen)
{
    int var = gen->domain->getVarNum(outerVariable, false);
    int invar = gen->domain->getVarNum(innerVariable, false);
    if (var == ILLEGAL_INT) {
        if (invar == ILLEGAL_INT) {
            PEITERATE(ei, gen) {
                TValue const val = outerVariable->computeValue(*ei);
                TValue const inval = innerVariable->computeValue(*ei);
                add(val, inval, ei.getWeight());
            }
        }
        else { // var == ILLEGAL_INT, invar is not
            PEITERATE(ei, gen) {
                TValue const val = outerVariable->computeValue(*ei);
                TValue const inval = ei.value_at(invar);
                add(val, inval, ei.getWeight());
            }
        }
    }
    else {
        if (invar == ILLEGAL_INT) {// invar == ILLEGAL_INT, var is not
            PEITERATE(ei, gen) {
                TValue const val = ei.value_at(var);
                TValue const inval = innerVariable->computeValue(*ei);
                add(val, inval, ei.getWeight());
            }
        }
        else {// both OK
            PEITERATE(ei, gen) {
                add(ei.value_at(var), ei.value_at(invar), ei.getWeight());
            }
        }
    }
}


TOrange *TContingencyAttrAttr::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    // unpickling
    if (args && (PyTuple_Size(args) == 3) && (PyList_Check(PyTuple_GET_ITEM(args, 2)))) {
        return TContingency::__new__(type, args, kw);
    }

    PyObject *pyvar1, *pyvar2;
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|O&:ContingencyAttrAttr",
             ContingencyAttrAttr_keywords, &pyvar1, &pyvar2,
             &PExampleTable::argconverter, &data)) {
                 return NULL;
    }
    if (data) {
        PDomain const &domain = data->domain;
        PVariable var1 = TDomain::varFromArg_byDomain(pyvar1, domain, false);
        PVariable var2 = TDomain::varFromArg_byDomain(pyvar2, domain, false);
        return new(type) TContingencyAttrAttr(var1, var2, data);
    }
    if (!OrVariable_Check(pyvar1)) {
        raiseError(PyExc_TypeError, "first argument should be a Variable, not '%s'",
            pyvar1->ob_type->tp_name);
    }
    if (!OrVariable_Check(pyvar2)) {
        raiseError(PyExc_TypeError, "second argument should be a Variable, not '%s'",
            pyvar2->ob_type->tp_name);
    }
    return new(type) TContingencyAttrAttr(PVariable(pyvar1), PVariable(pyvar2));
}


PyObject *TContingencyAttrAttr::py_p_attr(PyObject *args, PyObject *kw) const
{
    if (args && (PyTuple_Size(args)==1) && (!kw || !PyDict_Size(kw))) {
        PyObject *pyclass = PyTuple_GET_ITEM(args, 0);
        TValue const clsv = outerVariable->py2val(pyclass);
        return p_attrs(clsv).toPython();
    }

    PyObject *pyvalue, *pyclass;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO",
        ContingencyAttrAttr_p_attr_keywords, &pyvalue, &pyclass)) {
            return NULL;
    }
    TValue const val = outerVariable->py2val(pyvalue);
    TValue const cval = innerVariable->py2val(pyclass);
    return PyFloat_FromDouble(p_attr(val, cval));
}
