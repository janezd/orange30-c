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

/*! \file
    Functions and classes related to raising exceptions and converting them
    to Python errors.
*/

char excbuf[512], excbuf2[512];

void raiseError(PyObject *type, const char *anerr, ...)
{  
    va_list vargs;
    #ifdef HAVE_STDARG_PROTOTYPES
        va_start(vargs, anerr);
    #else
        va_start(vargs);
    #endif
    vsnprintf(excbuf, 512, anerr, vargs);
    throw PyException(type, excbuf);
}


/*! Constructs an exception by fetching a current Python exception */
PyException::PyException()
{ PyErr_Fetch(&type, &value, &tracebk); }


/*! Constructs an exception from Python objects */
PyException::PyException(PyObject *atype, PyObject *avalue, PyObject *atrace)
: type(atype), 
  value(avalue),
  tracebk(atrace)
{
    Py_XINCREF(type);
    Py_XINCREF(value);
    Py_XINCREF(tracebk);
}

/*! Constructs an exception with the given type and error message, and with
    no stack trace. */
PyException::PyException(PyObject *atype, const char *msg)
: type(atype),
  value(PyUnicode_FromString(msg)),
  tracebk(NULL)
{
    Py_XINCREF(type);
}

/*! Copy constructor */
PyException::PyException(PyException const &old)
: type(old.type),
  value(old.value),
  tracebk(old.tracebk)
{
    Py_XINCREF(type);
    Py_XINCREF(value);
    Py_XINCREF(tracebk);
}

/*! Copy operator */
PyException &PyException::operator =(PyException const &old)
{
    if (this == &old) {
        return *this;
    }
    Py_XINCREF(old.type);
    Py_XINCREF(old.value);
    Py_XINCREF(old.tracebk);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tracebk);
    type = old.type;
    value = old.value;
    tracebk = old.tracebk;
    return *this;
}


PyException::~PyException() throw()
{
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tracebk);
}


/*! Returns an error message from #value */
const char* PyException::what () const throw ()
{ 
    PyObject *str = PyObject_Str(value);
    if (str) {
        return PyBytes_AsString(str); 
    }
    else {
        return "Unidentified Python exception"; 
    }
}

/*! Restores a Python exception */
void PyException::restore()
{ 
    Py_XINCREF(type);
    Py_XINCREF(value);
    Py_XINCREF(tracebk);
    PyErr_Restore(type, value, tracebk); 
}


// Loads pickle error from module pickle
bool getPickleErrors()
{
    PyExc_PicklingError = PyExc_UnpicklingError = NULL;
    PyObject *pickleModule = PyImport_ImportModule("pickle");
    if (pickleModule) {
        PyObject *dict = PyModule_GetDict(pickleModule);
        if (dict) {
            PyExc_PicklingError = PyDict_GetItemString(dict, "PicklingError");
            PyExc_UnpicklingError = PyDict_GetItemString(dict, "UnpicklingError");
        }
    }
    if (!PyExc_PicklingError) {
        PyExc_PicklingError = PyExc_ValueError;
    }
    if (!PyExc_UnpicklingError) {
        PyExc_UnpicklingError = PyExc_ValueError;
    }
    Py_INCREF(PyExc_PicklingError);
    Py_INCREF(PyExc_UnpicklingError);
    return true;
}

PyObject *PyExc_PicklingError, *PyExc_UnpicklingError;
static bool foo = getPickleErrors();


// The code below is related to warnings and will probably be removed


bool setFilterWarnings(PyObject *filterFunction, char *action, char *message, PyObject *warning, char *moduleName)
{
    PyObject *args = Py_BuildValue("ssOs", action, message, warning, moduleName);
    PyObject *res = PyObject_CallObject(filterFunction, args);
    Py_DECREF(args);
    if (!res) {
      return false;
    }
    Py_DECREF(res);
    return true;
}


