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
#include "learner.px"

PyObject *TLearner::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable examples;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:Learner",
        Learner_call_keywords, &PExampleTable::argconverter, &examples)) {
            return NULL;
    }
    return (*this)(examples).getPyObject();
}


PyTypeObject *TLearner::getReturnType(PyTypeObject *defaultType) const
{
    PyTypeObject *retType = (PyTypeObject *)PyObject_GetAttrString(
        (PyObject *)OB_TYPE, "__classifier__");

    if (!retType) {
        PyErr_Clear();
        return defaultType;
    }

    if (!PyType_IsSubtype(retType, defaultType)) {
        Py_DECREF(retType); // it won't go away
        raiseError(PyExc_TypeError, "%s is not derived from %s",
            retType->tp_name, defaultType->tp_name);
    }

    return retType;
}
