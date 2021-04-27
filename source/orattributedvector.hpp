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


#ifndef __ORATTRIBUTEDVECTOR_HPP
#define __ORATTRIBUTEDVECTOR_HPP

#ifdef _MSC_VER
 #pragma warning (disable : 4786 4114 4018 4267)
#endif

#include "orattributedvector.ppp"

template<class T, class RH, PyTypeObject *MyPyType>
class TAttributedVector : public TOrangeVector<T, RH, MyPyType>
{ 
public:
    PVarList attributes;

    inline TAttributedVector<T, RH, MyPyType>();
    inline TAttributedVector<T, RH, MyPyType>(int const N, T const &V = T());
    inline TAttributedVector<T, RH, MyPyType>(PVarList const &attrs, T const &V = T());
    inline TAttributedVector<T, RH, MyPyType>(TAttributedVector<T, RH, MyPyType> const &old);
    inline TAttributedVector<T, RH, MyPyType>(vector<T> const &old);

    static int argconverter(PyObject *, void *);
    static int argconverter_n(PyObject *, void *);
    static int setter(PyObject *whom, PyObject *mine, size_t *offset);
    static TAttributedVector<T, RH, MyPyType> *setterConversion(PyObject *obj);

    static int getIndex(OrOrange *self, PyObject *index);
    static PyObject *__subscript__(OrOrange * self, PyObject* item);
    static int __ass_subscript__(OrOrange *self, PyObject *item, PyObject *value);
};

typedef TAttributedVector<double, TReferenceHandler<double>, 
                          &OrAttributedFloatList_Type > TAttributedFloatList;
typedef TAttributedVector<bool, TReferenceHandler<bool>,
                          &OrAttributedBoolList_Type > TAttributedBoolList;

PYVECTOR(AttributedFloatList, 0)
PYVECTOR(AttributedBoolList, 0)



template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType>::TAttributedVector()
{}


template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType>::TAttributedVector(int const N, T const &V)
: TOrangeVector<T, RH, MyPyType>(N, V)
{}


template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType>::TAttributedVector(PVarList const &attrs, T const &V)
: TOrangeVector<T, RH, MyPyType>(attrs->size(), V),
attributes(attrs)
{}


template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType>::TAttributedVector(
    TAttributedVector<T, RH, MyPyType> const &old)
: TOrangeVector<T, RH, MyPyType>(old),
attributes(old.attributes)
{}


template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType>::TAttributedVector(vector<T> const &old)
: TAttributedVector<T, RH, MyPyType>::TAttributedVector(old),
attributes((PyObject*)NULL)
{}


template<class T, class RH, PyTypeObject *MyPyType>
TAttributedVector<T, RH, MyPyType> *
TAttributedVector<T, RH, MyPyType>::setterConversion(PyObject *obj)
{
    return NULL;
}


template<class T, class RH, PyTypeObject *MyPyType>
int TAttributedVector<T, RH, MyPyType>::getIndex(OrOrange *self, PyObject *index)
{
    TAttributedVector<T, RH, MyPyType> *me = 
        &(TAttributedVector<T, RH, MyPyType> &)(self->orange);
    if (PyLong_Check(index)) {
        int res = (int)PyLong_AsLong(index);
        if (res < 0) {
            res += me->size();
        }
        if ((res >= me->size()) || (res < 0)) {
            PyErr_Format(PyExc_IndexError, "index %i is out of range", res);
            return ILLEGAL_INT;
        }
        return res;
    }

    if (OrVariable_Check(index)) {
        if (!me->attributes) {
            PyErr_Format(PyExc_AttributeError,
                "cannot index by variables since the variable list is "
                "not defined, need integer indices");
            return ILLEGAL_INT;
        }
        PVariable var(index);
        TVarList::const_iterator vi(me->attributes->begin());
        TVarList::const_iterator const ve(me->attributes->end());
        int ind = 0;
        for(; vi!=ve; vi++, ind++) {
            if (*vi == var) {
                return ind;
            }
        }
        PyErr_Format(PyExc_AttributeError,
            "attribute '%s' is not found", var->cname());
        return ILLEGAL_INT;
    }

    if (PyUnicode_Check(index)) {
        if (!me->attributes) {
            PyErr_Format(PyExc_AttributeError,
                "cannot index by variable names since the variable list is "
                "not defined, need integer indices");
            return ILLEGAL_INT;
        }
        string name = PyUnicode_As_string(index);
        TVarList::const_iterator vi(me->attributes->begin()), ve(me->attributes->end());
        int ind = 0;
        for(; vi!=ve; vi++, ind++) {
            if ((*vi)->getName() == name) {
                return ind;
            }
        }
        PyErr_Format(PyExc_AttributeError,
            "attribute '%s' not found", name.c_str());
        return ILLEGAL_INT;
    }

    PyErr_Format(PyExc_TypeError,
        "cannot index the list by '%s'", index->ob_type->tp_name);
    return ILLEGAL_INT;
}


template<class T, class RH, PyTypeObject *MyPyType>
PyObject *TAttributedVector<T, RH, MyPyType>::__subscript__(
    OrOrange * self, PyObject* item)
{
    if (PySlice_Check(item)) {
        return TAttributedVector<T, RH, MyPyType>::_get_slice(self, item);
    }
    const int ind = getIndex(self, item);
    return TAttributedVector<T, RH, MyPyType>::__item__(self, ind);
}


template<class T, class RH, PyTypeObject *MyPyType>
int TAttributedVector<T, RH, MyPyType>::__ass_subscript__(OrOrange *self,
                                                          PyObject *item,
                                                          PyObject *value)
{
    if (PySlice_Check(item)) {
        return TOrangeVector<T, RH, MyPyType>::__ass_subscript__(self, item, value);
    }
    const int ind = getIndex(self, item);
    return TAttributedVector<T, RH, MyPyType>::__ass_item__(self, ind, value);
}


#endif
