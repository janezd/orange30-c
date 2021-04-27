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


#ifndef __STRINGVARIABLE_HPP
#define __STRINGVARIABLE_HPP

#include "stringvariable.ppp"

/*! Descriptor for string variables. String variables can be used only
    for meta attributes. */
class TStringVariable : public TVariable {
public:
    __REGISTER_CLASS(StringVariable);

    TStringVariable(string const &aname="");

    virtual string pyval2str(PyObject *val) const;
    virtual PyObject *str2pyval(string const &valname) const;
    virtual PyObject *py2pyval(PyObject *pyval) const;

    /// @cond Python
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *keywords) PYDOC("([name]) -> StringVariable");
    PyObject *__richcmp__(PyObject *other, int op);
    /// @endcond
};

#endif
