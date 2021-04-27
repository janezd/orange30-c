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


#ifndef __PYVALUE_HPP
#define __PYVALUE_HPP

#include "pyvalue.ppp"

// Not used internally but only for exposing values in Python
class TPyValue : public TOrange {
public:
    __REGISTER_CLASS(PyValue)

    PVariable variable; //PR variable to which the value corresponds
    union {
        double value;
        PyObject *object;
    };

    TPyValue(PVariable const &);
    TPyValue(PVariable const &, PyObject *);
    TPyValue(PVariable const &, double const);
    TPyValue(PVariable const &, TMetaValue const &);
    TPyValue(TPyValue const &);
    ~TPyValue();

    inline bool isPrimitive() const { return !variable || variable->isPrimitive(); }

    static TOrange *__new__(PyTypeObject *, PyObject *, PyObject *) PYDOC("(variable[, value]) -> Value; value can be number or string");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepares arguments for unpickling");
    PyObject *__repr__() const;
    long __hash__() const;
    PyObject *__richcmp__(PyObject *other, int op) const;
    static int __setattr__(PyObject *self, PyObject *name, PyObject *value);

    PyObject *__int__() const;
    PyObject *__float__() const;
    static PyObject *__add__(PyObject *me, PyObject *other);
    static PyObject *__sub__(PyObject *me, PyObject *other);
    static PyObject *__mul__(PyObject *me, PyObject *other);
    static PyObject *__mod__(PyObject *me, PyObject *other);
    static PyObject *__divmod__(PyObject *me, PyObject *other);
    static PyObject *__pow__(PyObject *me, PyObject *other, PyObject *mod);
    PyObject *__abs__() const;
    PyObject *__bool__() const;

    static PyObject *__get__varType(TPyValue *me);

    PyObject *native() const PYARGS(METH_NOARGS, "converts the value to a string, number or other suitable presentation in Python");
    PyObject *isUndefined() const PYARGS(METH_NOARGS, "returns True if value is undefined");
    PyObject *isSpecial() const PYARGS(METH_NOARGS, "(obsolete) returns True if value is undefined");
    PyObject *is_DC() const PYARGS(METH_NOARGS, "(obsolete) returns True if value is undefined");
    PyObject *is_DK() const PYARGS(METH_NOARGS, "(obsolete) returns True if value is undefined");

    static int argconverter(PyObject *obj, TValue *addr);
};

#endif

