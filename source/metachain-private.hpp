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


TMetaValue::TMetaValue(TMetaId const anId, TValue const val)
    : isPrimitive(true),
    id(anId),
    value(val)
{}

TMetaValue::TMetaValue(TMetaId const anId, PyObject *val)
    : isPrimitive(false),
    id(anId),
    object(val)
{}

TMetaValue::TMetaValue(TMetaValue const &old)
    : isPrimitive(old.isPrimitive),
    id(old.id)
{
    if (isPrimitive) {
        value = old.value;
    }
    else {
        object = old.object; // no incref, it's all borrowed
    }
}


TMetaValue &TMetaValue::operator =(TMetaValue const &old)
{
    isPrimitive = old.isPrimitive;
    id = old.id;
    if (isPrimitive) {
        value = old.value;
    }
    else {
        object = old.object;  // no incref, it's all borrowed
    }
    return *this;
}


bool TMetaValue::isDefined() const
{
    return isPrimitive ? !isnan(value) : object != NULL;
}

// Private stuff, exposed only to allow inlining!

namespace MetaChain {

#pragma pack(1)

typedef struct {
    TMetaId id;
    int next;
} TMetaCommon;

typedef struct {
    TMetaId id;
    int next;
    TValue value;
} TMetaDouble;

typedef struct {
    TMetaId id;
    int next;
    PyObject *value;
} TMetaObject;


class TMetaPool {
    vector<TMetaDouble> poolDouble;
    vector<TMetaObject> poolObject;

    int freeDoublePtr;
    int freeObjectPtr;

public:
    TMetaPool();
    inline TMetaCommon &operator[](int const handle);
    inline TMetaDouble &atDouble(int const handle);
    inline TMetaObject &atObject(int const handle);

    int allocDouble(int next);
    int allocObject(int next);

    void freeDouble(int &handle);
    void freeObject(int &handle);
    inline void free(int &handle);
};


extern TMetaPool pool;

#pragma pack()


TMetaCommon &TMetaPool::operator[](int const handle)
{
    return handle < 0 ? (TMetaCommon &)atDouble(handle)
                      : (TMetaCommon &)atObject(handle);
}

TMetaDouble &TMetaPool::atDouble(int const handle)
{
    _ASSERT(handle < 0);
    return poolDouble[-handle];
}

TMetaObject &TMetaPool::atObject(int const handle)
{
    _ASSERT(handle > 0);
    return poolObject[handle];
}

void TMetaPool::free(int &handle)
{
    if (handle < 0) {
        freeDouble(handle);
    }
    else {
        freeObject(handle);
    }
}


inline int *findElement(int &handle, TMetaId const id, TMetaCommon *&el)
{
    int *handPtr = &handle;
    for(; *handPtr && (el=&pool[*handPtr])->id != id; handPtr = &el->next);
    return handPtr;
}


void killChain(int &handle)
{
    handle = 0;
}

void add(int &handle, TMetaId const id, TValue const value)
{
    handle = pool.allocDouble(handle);
    TMetaDouble &el = (TMetaDouble &)pool[handle];
    el.value = value;
    el.id = id;
}

void add(int &handle, TMetaId const id, PyObject *value)
{
    handle = pool.allocObject(handle);
    Py_XINCREF(value);
    TMetaObject &el = (TMetaObject &)pool[handle];
    el.value = value;
    el.id = id;
}


int advance(int const handle)
{
    return pool[handle].next;
}


void set(int &handle, TMetaId const id, TValue const value)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (*handPtr < 0) {
        ((TMetaDouble *)el)->value = value;
        return;
    }
    else if (*handPtr > 0) {
        // wrong pool: remove this one, add the new value at the beginning
        pool.freeObject(*handPtr);
    }
    add(handle, id, value);
}


void set(int &handle, TMetaId const id, PyObject *const value)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (*handPtr > 0) {
        TMetaObject *oel = (TMetaObject *)el;
        Py_XINCREF(value);
        Py_XDECREF(oel->value);
        oel->value = value;
        return;
    }
    else if (*handPtr < 0) {
        // wrong pool: remove this one, add the new value at the beginning
        pool.freeDouble(*handPtr);
    }
    add(handle, id, value);
}


void set(int &handle, int sourceHandle)
{
    TMetaCommon &el = pool[sourceHandle];
    if (sourceHandle < 0) {
        set(handle, el.id, ((TMetaDouble &)el).value);
    }
    else {
        set(handle, el.id, ((TMetaObject &)el).value);
    }      
}


void set(int &handle, TMetaValue const &metaValue)
{
    if (metaValue.isPrimitive) {
        set(handle, metaValue.id, metaValue.value);
    }
    else {
        set(handle, metaValue.id, metaValue.object);
    }
}


void add(int &handle, TMetaValue const &metaValue)
{
    if (metaValue.isPrimitive) {
        add(handle, metaValue.id, metaValue.value);
    }
    else {
        add(handle, metaValue.id, metaValue.object);
    }
}


TValue &getDouble(int handle, TMetaId id)
{
    double *value = getDoublePtr(handle, id);
    if (!value) {
        raiseError(PyExc_IndexError, "example has no meta attribute %i", id);
    }
    return *value;
}

TValue &setDefault(int &handle, TMetaId const id, TValue const defaultVal)
{
    double *value = getDoublePtr(handle, id);
    if (value) {
        return *value;
    }
    handle = pool.allocDouble(handle);
    TMetaDouble &el = (TMetaDouble &)pool[handle];
    el.value = defaultVal;
    el.id = id;
    return el.value;
}

PyObject *&getObject(int handle, TMetaId id)
{
    PyObject **value = getObjectPtr(handle, id);
    if (!value) {
        raiseError(PyExc_IndexError, "example has no meta attribute %i", id);
    }
    return *value;
}


bool has(int handle, TMetaId const id)
{
    TMetaCommon *el;
    return *findElement(handle, id, el) != 0;
}


TValue *getDoublePtr(int handle, TMetaId id)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (!*handPtr) {
        return NULL;
    }
    if (*handPtr > 0) {
        TMetaObject *oel = (TMetaObject *)el;
        if (oel) {
            raiseError(PyExc_TypeError, "expected a numeric meta value, not %s",
                ((TMetaObject *)el)->value->ob_type->tp_name);
        }
        else {
            raiseError(PyExc_TypeError,
                "expected a numeric meta value, not an object");
        }
    }
    return &((TMetaDouble *)el)->value;
}


PyObject **getObjectPtr(int handle, TMetaId id)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (!*handPtr) {
        return NULL;
    }
    if (*handPtr < 0) {
        raiseError(PyExc_TypeError, "expected a non-numeric meta value");
    }
    return &((TMetaObject *)el)->value;
}


TMetaValue get(int handle, TMetaId const id, bool throwExc)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (!*handPtr) {
        if (throwExc) {
            raiseError(PyExc_KeyError, "example has no meta attribute %i", id);
        }
        return TMetaValue(0, 0.0);
    }
    if (*handPtr < 0) {
        return TMetaValue(id, ((TMetaDouble *)el)->value);
    }
    else {
        return TMetaValue(id, ((TMetaObject *)el)->value);
    }
}


TMetaValue get(int handle)
{
    if (!handle) {
        raiseError(PyExc_RuntimeError, "invalid meta value handle (0)");
    }
    if (handle < 0) {
        TMetaDouble &el = pool.atDouble(handle);
        return TMetaValue(el.id, el.value);
    }
    else {
        TMetaObject &el = pool.atObject(handle);
        return TMetaValue(el.id, el.value);
    }
}


void remove(int &handle, TMetaId id, bool throwExc)
{
    TMetaCommon *el;
    int *handPtr = findElement(handle, id, el);
    if (*handPtr) {
        pool.free(*handPtr);
    }
    else {
        raiseError(PyExc_KeyError, "example has no meta attribute %i", id);
    }
}





} // end of namespace

