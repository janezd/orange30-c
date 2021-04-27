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
#include "contingencyattrclass.px"


TContingencyAttrClass::TContingencyAttrClass(PVariable const &attrVar,
                                             PVariable const &classVar)
: TContingencyClass(attrVar, classVar)
{}


TContingencyAttrClass::TContingencyAttrClass(PExampleTable const &gen, 
                                             PVariable const &var)
{ 
    PDomain const &domain = gen->domain;
    if (!domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class");
    }
    TAttrIdx const attrNo = domain->getVarNum(var, false);
    computeFromExampleTable(var, domain->classVar, gen, attrNo);
}


TContingencyAttrClass::TContingencyAttrClass(PExampleTable const &gen,
                                             TAttrIdx const attrNo)
{
    PDomain const &domain = gen->domain;
    if (!domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class");
    }
    if (attrNo >= int(domain->attributes->size())) {
        raiseError(PyExc_IndexError, "index %i out of range", attrNo);
    }
    PVariable attribute = domain->getVar(attrNo, false);
    if (!attribute) {
        raiseError(PyExc_IndexError, "attribute %i not found", attrNo);
    }
    computeFromExampleTable(attribute, domain->classVar, gen, attrNo);
}


void TContingencyAttrClass::add_gen(PExampleTable const &gen)
{ 
    int attrNo = gen->domain->getVarNum(innerVariable, false);
    if (attrNo != ILLEGAL_INT) {
        PEITERATE(ei, gen) {
            add(ei.value_at(attrNo), ei.getClass(), ei.getWeight());
        }
    }
    else {
        if (!innerVariable->getValueFrom) {
            raiseError(PyExc_ValueError,
                "variable '%s' is not in the domain and cannot be computed "
                "from other variables", innerVariable->cname());
        }
        PEITERATE(ei, gen) {
            add(innerVariable->computeValue(*ei), ei.getClass(), ei.getWeight());
        }
    }
}


void TContingencyAttrClass::add_gen(PExampleTable const &gen,
                                    TAttrIdx const attrNo)
{ 
    PEITERATE(ei, gen) {
        add(ei.value_at(attrNo), ei.getClass(), ei.getWeight());
    }
}


char *ContingencyAttrClass_keywords2[] = {"feature", "class_variable", NULL};

TOrange *TContingencyAttrClass::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    // unpickling
    if (args && (PyTuple_Size(args) == 3) && (PyList_Check(PyTuple_GET_ITEM(args, 2)))) {
        return TContingency::__new__(type, args, kw);
    }

    PVariable var, klass;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&O&:ContingencyAttrClass",
             ContingencyAttrClass_keywords2,
             &PVariable::argconverter, &var, &PVariable::argconverter, &klass)) {
         return new(type) TContingencyAttrClass(var, klass);
    }
    PyErr_Clear();

    PExampleTable data;
    PyObject *pyvar;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O&:Contingency",
        ContingencyAttrClass_keywords,
        &pyvar, &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    var = TDomain::varFromArg_byDomain(pyvar, data->domain, false);
    return new(type) TContingencyAttrClass(data, var);
}


PyObject *TContingencyAttrClass::py_p_class(PyObject *args, PyObject *kw) const
{
    PyObject *pyvalue, *pyclass=NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O",
        ContingencyAttrClass_p_class_keywords, &pyvalue, &pyclass)) {
            return NULL;
    }
    const TValue val = getAttribute()->py2val(pyvalue);
    if (!pyclass) {
        return p_classes(val).toPython();
    }
    const TValue cval = getClassVar()->py2val(pyclass);
    return PyFloat_FromDouble(p_class(val, cval));
}
