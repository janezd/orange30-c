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
#include "contingency.px"

void TContingency::init(PVariable const &var, PVariable const &innervar)
{ 
    if (!var->isPrimitive()) {
        raiseError(PyExc_TypeError,
            "'%s' is not a primitive variable", var->cname());
    }
    if (!innervar->isPrimitive()) {
        raiseError(PyExc_TypeError,
            "'%s' is not a primitive variable", innervar->cname());
    }
    outerVariable = var;
    innerVariable = innervar;
    outerDistribution = TDistribution::create(var);
    innerDistribution = TDistribution::create(innervar);
    innerDistributionUnknown = TDistribution::create(innervar);
    if (isDiscrete()) {
        discrete = new TDistributionVector();
        int i = outerVariable->noOfValues();
        discrete->reserve(i);
        while(i--) {
            discrete->push_back(TDistribution::create(innervar));
        }
    }
    else {
        continuous = new TDistributionMap();
    }
}

TContingency::TContingency(PVariable const &var, PVariable const &innervar)
{
    init(var, innervar);
}

TContingency::TContingency(TContingency const &old)
{
// Usually a bad practice but, hey, they are the same and there's quite some code!
    *this = old; 
}


TContingency &TContingency::operator =(const TContingency &old)
{ 
    if (this == &old) {
        return *this;
    }
    outerVariable = old.outerVariable;
    innerVariable = old.innerVariable;
    outerDistribution = CLONE(PDistribution, old.outerDistribution);
    innerDistribution = CLONE(PDistribution, old.innerDistribution);
    innerDistributionUnknown = CLONE(PDistribution, old.innerDistributionUnknown);
    if (isDiscrete()) {
        discrete = new TDistributionVector();
        PITERATE(TDistributionVector, di, old.discrete) {
            discrete->push_back(CLONE(PDistribution, *di));
        }
    }
    else {
        continuous = new TDistributionMap();
        PITERATE(TDistributionMap, di, old.continuous) {
            continuous->insert(continuous->end(),
                make_pair(di->first, CLONE(PDistribution, di->second)));
        }
    }
    return *this;
}


void TContingency::freeDistribution()
{
    if (!outerVariable) { // not initialized yet
        return;
    }
    if (isDiscrete()) {
        delete discrete;
    }
    else {
        delete continuous;
    }
}

TContingency::~TContingency()
{ 
    freeDistribution();
}


int TContingency::traverse_references(visitproc visit, void *arg)
{ 
    if (isDiscrete()) {
        PITERATE(TDistributionVector, di, discrete) {
            if (*di) {
                Py_VISIT(di->borrowPyObject());
            }
        }
    }
    else {
        PITERATE(TDistributionMap, di, continuous) {
            if (di->second) {
                Py_VISIT(di->second.borrowPyObject());
            }
        }
    }
    return TOrange::traverse_references(visit, arg);
}


int TContingency::clear_references()
{ 
    freeDistribution();
    return TOrange::clear_references();
}


PDistribution &TContingency::operator [](TValue const i)
{ 
    if (isDiscrete()) {
        const int di = int(i);
        while (discrete->size() <= di) {
            PDistribution nd = TDistribution::create(innerVariable);
            if (innerVarType() == TVariable::Discrete) {
                nd->add(innerVariable->noOfValues()-1, 0);
            }
            discrete->push_back(nd);
        }
        return (*discrete)[di];
    }
    else {
        TDistributionMap::iterator mi=continuous->lower_bound(i);
        if ((mi == continuous->end()) || (mi->first != i)) {
            PDistribution ret = TDistribution::create(innerVariable);
            if (innerVarType() == TVariable::Discrete) {
                ret->add(innerVariable->noOfValues()-1, 0);
            }
            mi = continuous->insert(mi, make_pair(i, ret));
        }
        return mi->second;
    }
}


void TContingency::add(const TValue outvalue, const TValue invalue, const double p)
{
    outerDistribution->add(outvalue, p);
    if (isnan(outvalue)) {
        innerDistributionUnknown->add(invalue, p);
    }
    else {
        innerDistribution->add(invalue, p);
        (*this)[outvalue]->add(invalue, p);
    }
}

       
PDistribution TContingency::p(TValue const f) const
{
    if (isDiscrete() ? f >= discrete->size() : !continuous->size()) {
        return TDistribution::create(innerVariable);
    }
    if (isDiscrete()) {
        return CLONE(PDistribution, (*this)[f]);
    }

    TDistributionMap::const_iterator i1;
    i1 = continuous->end();
    if (f > (*--i1).first) {
        return CLONE(PDistribution, i1->second);
    }
    i1 = continuous->lower_bound(f);
    if (((*i1).first == f) || (i1==continuous->begin())) {
        return CLONE(PDistribution, i1->second);
    }
    TDistributionMap::const_iterator i2 = i1;
    i1--;
    const double &x1 = (*i1).first;
    const double &x2 = (*i2).first;
    const double r = (f-x1) / (x2-x1);
    // We want to compute y1*(1-r) + y2*r
    // We know that r!=0, so we can compute (y1*(1-r)/r + y2) * r
    PDistribution res = CLONE(PDistribution, i1->second);
    *res *= (1-r)/r;
    *res += *(*i2).second;
    *res *= r;
    return res;
}


void TContingency::normalize()
{ 
    if (isDiscrete()) {
        PITERATE(TDistributionVector, ci, discrete) {
            (*ci)->normalize();
        }
    }
    else {
        PITERATE(TDistributionMap, ci, continuous) {
            (*ci).second->normalize();
        }
    }
}


char *Contingency_keywords3[] = {"variable", "data", NULL};
char *Contingency_keywords_unpickle[] = {"outer_variable", "inner_variable", "data", NULL};

TOrange *TContingency::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
If (optional) data is provided, the constructor will construct an instance
of either `ContingencyAttrAttr` or `ContingencyAttrClass` (if the second
variable) is the class variable of the data).

Another call signature is (variable, data), which constructs an
instance of `ContingencyAttrClass` for the given variable.
*/
{
    PyObject *pyvar1, *pyvar2;
    PExampleTable data;
    if ((   (type == &OrContingency_Type)
          || PyType_IsSubtype(type, &OrContingencyAttrClass_Type))
        && PyArg_ParseTupleAndKeywords(args, kw, "OO&:Contingency",
             Contingency_keywords3,
             &pyvar1, &PExampleTable::argconverter, &data)) {
         PVariable var1 = TDomain::varFromArg_byDomain(
             pyvar1, data->domain, false);
         if (type == &OrContingency_Type) {
             type = &OrContingencyAttrClass_Type;
         }
         return new(type) TContingencyAttrClass(data, var1);
    }
    PyErr_Clear();

    PyObject *listdata;
    if(PyArg_ParseTupleAndKeywords(args, kw, "O!O!O!:Contingency",
        Contingency_keywords_unpickle, &OrVariable_Type, &pyvar1,
        &OrVariable_Type, &pyvar2, &PyList_Type, &listdata)) {
            PyObject *vars = PyType_IsSubtype(type, &OrContingencyClassAttr_Type)
                ? PyTuple_Pack(2, pyvar2, pyvar1)
                : PyTuple_Pack(2, pyvar1, pyvar2);
            GUARD(vars);
            PyObject *pycont = type->tp_new(type, vars, NULL);
            if (!pycont) {
                return NULL;
            }
            GUARD(pycont);
            if (!OrContingency_Check(pycont)) {
                raiseError(PyExc_SystemError,
                    "Contingency constructor did not construct a contingency");
            }
            PContingency me = PContingency(pycont);
            if (me->isDiscrete()) {
                me->freeDistribution();
                me->discrete = new TDistributionVector();
                if (!pyListToVector<PDistribution, &OrDistribution_Type>(listdata, *me->discrete, false)) {
                    return NULL;
                }
            }
            else {
                me->freeDistribution();
                me->continuous = new TDistributionMap();
                for(Py_ssize_t i = 0, e = PyList_Size(listdata); i != e; i++) {
                    double d;
                    PDistribution dist;
                    if (!PyArg_ParseTuple(PyList_GET_ITEM(listdata, i), "dO&", 
                        &d, &PDistribution::argconverter, &dist)) {
                            return NULL;
                    }
                    (*me->continuous)[d] = dist;
                }
            }
            return me.getPtr(); 
    }
    PyErr_Clear();

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|O&:Contingency",
        Contingency_keywords,
        &pyvar1, &pyvar2, &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    if (data) {
        PDomain domain = data->domain;
        PVariable var1 = TDomain::varFromArg_byDomain(pyvar1, domain, false);
        PVariable var2 = TDomain::varFromArg_byDomain(pyvar2, domain, false);
        if (var2 == domain->classVar) {
            if (type == &OrContingency_Type) {
                type = &OrContingencyAttrClass_Type;
            }
            if (PyType_IsSubtype(type, &OrContingencyAttrClass_Type)) {
                return new(type) TContingencyAttrClass(data, var1);
            }
        }
        else if (var1 == domain->classVar) {
            if (PyType_IsSubtype(type, &OrContingencyClassAttr_Type)) {
                return new(type) TContingencyClassAttr(data, var2);
            }
        }
        if (type == &OrContingency_Type) {
            type = &OrContingencyAttrAttr_Type;
        }
        if (PyType_IsSubtype(type, &OrContingencyAttrAttr_Type)) {
            return new(type) TContingencyAttrAttr(var1, var2, data);
        }
        else {
            raiseError(PyExc_TypeError,
                "cannot construct an instance of '%s' using the provided arguments",
                type->tp_name);
        }
    }
    if (!OrVariable_Check(pyvar1)) {
        raiseError(PyExc_TypeError,
            "first argument should be a Variable, not '%s'",
            pyvar1->ob_type->tp_name);
    }
    if (!OrVariable_Check(pyvar2)) {
        raiseError(PyExc_TypeError,
            "second argument should be a Variable, not '%s'",
            pyvar2->ob_type->tp_name);
    }
    return new(type) TContingency(PVariable(pyvar1), PVariable(pyvar2));
}


PyObject *TContingency::py_add(PyObject *args, PyObject *kw)
{
    PyObject *pyouter, *pyinner;
    double weight = 1.0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|d:add",
        Contingency_add_keywords, &pyinner, &pyouter, &weight)) {
            return NULL;
    }
    TValue outval = outerVariable->py2val(pyouter);
    TValue inval = innerVariable->py2val(pyinner);
    add(outval, inval, weight);
    Py_RETURN_NONE;
}


PyObject *TContingency::py_normalize()
{
    normalize();
    Py_RETURN_NONE;
}


PyObject *TContingency::__subscript__(PyObject *index)
{
    if (PyTuple_Check(index) && (PyTuple_Size(index)==2)) {
        PyObject *pyouter = PyTuple_GET_ITEM(index, 0);
        PyObject *pyinner = PyTuple_GET_ITEM(index, 1);
        const TValue outer = outerVariable->py2val(pyouter);
        const TValue inner = innerVariable->py2val(pyinner);
        if (isnan(outer) || isnan(inner)) {
            raiseError(PyExc_IndexError,
                "contingency cannot be indexed by unknown values");
        }
        return PyFloat_FromDouble((*(*this)[outer])[inner]);
    }

    const TValue val = outerVariable->py2val(index);
    if (isnan(val)) {
        raiseError(PyExc_IndexError,
            "contingency cannot be indexed by unknown values");
    }
    return (*this)[val].toPython();
}


Py_ssize_t TContingency::__len__() const
{
    return size();
}

PyObject *TContingency::__item__(Py_ssize_t i) const
{
    if (!isDiscrete()) {
        raiseError(PyExc_TypeError, "continuous contingencies are not iterable");
    }
    if (i > discrete->size()) {
        raiseError(PyExc_IndexError, "index out of range");
    }
    return (*discrete)[i].toPython();
}

PyObject *TContingency::__getnewargs__(PyObject *, PyObject *) const
{
    PyObject *pycont;
    if (isDiscrete()) {
        pycont = vectorToPyList(*discrete);
    }
    else {
        pycont = PyList_New(continuous->size());
        Py_ssize_t i = 0;
        PITERATE(TDistributionMap, ci, continuous) {
            PyList_SetItem(pycont, i++,
                Py_BuildValue("(dN)", ci->first, ci->second.toPython()));
        }
    }
    return Py_BuildValue("(NNN)",
        outerVariable.toPython(), innerVariable.toPython(), pycont);
}

PyObject *TContingency::keys() const
{
    PyObject *res = PyList_New(size());
    Py_ssize_t i = 0;
    if (isDiscrete()) {
        while(i < size()) {
            PyObject *pyvalue = 
                PyUnicode_FromString(outerVariable->val2str(i).c_str());
            PyList_SetItem(res, i++, pyvalue);
        }
    }
    else {
        PITERATE(TDistributionMap, ci, continuous) {
            PyList_SetItem(res, i++, PyFloat_FromDouble(ci->first));
        }
    }
    return res;
}


PyObject *TContingency::values() const
{
    PyObject *res = PyList_New(size());
    Py_ssize_t i = 0;
    if (isDiscrete()) {
        PITERATE(TDistributionVector, ci, discrete) {
            PyList_SetItem(res, i++, ci->toPython());
        }
    }
    else {
        PITERATE(TDistributionMap, ci, continuous) {
            PyList_SetItem(res, i++, ci->second.toPython());
        }
    }
    return res;
}


PyObject *TContingency::items() const
{
    PyObject *res = PyList_New(size());
    Py_ssize_t i = 0;
    if (isDiscrete()) {
        PITERATE(TDistributionVector, ci, discrete) {
            PyObject *pair = Py_BuildValue("sN",
                outerVariable->val2str(i).c_str(), ci->toPython());
            PyList_SetItem(res, i++, pair);
        }
    }
    else {
        PITERATE(TDistributionMap, ci, continuous) {
            PyObject *pair = Py_BuildValue("dN",
                ci->first, ci->second.toPython());
            PyList_SetItem(res, i++, pair);
        }
    }
    return res;
}
