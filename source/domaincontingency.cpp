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
#include "contingency.hpp"
#include "contingencyclass.hpp"
#include "contingencyattrclass.hpp"
#include "contingencyclassattr.hpp"
#include "domaincontingency.px"

TDomainContingency::TDomainContingency(bool const acout)
: classIsOuter(acout)
{}


// Extract TContingency values for all attributes, by iterating through all examples from the generator
TDomainContingency::TDomainContingency(PExampleTable const &gen,
                                       bool const acout)
: classIsOuter(acout)
{ 
    computeFromExampleTable(gen);
}


// Extract TContingency values for all attributes, by iterating through all examples from the generator
TDomainContingency::TDomainContingency(PExampleTable const &gen,
                                       vector<bool> const &attributes,
                                       bool const acout)
: classIsOuter(acout)
{ 
    computeFromExampleTable(gen, &attributes);
}


int TDomainContingency::traverse_references(visitproc visit, void *arg)
{
    this_ITERATE(di) {
        if (*di) {
            Py_VISIT(di->borrowPyObject());
        }
    }
    return TOrange::traverse_references(visit, arg);
}


int TDomainContingency::clear_references()
{ 
    clear();
    return TOrange::clear_references();
}


void TDomainContingency::computeFromExampleTable(PExampleTable const &gen,
                                                 vector<bool> const *attributes,
                                                 PDomain const &newDomain)
{ 
    PDomain myDomain = newDomain ? newDomain : gen->domain;
    PVariable classVar = myDomain->classVar;
    if (!classVar) {
        raiseError(PyExc_ValueError, "domain has no class");
    }
    classes = TDistribution::create(classVar);
    bool classDiscrete = classVar->varType == TVariable::Discrete;
    double nClasses_1 = double(classVar->noOfValues()-1);
    vector<bool>::const_iterator ai, ae;
    if (attributes) {
        ai = attributes->begin();
        ae = attributes->end();
    }
    PITERATE(TVarList, vli, myDomain->attributes) {
        if (attributes) {
            if (ai == ae) {
                break;
            }
            if (!*ai++) {
                push_back(PContingencyClass());
                continue;
            }
        }
        TContingencyClass *cont;
        if (classIsOuter) {
           cont = new TContingencyClassAttr(*vli, classVar);
        }
        else {
            cont = new TContingencyAttrClass(*vli, classVar);
        }
        push_back(PContingencyClass(cont));
        if ((nClasses_1 > 0) && ((*vli)->varType==TVariable::Discrete)) {
            for(int i=0, e=(*vli)->noOfValues(); i!=e; i++) {
                cont->add_attrclass(TValue(i), nClasses_1, 0);
            }
        }
    }
    PExample const newExample = newDomain ? TExample::constructFree(newDomain) : PExample();
    PEITERATE(fi, gen) {
        TExample::iterator vi;
        TValue cls;
        if (newDomain) {
			newDomain->convert(newExample.borrowPtr(), *fi, true);
            vi = newExample->begin();
            cls = newExample->getClass();
        }
        else {
            vi = fi->begin();
            cls = fi.getClass();
        }
        const double weight = fi.getWeight();
        classes->add(cls, weight);
        for(iterator si(begin()), se(end()); si!=se; vi++, si++) {
            if (*si) {
                (*si)->add_attrclass(*vi, cls, weight);
            }
        }
    }
}


void TDomainContingency::normalize()
{ 
    classes->normalize();
    this_ITERATE(ti) {
        (*ti)->normalize();
    }
}



TOrange *TDomainContingency::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_GET_SIZE(args) == 1) && (PyList_Check(PyTuple_GET_ITEM(args, 0)))) {
        PDomainContingency me(new(type) TDomainContingency());
        if (!pyListToVector<PContingencyClass, &OrContingencyClass_Type>(
            PyTuple_GET_ITEM(args, 0), me->contingencies, true)) {
                return NULL;
        }
        return me.getPtr();
    }
    PExampleTable table;
    int skipDiscrete = 0, skipContinuous = 0, classOuter=0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|iii:DomainDistributions",
        DomainContingency_keywords,
        &PExampleTable::argconverter, &table, &skipDiscrete, &skipContinuous)) {
            return NULL;
    }
    if (skipDiscrete || skipContinuous) {
        PVarList attributes = table->domain->attributes;
        vector<bool> takeattrs;
        takeattrs.reserve(attributes->size());
        PITERATE(TVarList, ai, attributes) {
            takeattrs.push_back((*ai)->varType == TVariable::Discrete ?
                !skipDiscrete : !skipContinuous);
        }
        return new(type) TDomainContingency(table, takeattrs, classOuter != 0);
    }
    else {
        return new(type) TDomainContingency(table, classOuter != 0);
    }
}


PyObject *TDomainContingency::__getnewargs__() const
{
    return PyTuple_Pack(1, vectorToPyList(contingencies));
}


Py_ssize_t TDomainContingency::__len__() const
{
    return size();
}


PyObject *TDomainContingency::__item__(Py_ssize_t index) const
{
    if ((index < 0) || (index >= size())) {
        raiseError(PyExc_IndexError, "index %i out of range", index);
    }
    return (*this)[index].toPython();
}


int TDomainContingency::getItemIndex(PyObject *index) const
{ 
    if (PyLong_Check(index)) {
        int i = (int)PyLong_AsLong(index);
        if (i < 0) {
            i += size();
        }
        if ((i < 0) || (i >= size())) {
            raiseError(PyExc_IndexError, "index %i out of range", i);
        }
        return i;
    }
    if (PyUnicode_Check(index)) {
        string s = PyUnicode_As_string(index);
        const_this_ITERATE(ci) {
            if (*ci &&
                (*ci)->outerVariable &&
                ((*ci)->outerVariable->getName() == s)) {
                    return ci - begin();
            }
        }
        raiseError(PyExc_IndexError,
            "attribute '%s' not found in domain", s.c_str());
    }
    if (OrVariable_Check(index)) {
        PVariable var(index);
        const_this_ITERATE(ci) {
            if (*ci && (*ci)->outerVariable && ((*ci)->outerVariable == var)) {
                return ci - begin();
            }
        }
        raiseError(PyExc_IndexError,
            "attribute '%s' not found in domain", var->cname());
    }
    raiseError(PyExc_IndexError,
        "DomainContingency can be indexed by int, str or Variable, not '%s'",
        index->ob_type->tp_name);
    return 0;
}


PyObject *TDomainContingency::__subscript__(PyObject *index) const
{
    int pos = getItemIndex(index);
    return (*this)[pos].toPython();
}


PyObject *TDomainContingency::py_normalize()
{
    normalize();
    Py_RETURN_NONE;
}
