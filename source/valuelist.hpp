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


#ifndef __VALUELIST_HPP
#define __VALUELIST_HPP

#ifdef _MSC_VER
 #pragma warning (disable : 4786 4114 4018 4267)
#endif

#include "valuelist.ppp"

class TValueList : public TFloatList
{ 
public:
    __REGISTER_CLASS(ValueList);

    PVariable variable; //P variable to which the values refer

    inline TValueList();
    inline TValueList(PVariable const &);
    inline TValueList(TValueList const &);
    inline TValueList(vector<double> const &);

    static TValueList *setterConversion(PyObject *obj);
    static bool convertVarSeq(PVariable const &var, PyObject *seq,
                              vector<double> &values);

    inline int getIndex(PyObject *index);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("([variable, values]); construct a new ValueList");
    PyObject *__getnewargs__() PYARGS(METH_NOARGS, "(); prepare arguments for pickling");
    PyObject *__subscript__(PyObject* item);
    int __ass_subscript__(PyObject *item, PyObject *value);
};

//P YVECTOR(ValueList, 0)

TValueList::TValueList()
{}

TValueList::TValueList(PVariable const &var)
: variable(var)
{}

TValueList::TValueList(TValueList const &vl)
: TFloatList(vl),
variable(vl.variable)
{}

TValueList::TValueList(vector<double> const &v)
: TFloatList(v)
{}


int TValueList::getIndex(PyObject *index)
{
    if (!PyLong_Check(index)) {
        PyErr_Format(PyExc_TypeError,
            "indices must be integers, not '%s'", index->ob_type->tp_name);
        return -1;
    }
    size_t const idx = PyLong_AsLong(index);
    if (idx >= size()) {
        PyErr_Format(PyExc_IndexError, "index %i out of range", idx);
        return -1;
    }
    return idx;
}


#endif
