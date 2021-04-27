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


#ifndef __ORVECTOR_HPP
#define __ORVECTOR_HPP

#include "orvector.ppp"

#ifdef _MSC_VER
 #pragma warning (disable : 4786 4114 4018 4267)
#endif

template<class T>
class TReferenceHandler {
public:
    inline static int VisitRefs(visitproc visit, void *arg, T *b, T *e) { return 0; }
    inline static T convertFromPython(PyObject *obj) { return T(); }
    inline static PyObject *convertToPython(const T &it) { return NULL; }
};

template<class T>
class TWrappedReferenceHandler {
public:
    inline static int VisitRefs(visitproc visit, void *arg, T *b, T *e)
        { for(; b!= e; b++) { Py_VISIT(b->borrowPyObject()); } return 0; }
    inline static T convertFromPython(PyObject *obj);
    inline static PyObject *convertToPython(const T &it);
};


template<class T, class RH, PyTypeObject *MyPyType>
class TOrangeVector : public TOrange
{ 
public:
    typedef TOrangeVector<T, RH, MyPyType> MyType;
    typedef T *iterator;
    typedef T const *const_iterator;

    iterator _First, _Last, _End;

    class reverse_iterator {
    public:
        iterator position;
        inline explicit reverse_iterator(iterator p)         : position(p) {}
        inline reverse_iterator(const reverse_iterator &old) : position(old.position) {}
        inline reverse_iterator &operator ++()               {  --position; return *this; }
        inline reverse_iterator operator ++(int)             { reverse_iterator sv = *this; position--; return sv; }
        inline reverse_iterator &operator --()               {  ++position; return *this; }
        inline reverse_iterator operator --(int)             { reverse_iterator sv = *this; position++; return sv; }
        inline T &operator *() const                         { return position[-1]; }
        inline T *operator->() const                         { return (&**this); }
        inline reverse_iterator operator +(const int &N)     { return reverse_iterator(position - N); }
        inline reverse_iterator operator -(const int &N)     { return reverse_iterator(position + N); }
        int operator -(const reverse_iterator &other) const  { return other.position - position; }
        reverse_iterator &operator +=(const int &N)          { position -= N; return *this; }
        reverse_iterator &operator -=(const int &N)          { position += N; return *this; }
        bool operator == (const reverse_iterator &other) const  { return position == other.position; }
        bool operator != (const reverse_iterator &other) const  { return position != other.position; }
        bool operator < (const reverse_iterator &other) const   { return position > other.position; }
        bool operator <= (const reverse_iterator &other) const  { return position >= other.position; }
        bool operator > (const reverse_iterator &other) const   { return position < other.position; }
        bool operator >= (const reverse_iterator &other) const  { return position <= other.position; }
    };

    static inline void *operator new(size_t size) { return &PyObject_GC_New(OrOrange, MyPyType)->orange; }
    static inline void *operator new(size_t size, PyTypeObject *type) { return &PyObject_GC_New(OrOrange, type)->orange; }
    static inline void operator delete(void *obj) { TOrange::operator delete(obj); }
    static inline void operator delete(void *obj, PyTypeObject *) { TOrange::operator delete(obj); }

    inline void _Resize(const int &n);
    inline TOrangeVector<T, RH, MyPyType>();
    inline TOrangeVector<T, RH, MyPyType>(const int &N, const T &V = T());
    inline TOrangeVector<T, RH, MyPyType>(const TOrangeVector<T, RH, MyPyType> &old);
    inline TOrangeVector<T, RH, MyPyType>(const vector<T> &old);
    inline TOrangeVector<T, RH, MyPyType> &operator =(const TOrangeVector<T, RH, MyPyType> &old);
    inline ~TOrangeVector<T, RH, MyPyType>();

    bool operator ==(const TOrangeVector<T, RH, MyPyType> &other) const;

    TOrangeVector<T, RH, MyPyType> *clone() const { return new(OB_TYPE) TOrangeVector<T, RH, MyPyType>(*this); }

    inline iterator begin()                       { return _First; }
    inline const_iterator begin() const           { return _First; }
    inline reverse_iterator rbegin()              { return reverse_iterator(end()); }
    inline iterator end()                         { return _Last; }
    inline const_iterator end() const             { return _Last; }
    inline reverse_iterator rend()                { return reverse_iterator(begin()); }

    inline T &back()                              { return _Last[-1]; }
    inline const T &back() const                  { return _Last[-1]; }
    inline T &front()                             { return *_First; }
    inline const T &front() const                 { return *_First; }

    inline T &operator[](const int i)             { return _First[i]; }
    inline const T &operator[](const int i) const { return _First[i]; }
    
    inline bool empty() const                     { return _First == _Last; }
    inline int size() const                       { return _Last - _First; }

    inline T &at(const int &N);
    const T &at(const int &N) const;
    void clear();
    inline iterator erase(iterator it);
    iterator erase(iterator first, iterator last);
    inline iterator insert(iterator p, const T &X = T());
    void insert(iterator p, const int &n, const T &X);
    void insert(iterator p, iterator first, iterator last);
    inline void push_back(T const &x);
    void reserve(const int n);
    void resize(const int n, T x = T());

    virtual int clear_references();
    virtual int traverse_references(visitproc visit, void *arg);

    static PyObject *newFromArgument(PyTypeObject *type, PyObject *arg);
    static MyType *setterConversion(PyObject *obj);

    static PyObject *__new__(PyTypeObject *type, PyObject *args, PyObject *kw);
    static void __dealloc__(OrOrange *self);
    static int __traverse__(OrOrange *self, visitproc visit, void *args);
    static int __clear__(OrOrange *self);
    static PyObject *__getnewargs__(OrOrange *self);
    static Py_ssize_t __len__(OrOrange *self);
    static PyObject *__item__(OrOrange *self, Py_ssize_t ind);
    static int __ass_item__(OrOrange *self, Py_ssize_t index, PyObject *item);
    static PyObject *__subscript__(OrOrange * self, PyObject* item);
    static PyObject *__repeat__(OrOrange *self, Py_ssize_t times);
    static PyObject *__richcmp__(OrOrange *self, PyObject *other, int op);
    static PyObject *__str__(OrOrange *self);
    static PyObject *__repr__(OrOrange *self);
    static int __contains__(OrOrange *self, PyObject *item);
    static PyObject *__concat__(PyObject *self, PyObject *arg);
    static int __bool__(OrOrange *self, PyObject *arg);
    static PyObject *_get_slice(OrOrange *self, PyObject *item);
    static int __ass_subscript__(OrOrange *self, PyObject *item, PyObject *value);
    static int _set_slice(OrOrange *self, Py_ssize_t start, Py_ssize_t stop, PyObject *args);
    static PyObject *append(OrOrange *self, PyObject *item);
    static PyObject *count(OrOrange *self, PyObject *item);
    static PyObject *extend(OrOrange *self, PyObject *arg);
    static PyObject *index(OrOrange *self, PyObject *item);
    static PyObject *py_insert(OrOrange *self, PyObject *args);
    static PyObject *pop(OrOrange *self, PyObject *args);
    static PyObject *remove(OrOrange *self, PyObject *item);
    static PyObject *reverse(OrOrange *self);

    class TCmpByCallback
    {
    public:
        PyObject *cmpfunc;
        TCmpByCallback(PyObject *func);
        inline TCmpByCallback(const TCmpByCallback &other);
        inline ~TCmpByCallback();
        bool operator()(const T &x, const T &y) const;
    };

    static PyObject *sort(OrOrange *self, PyObject *args);
};


typedef TOrangeVector<string, TReferenceHandler<string>, &OrStringList_Type> TStringList;
typedef TOrangeVector<bool, TReferenceHandler<bool>, &OrBoolList_Type> TBoolList;
typedef TOrangeVector<int, TReferenceHandler<int>, &OrIntList_Type> TIntList;
typedef TOrangeVector<double, TReferenceHandler<double>, &OrFloatList_Type > TFloatList;
typedef TOrangeVector<pair<int, int>, TReferenceHandler<pair<int, int> >, &OrIntIntList_Type > TIntIntList;
typedef TOrangeVector<pair<double, double>, TReferenceHandler<pair<double, double> >, &OrFloatFloatList_Type > TFloatFloatList;

PYVECTOR(StringList, 0)
PYVECTOR(BoolList, 0)
PYVECTOR(IntList, 0)
PYVECTOR(FloatList, 0)
PYVECTOR(IntIntList, 0)
PYVECTOR(FloatFloatList, 0)

typedef TOrangeVector<PFloatList, TWrappedReferenceHandler<PFloatList>, &OrFloatListList_Type> TFloatListList;
PYVECTOR(FloatListList, FloatList)


/* These two do not really belong here; they were moved from stladdon.hpp since gcc complained
about using undefined type OrOrange *.  */
template<typename T>
PyObject *vectorToPyList(vector<T> const &v)
{
    PyObject *l = PyList_New(v.size());
    Py_ssize_t i = 0;
    for(typename vector<T>::const_iterator vi(v.begin()), ve(v.end());
        vi!=ve; vi++) {
            PyList_SetItem(l, i++, vi->toPython());
    }
    return l;
}

class OrOrange;

template<class T, PyTypeObject *type>
bool pyListToVector(PyObject *l, vector<T> &v, bool allowNULL)
{
    if (!PyList_Check(l)) {
        PyErr_Format(PyExc_TypeError,
            "expected list, not '%s'", l->ob_type->tp_name);
        return false;
    }
    v.resize(PyList_Size(l), T());
    Py_ssize_t i = 0;
    ITERATE(typename vector<T>, vi, v) {
        PyObject *item = PyList_GetItem(l, i++);
        if (allowNULL && (item == Py_None)) {
            *vi = T();
        }
        else { 
            if (!PyObject_IsInstance(item, (PyObject *)type)) {
                v.clear();
                PyErr_Format(PyExc_TypeError,
                    "expected instances of %s; item %i is %s",
                    type->tp_name, i-1, item->ob_type->tp_name);
                return false;
            }
            *vi = T(item);
        }
    }
    return true;
}



int _RoundUpSize(const int &n);
bool checkIndex(Py_ssize_t &index, Py_ssize_t max);
bool checkIndices(Py_ssize_t &start, Py_ssize_t &stop, Py_ssize_t max);

template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::_Resize(const int &n)
{
    int sze = _RoundUpSize(n);
    if (!_First) {
        _Last = _First = new T[sze];
        _End = _First + sze;
    }
    else if (_End - _First != sze) {
        int osize = size();
        iterator nfirst = new T[sze];
        copy(_First, _Last, nfirst);
//        memcpy(nfirst, _First, osize*sizeof(T));
        delete[] _First;
        _First = nfirst;
        _Last = _First + osize;
        _End = _First + sze;
    }
}

template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TOrangeVector()
: _First(NULL), _Last(NULL), _End(NULL)
{}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TOrangeVector(const int &N, const T &V)
: _First(NULL), _Last(NULL), _End(NULL)
{
    _Resize(N);
    int n;
    for(n = N; n--; *_Last++ = V);
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TOrangeVector(
    const TOrangeVector<T, RH, MyPyType> &old)
: _First(NULL), _Last(NULL), _End(NULL)
{
    _Resize(old.size());
    for(const_iterator r = old._First; r != old._Last; *_Last++ = *r++);
}
     
     
template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TOrangeVector(const vector<T> &old)
: _First(NULL), _Last(NULL), _End(NULL)
{
    _Resize(old.size());
    for(typename vector<T>::const_iterator vi(old.begin()), vi_end(old.end());
        vi != vi_end; *_Last++ = *vi++);
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType> &TOrangeVector<T, RH, MyPyType>::operator =(
    const TOrangeVector<T, RH, MyPyType> &old)
{ 
    if (&old == this)
        return *this;
    _Resize(old.size());
    for(iterator f = old._First; f != old._Last; *_Last++ = *f++);
    return *this;
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::~TOrangeVector()
{ 
    delete[] _First;
    _First = _Last = _End = NULL;
}


template<class T, class RH, PyTypeObject *MyPyType>
bool TOrangeVector<T, RH, MyPyType>::operator ==(
    const TOrangeVector<T, RH, MyPyType> &other) const
{
    if (size() != other.size()) {
        return false;
    }
    for(const_iterator mi(begin()), me(end()), oi(begin()); me!=me; mi++, oi++) {
        if (*mi != *oi) {
            return false;
        }
    }
    return true;
}


template<class T, class RH, PyTypeObject *MyPyType>
T &TOrangeVector<T, RH, MyPyType>::at(const int &N)
{ 
    if (N >= size())
        raiseError(PyExc_IndexError, "vector index out of range");
    return _First[N];
}

template<class T, class RH, PyTypeObject *MyPyType>
const T &TOrangeVector<T, RH, MyPyType>::at(const int &N) const
{ 
    if (N >= size())
        raiseError(PyExc_IndexError, "vector index out of range");
    return _First[N];
}

template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::clear()
{ 
    delete[] _First;
    _First = _End = _Last = NULL;
}

template<class T, class RH, PyTypeObject *MyPyType>
typename TOrangeVector<T, RH, MyPyType>::iterator TOrangeVector<T, RH, MyPyType>::erase(
    iterator it)
{ 
    for(iterator it2 = it+1; it2 != _Last; it2++) {
        it2[-1] = it2[0];
    }
    _Last--;
    return it;
}


template<class T, class RH, PyTypeObject *MyPyType>
typename TOrangeVector<T, RH, MyPyType>::iterator TOrangeVector<T, RH, MyPyType>::erase(
    iterator first, iterator last)
{ 
    if (first != last) {
        iterator it2;
        for(it2 = first; last != _Last; *it2++ = *last++);
        last = _Last;
        _Last = it2;
        for(; it2 != last; *it2++ = T());
    }
    return first;
}

    
template<class T, class RH, PyTypeObject *MyPyType>
typename TOrangeVector<T, RH, MyPyType>::iterator TOrangeVector<T, RH, MyPyType>::insert(
    iterator p, const T &X)
{ 
    const int ind = p - _First;
    insert(p, 1, X);
    return _First + ind;        
}


template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::insert(iterator p, const int &n, const T &X)
{
    if (_End - _Last < n) {
        const int pi = p - _First;
        _Resize(size() + n);
        p = _First + pi;
    }
  	for(iterator b = _Last; b != p; --b, b[n] = *b);
  	for(iterator e = p + n; p != e; *p++ = X);
    _Last += n;
}


template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::insert(iterator p, iterator first, iterator last)
{
    const int n = last - first;
    if (_End - _Last < n) {
        const int pi = p - _First;
        _Resize(size() + n);
        p = _First + pi;
    }
  	for(iterator b = _Last; b != p; --b, b[n] = *b);
    for(; first != last; *p++ = *first++);
    _Last += n;
}


template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::push_back(T const &x)
{  
    if (_Last == _End) {
        _Resize(size() + 1);
    }
    *_Last++ = x;
}


template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::reserve(const int n)
{ 
    if (n >= _Last - _First) {
        _Resize(n);
    }
}

template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::resize(const int n, T x)
{ 
    if (n < size()) {
        for(iterator it=_First + n; it != _Last; *it++ = T());
        _Resize(n);
        _Last = _First + n;
    }
    else {
        _Resize(n);
        for(iterator _nLast = _First + n; _Last != _nLast; *_Last++ = x);
    }
}

template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::newFromArgument(
    PyTypeObject *type, PyObject *arg) 
{
    PyObject *iter = NULL;
    if (arg) {
        iter = PyObject_GetIter(arg);
        if (!iter) {
            Py_DECREF(arg);
            return PyErr_Format(PyExc_TypeError,
                "invalid arguments for '%s' constructor (sequence expected)",
                type->tp_name);
        }
    }
    GUARD(iter);
    MyType *me = new(type) MyType();
    if (arg) {
        int i = 0;
        for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter), i++) {
            PyObject *it2 = item;
            GUARD(it2);
            T obj(RH::convertFromPython(item));
            if (PyErr_Occurred()) {
                for(; me->_First != me->_Last; *(me->_First)++ = T());
                me->_First = me->_Last = NULL;
                delete me;
                return NULL;
            }
            me->push_back(obj);
        }
    }
    return PyObject_FromNewOrange(me);
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType> *TOrangeVector<T, RH, MyPyType>::setterConversion(
    PyObject *obj)
{
    PyObject *lst = newFromArgument(MyPyType, obj);
    return lst ? (MyType *)&((OrOrange *)lst)->orange : NULL;
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *arg = NULL;
    if (!PyArg_ParseTuple(args, "|O:OrangeVector", &arg)) {
        return PyErr_Format(PyExc_TypeError,
            "invalid arguments for '%s' constructor (sequence expected)",
            type->tp_name);
    }
    return newFromArgument(type, arg);
}


template<class T, class RH, PyTypeObject *MyPyType>
void TOrangeVector<T, RH, MyPyType>::__dealloc__(OrOrange *self)
{
    delete (TOrangeVector<T, RH, MyPyType> *)&(self->orange);
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__traverse__(
    OrOrange *self, visitproc visit, void *args)
{ 
    return ((TOrangeVector<T, RH, MyPyType> *)&(self->orange))->
        traverse_references(visit, args);
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__clear__(OrOrange *self)
{ 
    return ((TOrangeVector<T, RH, MyPyType> *)&(self->orange))->
        clear_references();
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__getnewargs__(OrOrange *self)
{
    MyType &me = (MyType &)(self->orange);
    PyObject *aslist = PyList_New(me.size());
    Py_ssize_t i = 0;
    for(const_iterator mi=me._First; mi != me._Last; mi++, i++) {
        PyList_SetItem(aslist, i, RH::convertToPython(*mi));
    }
    return Py_BuildValue("(N)", aslist);
}


template<class T, class RH, PyTypeObject *MyPyType>
Py_ssize_t TOrangeVector<T, RH, MyPyType>::__len__(OrOrange *self)
{
    try {
        return ((MyType &)(self->orange)).size();
    }
    PyCATCH_1
}  


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__item__(
    OrOrange *self, Py_ssize_t ind)
{
    MyType &me = (MyType &)(self->orange);
    if (!checkIndex(ind, me.size())) {
        return NULL;
    }
    PyObject *res = RH::convertToPython(me[ind]);
    return res;
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__ass_item__(
    OrOrange *self, Py_ssize_t index, PyObject *item)
{ 
    try {
        MyType &me = (MyType &)self->orange;
        if (!checkIndex(index, me.size())) {
            return -1;
        }
        if(!item) {
            me.erase(me.begin()+index);
        }
        else {
            T obj(RH::convertFromPython(item));
            if (PyErr_Occurred())
                return -1;
            me[index] = obj;
        }
        return 0;
    }
    PyCATCH_1
}

    // Adapted from Python 3.1's list object 
template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__subscript__(OrOrange * self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += ((MyType &)(self->orange)).size();
        return __item__(self, i);
    }
    return _get_slice(self, item);
    }


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__repeat__(OrOrange *self, Py_ssize_t times)
{
    PyObject *emtuple = NULL, *emdict = NULL, *newList = NULL;
    try {
        emtuple = PyTuple_New(0);
        emdict = PyDict_New();
        newList = Py_TYPE(self)->tp_new(Py_TYPE(self), emtuple, emdict);
        Py_DECREF(emtuple); 
        emtuple = NULL;
        Py_DECREF(emdict);
        emdict = NULL;
        if (!newList) {
            return NULL;
        }
        MyType &me = (MyType &)self->orange;
        MyType &newhim = (MyType &)((OrOrange *)newList)->orange;
        while (times-- > 0) {
            for (const_iterator li = me.begin(), le = me.end(); li!=le; li++) {
                newhim.push_back(*li);
            }
        }
        return newList;
    }
    catch (exception err) {
        Py_XDECREF(emtuple);
        Py_XDECREF(emdict);
        Py_XDECREF(newList);
        return PyErr_Format(PyExc_Exception, err.what());
    }
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__richcmp__(OrOrange *self, PyObject *other, int op)
{ 
    try {
        PyObject *hisIter = PyObject_GetIter(other);
        if (!hisIter) {
            return PyErr_Format(PyExc_TypeError,
                "invalid arguments, expected sequence, not '%s'",
                other->ob_type->tp_name);
        }
        GUARD(hisIter);
        PyObject *res = NULL;
        MyType &me = (MyType &)(self->orange);
        iterator myIter(me.begin());
        for(;;myIter++) {
            PyObject *hisItem = PyIter_Next(hisIter);
            GUARD(hisItem);
            PyObject *myItem = NULL;
            if (myIter != me.end()) {
                myItem = RH::convertToPython(*myIter);
                if (!myItem)
                    return NULL;
            }
            GUARD(myItem);
            if (!myItem || !hisItem) {
                if (   myItem &&  ((op == Py_NE) || (op == Py_GT) || (op == Py_GE))
                    || hisItem && ((op == Py_NE) || (op == Py_LT) || (op == Py_LE))
                    || !myItem && !hisItem && 
                             ((op == Py_EQ) || (op == Py_LE) || (op == Py_GE))) {
                        Py_RETURN_TRUE;
                }
                else {
                    Py_RETURN_FALSE;
                }
            }
            const int k = PyObject_RichCompareBool(myItem, hisItem, Py_NE);
            if (k) {
                return k == -1 ? NULL : PyObject_RichCompare(myItem, hisItem, op);
            }
        }
    }
    PyCATCH
        return NULL; // can't come here
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__str__(OrOrange *self)
{ 
    MyType &me = (MyType &)(self->orange);
    string res("<");
    for(iterator bi(me.begin()), ei(bi), ee(me.end()); ei!=ee; ei++) {
        if (ei!=bi) {
            res += ", ";
        }
        PyObject *item = RH::convertToPython(*ei);
        if (!item)
            return NULL;
        PyObject *repred = PyObject_Str(item);
        Py_DECREF(item);
        res += PyUnicode_As_string(repred);
        Py_DECREF(repred);
    }
    res += ">";
    return PyUnicode_FromString(res.c_str());
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__repr__(OrOrange *self) {
    return __str__(self);
}

template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__contains__(OrOrange *self, PyObject *item)
{ 
    try {
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return -1;
        MyType &me = (MyType &)(self->orange);
        int cnt = 0;
        for(iterator bi(me.begin()), be(me.end()); bi!=be; bi++) {
            if (obj==*bi) {
                return 1;
            }
        }
        return 0;
    }
    PyCATCH_1
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::__concat__(PyObject *self, PyObject *arg)
{ 
    try {
        OrOrange *newList = (OrOrange *)newFromArgument(self->ob_type, self);
        MyType &him = (MyType &)(newList->orange);
        if (_set_slice(newList, him.size(), him.size(), arg) == -1) {
            Py_DECREF(newList);
            return NULL;
        }
        return (PyObject *)newList;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__bool__(OrOrange *self, PyObject *arg)
{
    return ((MyType &)(self->orange)).size() ? 1 : 0;
}


/* Adapted from Python 3.1's list object */
template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::_get_slice(OrOrange *self, PyObject *item)
{
    if (PySlice_Check(item)) {
        MyType &me = (MyType &)(self->orange);
        Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
        if (PySlice_GetIndicesEx(item, me.size(),
            &start, &stop, &step, &slicelength) < 0) {
                return NULL;
        }
#else
        if (PySlice_GetIndicesEx((PySliceObject *)item, me.size(),
            &start, &stop, &step, &slicelength) < 0) {
                return NULL;
        }
#endif
        TOrangeVector<T, RH, MyPyType> *newList = 
            new (self->ob_base.ob_type)(TOrangeVector<T, RH, MyPyType>)();
        iterator it = me.begin()+start;
        for (int i = 0; i < slicelength; it += step, i++) {
            newList->push_back(*it);
        }
        return PyObject_FromNewOrange(newList);
    }
    else {
        return PyErr_Format(PyExc_TypeError,
            "list indices must be integers, not %.200s",
            item->ob_type->tp_name);
    }
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::__ass_subscript__(
    OrOrange *self, PyObject *item, PyObject *value)
{
    MyType &me = (MyType &)(self->orange);
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += me.size();
        return __ass_item__(self, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
        if (PySlice_GetIndicesEx(item, me.size(),
            &start, &stop, &step, &slicelength) < 0) {
                return -1;
        }
#else
        if (PySlice_GetIndicesEx((PySliceObject *)item, me.size(),
            &start, &stop, &step, &slicelength) < 0) {
                return -1;
        }
#endif
        if (step != 1) {
            PyErr_Format(PyExc_IndexError,
                "Orange's vectors can only assert slices with a step of 1 not %i",
                step);
            return -1;
        }
        return _set_slice(self, start, stop, value);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "list indices must be integers, not %.200s",
            item->ob_type->tp_name);
        return -1;
    }
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::_set_slice(
    OrOrange *self, Py_ssize_t start, Py_ssize_t stop, PyObject *args)
{ 
    try {
        MyType &me = (MyType &)(self->orange);
        if (!checkIndices(start, stop, me.size())) {
            return -1;
        }
        if (!args) {
            me.erase(me.begin()+start, me.begin()+stop);
            return 0;
        }
        PyObject *newList = newFromArgument(Py_TYPE(self), args);
        if (!newList) {
            return -1;
        }
        MyType &newHim = (MyType &)(((OrOrange *)newList)->orange);

        me.erase(me.begin()+start, me.begin()+stop);
        me.insert(me.begin()+start, newHim.begin(), newHim.end());
        Py_DECREF(newList);
        newList = NULL;
        return 0;
    }
    PyCATCH_1
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::append(OrOrange *self, PyObject *item)
{ 
    try {
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return NULL;
        ((MyType &)(self->orange)).push_back(obj);
        Py_RETURN_NONE;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::count(OrOrange *self, PyObject *item)
{ 
    try {
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return NULL;
        MyType &me = (MyType &)(self->orange);
        int cnt=0;
        for(iterator bi(me.begin()), be(me.end()); bi!=be; bi++) {
            if (obj==*bi) {
                cnt++;
            }
        }
        return PyLong_FromLong(cnt);
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::extend(OrOrange *self, PyObject *arg)
{
    try {
        MyType &me = (MyType &)(self->orange);
        if (_set_slice(self, me.size(), me.size(), arg) == -1) {
            return NULL;
        }
        Py_INCREF(self);
        return (PyObject *)self;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::index(OrOrange *self, PyObject *item)
{ 
    try {
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return NULL;
        MyType &me = (MyType &)(self->orange);
        int i=0;
        for(iterator bi(me.begin()), be(me.end()); bi!=be; bi++, i++) {
            if (obj==*bi) {
                return PyLong_FromLong(i);
            }
        }
        return PyErr_Format(PyExc_ValueError, "index(x): x not in list");
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::py_insert(OrOrange *self, PyObject *args)
{ 
    try {
        PyObject *item;
        int iindex;
        if (!PyArg_ParseTuple(args, "iO:insert", &iindex, &item)) {
            return NULL;
        }
        MyType &me = (MyType &)(self->orange);
        Py_ssize_t index = iindex;
        if (index > me.size()) {
            index = me.size();
        }
        if (index < 0) {
            index += me.size();
            if (index < 0) {
                index = 0;
            }
        }
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return NULL;
        me.insert(me.begin()+index, obj);
        Py_RETURN_NONE;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::pop(OrOrange *self, PyObject *args)
{ 
    try {
        MyType &me = (MyType &)self->orange;
        int idx = me.size()-1;
        if (!PyArg_ParseTuple(args, "|i:pop", &idx)) {
            return NULL;
        }
        PyObject *ret = __item__(self, idx);
        if (!ret) {
            return NULL;
        }
        me.erase(me.begin()+idx);
        return ret;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::remove(OrOrange *self, PyObject *item)
{ 
    try {
        T obj(RH::convertFromPython(item));
        if (PyErr_Occurred())
            return NULL;
        MyType &me = (MyType &)(self->orange);
        int i=0;
        for(iterator bi(me.begin()), be(me.end()); bi!=be; bi++, i++) {
            if (obj==*bi) {
                me.erase(bi);
                Py_RETURN_NONE;
            }
        }
        return PyErr_Format(PyExc_ValueError, "remove(x): x not in list");
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::reverse(OrOrange *self)
{ 
    try {
        MyType &me = (MyType &)(self->orange);
        std::reverse(me.begin(), me.end());
        Py_RETURN_NONE;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TCmpByCallback::TCmpByCallback(PyObject *func)
{ 
    if (!PyCallable_Check(func))
        raiseError(PyExc_TypeError, "compare object not callable");
    cmpfunc = func;
    Py_INCREF(cmpfunc);
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TCmpByCallback::TCmpByCallback(const TCmpByCallback &other)
: cmpfunc(other.cmpfunc)
{ 
    Py_INCREF(cmpfunc);
}


template<class T, class RH, PyTypeObject *MyPyType>
TOrangeVector<T, RH, MyPyType>::TCmpByCallback::~TCmpByCallback()
{ Py_DECREF(cmpfunc); 
}


template<class T, class RH, PyTypeObject *MyPyType>
bool TOrangeVector<T, RH, MyPyType>::TCmpByCallback::operator()(const T &x, const T &y) const
{ 
    PyObject *pyx(RH::convertToPython(x)), *pyy(RH::convertToPython(y));
    if (!pyx || !pyy)
        throw PyException();
    PyObject *cmpres = PyObject_CallFunction(cmpfunc, "OO", pyx, pyy);
    Py_DECREF(pyx);
    Py_DECREF(pyy);
    if (!cmpres) {
        throw PyException();
    }
    const int res = PyLong_AsLong(cmpres);
    Py_DECREF(cmpres);
    return res<0;
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TOrangeVector<T, RH, MyPyType>::sort(OrOrange *self, PyObject *args)
{
    try {
        PyObject *cmpfunc=NULL;
        if (!PyArg_ParseTuple(args, "|O:sort", &cmpfunc)) {
            return NULL;
        }
        MyType &me = (MyType &)(self->orange);
        if (cmpfunc) {
            std::sort(me.begin(), me.end(), TCmpByCallback(cmpfunc));
        }
        else {
            std::sort(me.begin(), me.end());
        }
        Py_RETURN_NONE;
    }
    PyCATCH
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::clear_references() 
{
    clear();
    return TOrange::clear_references();
}


template<class T, class RH, PyTypeObject *MyPyType>
int TOrangeVector<T, RH, MyPyType>::traverse_references(visitproc visit, void *arg)
{ 
    int err = RH::VisitRefs(visit, arg, _First, _Last);
    if (!err) {
        err = TOrange::traverse_references(visit, arg);
    }
    return err;
}

template<class T>
T TWrappedReferenceHandler<T>::convertFromPython(PyObject *obj)
{ 
    if (!OrOrange_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "Cannot convert an instance of '%s' to appropriate Orange type",
            obj->ob_type->tp_name);
        return T();
    }
    return T(obj);
}


template<class T>
PyObject *TWrappedReferenceHandler<T>::convertToPython(const T &it)
{
    return it.toPython(); 
}   


template<>
inline int TReferenceHandler<int>::convertFromPython(PyObject *obj)
{ 
    if (!PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "cannot convert '%s' to an int",
            obj->ob_type->tp_name);
        return 0;
    }
    return PyLong_AsLong(obj);
}


template<>
inline PyObject *TReferenceHandler<int>::convertToPython(const int &it)
{
    return PyLong_FromLong(it);
}   


template<>
inline string TReferenceHandler<string>::convertFromPython(PyObject *obj)
{ 
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "cannot convert '%s' to a string", obj->ob_type->tp_name);
        return string();
    }
    return PyUnicode_As_string(obj);
}


template<>
inline PyObject *TReferenceHandler<string>::convertToPython(const string &it)
{
    return PyUnicode_FromString(it.c_str());
}   


template<>
inline double TReferenceHandler<double>::convertFromPython(PyObject *obj)
{ 
    double res;
    if (!PyNumber_ToDouble(obj, res)) {
        PyErr_Format(PyExc_TypeError, 
            "cannot convert '%s' to a float", obj->ob_type->tp_name);
        return 0;
    }
    return res;
}


template<>
inline PyObject *TReferenceHandler<double>::convertToPython(const double &it)
{
    return PyFloat_FromDouble(it);
}   


template<>
inline pair<int, int> TReferenceHandler<pair<int, int> >::convertFromPython(PyObject *obj)
{ 
    if (!PyTuple_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "IntIntList: a tuple expected, got '%s'", obj->ob_type->tp_name);
        return pair<int, int>();
    }
    int f, s;
    if (!PyArg_ParseTuple(obj, "ii:IntIntList", &f, &s)) {
        PyErr_Clear();
        return pair<int, int>();
    }
    return pair<int, int>(f, s);
}


template<>
inline PyObject *TReferenceHandler<pair<int, int> >::convertToPython(const pair<int, int> &ii)
{
    return Py_BuildValue("(ii)", ii.first, ii.second);
}

template<>
inline pair<double, double> TReferenceHandler<pair<double, double> >::convertFromPython(PyObject *obj)
{ 
    if (!PyTuple_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "FloatFloatList: a tuple expected, got '%s'", obj->ob_type->tp_name);
        return pair<double, double>();
    }
    double f, s;
    if (!PyArg_ParseTuple(obj, "dd:IntIntList", &f, &s)) {
        PyErr_Clear();
        return pair<double, double>();
    }
    return pair<double, double>(f, s);
}


template<>
inline PyObject *TReferenceHandler<pair<double, double> >::
    convertToPython(const pair<double, double> &ii)
{
    return Py_BuildValue("(dd)", ii.first, ii.second);
}

#endif
