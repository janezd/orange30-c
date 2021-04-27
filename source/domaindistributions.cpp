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
#include "domaindistributions.px"


TDomainDistributions::TDomainDistributions()
{}


TDomainDistributions::TDomainDistributions(PExampleTable const &gen,
                                           bool const skipDiscrete, 
                                           bool const skipContinuous)
{
    reserve(gen->domain->variables->size());
    PITERATE(TVarList, vi, gen->domain->variables) {
        if (((*vi)->varType == TVariable::Discrete ? skipDiscrete : skipContinuous)) {
            push_back(PDistribution());
        }
        else {
            push_back(TDistribution::create(*vi));
        }
    }
    PEITERATE(fi, gen) {
        TExample::iterator ei = fi->begin();
        const double weight = fi.getWeight();
        this_ITERATE(di) {
            if (*di) {
                (*di)->add(*ei, weight);
            }
            ei++;
        }
    }
}


TDomainDistributions::TDomainDistributions(TDomainDistributions const &other)
: distributions(other.distributions)
{}


void TDomainDistributions::normalize()
{
    this_ITERATE(di) {
        (*di)->normalize(); 
    }
}


int TDomainDistributions::traverse_references(visitproc visit, void *arg)
{
    this_ITERATE(di) {
        if (*di) {
            Py_VISIT(di->borrowPyObject());
        }
    }
    return TOrange::traverse_references(visit, arg);
}


int TDomainDistributions::clear_references()
{ 
    clear();
    return TOrange::clear_references();
}


TOrange *TDomainDistributions::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_GET_SIZE(args) == 1) &&
        PyList_Check(PyTuple_GET_ITEM(args, 0))) {
            PDomainDistributions me(new(type) TDomainDistributions());
            if (!pyListToVector<PDistribution, &OrDistribution_Type>
                (PyTuple_GET_ITEM(args, 0), me->distributions, true)) {
                    return NULL;
            }
            return me.getPtr();
    }
    PExampleTable table;
    int skipDiscrete = 0, skipContinuous = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|ii:DomainDistributions",
        DomainDistributions_keywords, &PExampleTable::argconverter, &table,
        &skipDiscrete, &skipContinuous)) {
            return NULL;
    }
    return new(type) TDomainDistributions(
        table, skipDiscrete!=0, skipContinuous!=0);
}


PyObject *TDomainDistributions::__getnewargs__() const
{
    return Py_BuildValue("(N)", vectorToPyList(distributions));
}


Py_ssize_t TDomainDistributions::__len__() const
{
    return size();
}


PyObject *TDomainDistributions::__item__(Py_ssize_t index) const
{
    if ((index < 0) || (index >= size())) {
        raiseError(PyExc_IndexError, "index %i out of range", index);
    }
    return (*this)[index].toPython();
}


TAttrIdx TDomainDistributions::getItemIndex(PyObject *index) const
{ 
    if (PyLong_Check(index)) {
        TAttrIdx i = PyLong_AsLong(index);
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
            if (*ci && (*ci)->variable && ((*ci)->variable->getName() == s)) {
                return ci - begin();
            }
        }
        raiseError(PyExc_IndexError,
            "attribute '%s' not found in domain", s.c_str());
    }
    if (OrVariable_Check(index)) {
        PVariable var(index);
        const_this_ITERATE(ci) {
            if (*ci && (*ci)->variable && ((*ci)->variable == var)) {
                return ci - begin();
            }
        }
        raiseError(PyExc_IndexError,
            "attribute '%s' not found in domain", var->cname());
    }
    raiseError(PyExc_IndexError,
        "DomainDistribution can be indexed by int, str or Variable, not '%s'",
        index->ob_type->tp_name);
    return 0;
}


PyObject *TDomainDistributions::__subscript__(PyObject *index) const
{
    int pos = getItemIndex(index);
    return (*this)[pos].toPython();
}
