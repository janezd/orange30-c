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
#include "stringvariable.px"

/// Constructs a variable with the given name
TStringVariable::TStringVariable(string const &aname)
: TVariable(aname, TVariable::String)
{}

/*! Converts a Python object to a string; the object must by an instance of
    \c PyUnicode or \c None, which is interpreted as unknown and represented
    with a string "?". */
string TStringVariable::pyval2str(PyObject *val) const
{
    if (!val || (val == Py_None)) {
        return "?";
    }
    // We need this check since the descriptor can be added to the domain later!
    if (!PyUnicode_Check(val)) {
        raiseError(PyExc_TypeError,
            "Values of '%s' should be strings, not '%s'",
            cname(), val->ob_type->tp_name);
    }
    return PyUnicode_As_string(val);
}

/*! Converts a string to Python unicode. */
PyObject *TStringVariable::str2pyval(string const &valname) const
{
    return PyUnicode_FromString(valname.c_str());
}

/*! Converts a value represented as PyObject to PyObject; it checks that
    the type is \c PyUnicode_Type (raises an exception if not) and increases
    the object's reference count. */
PyObject *TStringVariable::py2pyval(PyObject *val) const
{
    if (!PyUnicode_Check(val)) {
        raiseError(PyExc_TypeError,
            "Values of '%s' should be strings, not '%s'",
            cname(), val->ob_type->tp_name);
    }
    Py_INCREF(val);
    return val;
}

/// @cond Python
TOrange *TStringVariable::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_Size(args) == 5)) {
        return TVariable::__new__(type, args, kw);
    }
    PyObject *name = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O!:StringVariable",
        StringVariable_keywords, &PyUnicode_Type, &name)) {
        return NULL;
    }
    return new (type)TStringVariable(name ? PyUnicode_As_string(name) : "");
}

#include "filter.hpp"

PyObject *TStringVariable::__richcmp__(PyObject *other, int op)
{
    if (OrVariable_Check(other)) {
        return TVariable::__richcmp__(other, op);
    }

    // Filter for a list of values
    // unicode is not the only legal type for a single value,
    // but it's the only for which we can get an iterator
    if (((op == Py_EQ) || (op == Py_NE)) && !PyUnicode_Check(other)) {
        PStringList values;
        if (PStringList::argconverter(other, &values)) {
            PFilter_values filter(new TFilter_values());
            filter->addCondition(
                PVariable::fromBorrowedPtr(this), values, op == Py_NE);
            return filter.getPyObject();
        }
        PyErr_Clear();
    }

    if (!PyUnicode_Check(other)) {
        return PyErr_Format(PyExc_TypeError,
            "invalid operator for comparison with a list of strings");
    }
    string ref = PyUnicode_As_string(other);
    int myop = TValueFilter::operatorFromPy(op);
    if (myop == TValueFilter::None) {
        return NULL;
    }
    PFilter_values filter(new TFilter_values());
    filter->addCondition(PVariable::fromBorrowedPtr(this), myop, ref, string());
    return filter.getPyObject();
}
/// @endcond