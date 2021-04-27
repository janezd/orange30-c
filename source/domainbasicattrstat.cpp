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
#include "domainbasicattrstat.px"
#include <functional>


TDomainBasicAttrStat::TDomainBasicAttrStat()
: hasClassVar(false)
{}


TDomainBasicAttrStat::TDomainBasicAttrStat(PExampleTable const &examples)
: hasClassVar(examples->domain->classVar)
{ 
    PITERATE(TVarList, vi, examples->domain->variables) {
        PBasicAttrStat stat((*vi)->varType==TVariable::Continuous
            ? new TBasicAttrStat(PContinuousVariable(*vi), true) : NULL);
        push_back(stat);
    }
    PEITERATE(fi, examples) {
        TExample::iterator ei = fi->begin();
        float wei = fi.getWeight();
        for(iterator di(begin()); di!=end(); di++, ei++)
            if (*di && !isnan(*ei))
                (*di)->add(*ei, wei);
    }
    this_ITERATE(di) {
        if (*di) {
            (*di)->holdRecomputation = false;
            (*di)->recompute();
        }
    }
}


int TDomainBasicAttrStat::traverse_references(visitproc visit, void *arg)
{
    this_ITERATE(di) {
        if (*di) {
            Py_VISIT((*di).borrowPyObject());
        }
    }
    return TOrange::traverse_references(visit, arg);
}


int TDomainBasicAttrStat::clear_references()
{ 
    clear();
    return TOrange::clear_references();
}


void TDomainBasicAttrStat::purge()
{ 
    erase(remove_if(begin(), end(), logical_not<PBasicAttrStat>()), end());
}


TOrange *TDomainBasicAttrStat::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_GET_SIZE(args) == 1) &&
        PyList_Check(PyTuple_GET_ITEM(args, 0))) {
        PDomainBasicAttrStat me(new(type) TDomainBasicAttrStat());
        if (!pyListToVector<PBasicAttrStat, &OrBasicAttrStat_Type>(
                             PyTuple_GET_ITEM(args, 0), me->stats, true)) {
            return NULL;
        }
        return me.getPtr();
    }
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:DomainBasicAttrStat",
        DomainBasicAttrStat_keywords, &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    return new(type) TDomainBasicAttrStat(data);
}


PyObject *TDomainBasicAttrStat::__getnewargs__() const
{
    return Py_BuildValue("(N)", vectorToPyList(stats));
}


Py_ssize_t TDomainBasicAttrStat::__len__() const
{
    return size();
}


PyObject *TDomainBasicAttrStat::__item__(Py_ssize_t index) const
{
    if ((index < 0) || (index >= size())) {
        raiseError(PyExc_IndexError, "index %i out of range", index);
    }
    return (*this)[index].toPython();
}


TAttrIdx TDomainBasicAttrStat::getItemIndex(PyObject *index) const
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
        raiseError(PyExc_IndexError, "data has no attribute '%s'", s.c_str());
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
        "DomainBasicAttrStat can be indexed by int, str or Variable, not '%s'",
        index->ob_type->tp_name);
    return 0;
}


PyObject *TDomainBasicAttrStat::__subscript__(PyObject *index) const
{
    int pos = getItemIndex(index);
    return (*this)[pos].toPython();
}


PyObject *TDomainBasicAttrStat::py_purge()
{
    purge();
    Py_RETURN_NONE;
}
