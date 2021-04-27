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

#ifndef __ERRORS_HPP
#define __ERRORS_HPP

/*! \file
    Functions and macros related to raising exceptions and converting them
    to Python errors.
*/

/*! Exception class thrown by #raiseError. The exception stores a Python
    exception object and can be "restored" into a Python exception. */
class PyException : public std::exception {
public:
    PyObject *type;    ///< Python exception type
    PyObject *value;   ///< Error message as Python string
    PyObject *tracebk; ///< Python trace, if available

   PyException(); 
   PyException(PyObject *atype, PyObject *avalue, PyObject *atrace);
   PyException(PyObject *atype, const char *des);
   PyException(PyException const &old);
   PyException &operator =(PyException const &old);
   ~PyException() throw();

   virtual const char* what () const throw ();
   void restore();
};


/*! Raises a PyException with the given type and message. \c anerr can be
    a C format string. */
void raiseError(PyObject *type, const char *anerr, ...);

/// The pickling error exception imported from module \c pickle
extern PyObject *PyExc_PicklingError;

/// The unpickling error exception imported from module \c pickle
extern PyObject  *PyExc_UnpicklingError;

/// Catches a PyException, restores it and returns \c r
#define PyCATCH_r(r)  catch (PyException err) { err.restore(); return r; }

/// Catches a PyException, restores it and returns \c NULL
#define PyCATCH PyCATCH_r(NULL)

/// Catches a PyException, restores it and returns \c -1
#define PyCATCH_1 PyCATCH_r(-1)

#endif

