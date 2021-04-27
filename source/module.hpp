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


#ifndef __ORANGE_MODULE_HPP
#define __ORANGE_MODULE_HPP

extern PyObject *orangeModule;
extern PyObject *orangeVersion;
extern PyObject *VarTypes;

PYMODULECONSTANT(version, orangeVersion)
PYMODULECONSTANT(VarTypes, VarTypes)

PYCONSTANT_FLOAT(Illegal_Float, ILLEGAL_FLOAT)

// Returns a borrowed reference!
inline PyObject *getExportedFunction(const char *func)
{ return PyDict_GetItemString(PyModule_GetDict(orangeModule), func); }

// Returns a borrowed reference!
inline PyObject *getExportedFunction(PyObject *module, const char *func)
{ return PyDict_GetItemString(PyModule_GetDict(module), func); }


// field 'value' is a bit redundant since 'obj' is expected to be an instance
// of long having the same value. But let us keep it for compatibility.
typedef struct {
    char *name;
    long value;
    PyObject *obj;
} TNamedConstantsDef;

typedef struct {
    size_t offset;
    TNamedConstantsDef const *namedConstants;
} TNamedConstantsFieldClosure;


PyObject *getter_namedConstant(PyObject *, TNamedConstantsFieldClosure *);
int setter_namedConstant(PyObject *, PyObject *, TNamedConstantsFieldClosure *);

// PyObject *unpickleConstant(TNamedConstantRecord const *, PyObject *args);

PyObject *get_constantName(PyObject *self, TNamedConstantsDef ncs[]);
PyObject *get_constantObject(const long value, TNamedConstantsDef ncs[]);

PyObject *getter_namedConstantConstant(PyObject *, PyObject *);

PyObject *reduceNamedConstant(PyObject *self);
PYMODULEFUNCTION PyObject *__unpickleNamedConstant(PyObject *self, PyObject *args) PYARGS(METH_VARARGS, "(type, value)");

PyObject *getnewargs__no_pickling(PyObject *);
PyObject *getnewargs_fromList(PyObject *self, char *attrs[]);

#endif
