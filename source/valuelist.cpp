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
#include "valuelist.px"


bool TValueList::convertVarSeq(PVariable const &var, PyObject *seq,
                               vector<double> &values)
{
    PyObject *iter = PyObject_GetIter(seq);
    if (!iter) {
        PyErr_Format(PyExc_TypeError,
            "invalid arguments; variable and sequence expected");
        return false;
    }
    GUARD(iter);
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
        GUARD(item);
        values.push_back(var->py2val(item));
    }
    return true;
}


TValueList *TValueList::setterConversion(PyObject *obj)
{
    vector<double> values;
    PVariable var;
    PyObject *seq;
    if (PyTuple_Check(obj) &&
        PyArg_ParseTuple(obj, "O&O", &PVariable::argconverter, &var, &seq)) {
            if (!convertVarSeq(var, seq, values)) {
                return NULL;
            }
    }
    else if (!convertFromPython(obj, values)) {
        return NULL;
    }
    PyErr_Clear();
    TValueList *vf = new TValueList(values);
    vf->variable = var;
    return vf;
}


TOrange *TValueList::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    vector<double> values;
    PVariable var;
    PyObject *seq;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&|O", ValueList_keywords,
        &PVariable::argconverter_n, &var, &seq)) {
            if (!(var ? convertVarSeq(var, seq, values)
                      : convertFromPython(seq, values))) {
                return NULL;
            }
    }
    else if (!kw && args && (PyTuple_Size(args)==1)) {
        if (!convertFromPython(PyTuple_GET_ITEM(args, 0), values)) {
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError, "invalid arguments for ValueList");
        return NULL;
    }
    TValueList *vf = new(type) TValueList(values);
    vf->variable = var;
    return vf;
}


PyObject *TValueList::__subscript__(PyObject* index)
{
    int const idx = getIndex(index);
    if (idx < 0) {
        return NULL;
    }
    return PyObject_FromNewOrange(new TPyValue(variable, (*this)[idx]));
}
    
int TValueList::__ass_subscript__(PyObject* index, PyObject *value)
{
    int const idx = getIndex(index);
    if (idx < 0) {
        return NULL;
    }
    double val = variable ? variable->py2val(value) : PyNumber_AsDouble(value);
    (*this)[idx] = val;
    return 0;
}


PyObject *TValueList::__getnewargs__()
{
    PyObject *aslist = PyList_New(size());
    Py_ssize_t i = 0;
    for(const_iterator mi=_First; mi != _Last; mi++, i++) {
        PyList_SetItem(aslist, i, TReferenceHandler<double>::convertToPython(*mi));
    }
    return Py_BuildValue("(NN)", variable.toPython(), aslist);
}


