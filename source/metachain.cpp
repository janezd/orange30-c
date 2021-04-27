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

namespace MetaChain {

TMetaPool pool;

TMetaPool::TMetaPool()
: poolDouble(1),
poolObject(1),
freeDoublePtr(0),
freeObjectPtr(0)
{}


int TMetaPool::allocDouble(int next)
{
    if (freeDoublePtr) {
        int const res = freeDoublePtr;
        TMetaDouble &el = atDouble(freeDoublePtr);
        freeDoublePtr = el.next;
        el.next = next;
        return res;
    }
    else {
        poolDouble.push_back(TMetaDouble());
        poolDouble.back().next = next;
        return - int(poolDouble.size() - 1);
    }
}

int TMetaPool::allocObject(int next)
{
    if (freeObjectPtr) {
        int const res = freeObjectPtr;
        TMetaObject &el = atObject(freeObjectPtr);
        freeObjectPtr = el.next;
        el.next = next;
        return res;
    }
    else {
        poolObject.push_back(TMetaObject());
        TMetaObject &el = poolObject.back();
        el.value = NULL;
        el.next = next;
        return poolObject.size() - 1;
    }
}


void TMetaPool::freeDouble(int &handle)
{
    _ASSERT(handle < 0);
    TMetaDouble &el = atDouble(handle);
    int prevHandle = handle;
    handle = el.next;
    el.next = freeDoublePtr;
    freeDoublePtr = prevHandle;
}

void TMetaPool::freeObject(int &handle)
{
    _ASSERT(handle > 0);
    TMetaObject &el = atObject(handle);
    Py_XDECREF(el.value);
    el.value = NULL;

    int prevHandle = handle;
    handle = el.next;
    el.next = freeObjectPtr;
    freeObjectPtr = prevHandle;
}



void freeChain(int &handle)
{
    int handPtr = handle, nextPtr;
    for(; handPtr; handPtr = nextPtr) {
        nextPtr = pool[handPtr].next;
        pool.free(handPtr);
    }
}


void copyChain(int &handle, int source)
{
    if (handle == source) {
        return;
    }
    for(; source; source = pool[source].next) {
        set(handle, source);
    }
}

inline void insertChain(int &handle, int source)
{
    while(source) {
        if (source < 0) {
            TMetaDouble &el = pool.atDouble(source);
            add(handle, el.id, el.value);
            source = el.next;
        }
        else {
            TMetaObject &el = pool.atObject(source);
            add(handle, el.id, el.value);
            source = el.next;
        }
    }
}
 

/*! Pack the chain as a list of pairs (variable, value) for pickling
    of examples and tables.
    Meta values that are not registered in the domain are ignored.
    Missing values are coded as \c None.

    \param idx The index of the first meta value
    \param domain Domain to which the meta ids correspond
*/
PyObject *packChain(int handle, PDomain const &domain)
{
    PyObject *res = PyList_New(0);
    while(handle) {
        TMetaCommon const &el = pool[handle];
        PVariable var = domain->getMetaVar(el.id, false);
        if (var) {
            PyObject *o;
            if (handle < 0 ?
                isnan(((TMetaDouble &)el).value) : !((TMetaObject &)el).value) {
                    o = Py_BuildValue("(NO)", var.getPyObject(), Py_None);
            }
            else if (handle < 0) {
                o = Py_BuildValue("(Nd)", var.getPyObject(), ((TMetaDouble &)el).value);
            }
            else {
                o = Py_BuildValue("(NO)", var.getPyObject(), ((TMetaObject &)el).value);
            }
            PyList_Append(res, o);
            Py_DECREF(o);
        }
        handle = el.next;
    }
    return res;
}

/*! Unpack the chain as a list of pairs (variable, value) for pickling
    of examples and tables. \c None is interpreted as missing value.

    \param chain Packed chain
    \param idx The index of the first meta value
    \param domain Domain to which the meta ids correspond
*/
void unpackChain(PyObject *chain, int &handle, PDomain const &domain)
{
    killChain(handle);
    for(Py_ssize_t i = 0, sze = PyList_Size(chain); i < sze; i++) {
        PVariable var;
        PyObject *pyval;
        if (!PyArg_ParseTuple(PyList_GetItem(chain, i), "O&O",
            &PVariable::argconverter, &var, &pyval)) {
                raiseError(PyExc_UnpicklingError, "invalid meta value");
        }
        TMetaId id = domain->getMetaNum(var);
        if (var->isPrimitive()) {
            add(handle, id, Py_None ? UNDEFINED_VALUE : PyFloat_AsDouble(pyval));
        }
        else {
            add(handle, id, Py_None ? NULL : pyval);
        }
    }
}

}; // namespace MetaChain