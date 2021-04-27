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

PyObject *getter_namedConstant(PyObject *self, TNamedConstantsFieldClosure *closure)
{
    int &value = *(int *)((char *)(self)+closure->offset);
    for(TNamedConstantsDef const *ni = closure->namedConstants; ni->name; ni++) {
        if (ni->value == value) {
            Py_INCREF(ni->obj);
            return ni->obj;
        }
    }
    return PyLong_FromLong(value);
}

int setter_namedConstant(PyObject *self, PyObject *obj, TNamedConstantsFieldClosure *closure)
{
    for(TNamedConstantsDef const *ni = closure->namedConstants; ni->name; ni++) {
        if (ni->obj == obj) {
            *(int *)((char *)(self)+closure->offset) = ni->value;
            return 0;
        }
    }
    PyErr_Format(PyExc_TypeError, "invalid type ('%s')", obj->ob_type->tp_name);
    return -1;
}

PyObject *getter_namedConstantConstant(PyObject *, PyObject *cst)
{
    Py_INCREF(cst);
    return cst;
}

PyObject *get_constantName(PyObject *self, TNamedConstantsDef ncs[])
{
    const long val = PyLong_AsLong(self);
    for(TNamedConstantsDef const *ni = ncs; ni->name; ni++) {
        if (ni->value == val) {
            return PyUnicode_FromString(ni->name);
        }
    }
    return self->ob_type->tp_base->tp_repr(self);
}

PyObject *get_constantObject(const long val, TNamedConstantsDef ncs[])
{
    for(TNamedConstantsDef const *ni = ncs; ni->name; ni++) {
        if (ni->value == val) {
            return ni->obj;
        }
    }
    return NULL;
}


PyObject *reduceNamedConstant(PyObject *self)
{ 
    PyObject *unpickler = PyDict_GetItemString(
        PyModule_GetDict(orangeModule), "__unpickle_named_constant");
    long val = PyLong_AsLong(self);
    return Py_BuildValue("O(sN)",
        unpickler, self->ob_type->tp_name, PyObject_Repr(self));
}


static void getNext(PyObject *&o, char *sname)
{
    PyObject *nst;
    if (PyType_Check(o)) {
        nst = PyObject_GetAttrString(o, sname);
    }
    else if (PyModule_Check(o)) {
        nst = PyDict_GetItemString(PyModule_GetDict(o), sname);
        Py_XINCREF(nst);
    }
    Py_DECREF(o);
    o = nst;
}

PyObject *__unpickleNamedConstant(PyObject *self, PyObject *args)
{
    char *constname;
    char *valname;
    if (!PyArg_ParseTuple(args, "ss", &constname, &valname)) {
        return NULL;
    }
    ssize_t const slen = strlen(constname);
    char *nameCopy = strcpy((char *)malloc(slen+1), constname);
    PyObject *st = orangeModule;
    Py_INCREF(st);
    for(char *sname = nameCopy, *dot, *en = nameCopy+slen;
        st && sname != en; 
        sname = dot+1) {
            dot = strchr(sname, '.');
            if (dot) {
                *dot = 0;
            }
            else {
                dot = en-1;
            }
            getNext(st, sname);
            if (!st) {
                return NULL;
            }

    }
    getNext(st, valname);
    return st;
}


PyObject *orangeVersion = PyUnicode_FromString("3.0b ("__TIME__", "__DATE__")");


/*void tdidt_cpp_gcUnsafeInitialization();
void random_cpp_gcUnsafeInitialization();
void pythonVariables_unsafeInitializion();

void gcorangeUnsafeStaticInitialization()
{ tdidt_cpp_gcUnsafeInitialization();
  random_cpp_gcUnsafeInitialization();
  pythonVariables_unsafeInitializion();
}
*/


PyObject *orangeModule;

extern TNamedConstantsDef Variable_Type_constList[];

PyObject *constructVarTypes()
{
    PyObject *mod = PyModule_New("VarTypes");
    for(TNamedConstantsDef const *nc = Variable_Type_constList; nc->name; nc++) {
        Py_INCREF(nc->obj);
        PyModule_AddObject(mod, nc->name, nc->obj);
    }
    return mod;
}

PyObject *VarTypes = constructVarTypes();
    
    
PyObject *getnewargs__no_pickling(PyObject *obj)
{
    return PyErr_Format(PyExc_PicklingError,
        "instances of '%s' cannot be pickled", obj->ob_type->tp_name);
}

PyObject *getnewargs_fromList(PyObject *self, char *attrs[])
{
    char const *const *ai;
    int nattrs;
    for(nattrs = 0, ai = attrs; *ai; ai++, nattrs++);
    PyObject *args = PyTuple_New(nattrs);
    for(nattrs = 0, ai = attrs; *ai; ai++, nattrs++) {
        PyObject *val = PyObject_GetAttrString(self, *ai);
        if (!val) {
            Py_DECREF(args);
            return PyErr_Format(PyExc_SystemError,
                "pickling failed: attribute %s not found", *ai);
        }
        PyTuple_SET_ITEM(args, nattrs, val);
    }
    return args;
}




#include "initialization.px"
