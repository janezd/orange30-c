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
#include "pyvalue.px"

TPyValue::TPyValue(PVariable const &var)
: variable(var)
{
    if (isPrimitive()) {
        value = UNDEFINED_VALUE;
    }
    else {
        object = NULL;
    }
}

TPyValue::TPyValue(PVariable const &var, TValue const val)
: variable(var),
  value(val)
{
    if (!isPrimitive()) {
        raiseError(PyExc_TypeError,
            "Non-primitive variable '%s' initialized with a primitive value",
            var->cname());
    }
}

TPyValue::TPyValue(PVariable const &var, PyObject *obj)
: variable(var),
  object(obj)
{
    Py_XINCREF(obj);
    if (!var) {
        raiseError(PyExc_TypeError,
            "Non-primitive values need to be associated with a variable");
    }
    if (isPrimitive()) {
        raiseError(PyExc_TypeError,
            "Primitive variable '%s' cannot be initialized with a Python object",
            var->cname());
    }
}

TPyValue::TPyValue(TPyValue const &val)
: variable(val.variable)
{
    if (isPrimitive()) {
        value = val.value;
    }
    else {
        object = val.object;
        Py_XINCREF(object);
    }
}

TPyValue::TPyValue(PVariable const &var, TMetaValue const &val)
: variable(var)
{
    if (isPrimitive() != val.isPrimitive) {
        raiseError(PyExc_TypeError,
            "Invalid value (primitive value assigned to non-primitive variable and vice versa)");
    }
    if (isPrimitive()) {
        value = val.value;
    }
    else {
        object = val.object;
        Py_XINCREF(object);
    }
}


TPyValue::~TPyValue()
{
    if (!isPrimitive())
        Py_XDECREF(object);
}


TOrange *TPyValue::__new__(PyTypeObject *type, PyObject *args, PyObject *)
{
    PVariable var;
    PyObject *pyval = NULL;
    if (!PyArg_ParseTuple(args, "O&|O:Value",
        &PVariable::argconverter_n, &var, &pyval)) {
        return NULL;
    }
    /* PyValue: check the variable type and copy the value if it matches */
    if (pyval && OrPyValue_Check(pyval)) {
        TPyValue &val = ((OrPyValue *)(pyval))->orange;
        if (val.variable && var && (val.variable != var)) {
            PyErr_Format(PyExc_TypeError, "excepted a value of '%s', not '%s'",
                var->cname(), val.variable->cname());
            return NULL;
        }
        return new (type)TPyValue(val);
    }
    /* Non-primitive variables handle everything themselves, except for PyValues */
    if (var && !var->isPrimitive()) {
        return new(type) TPyValue(var, var->py2pyval(pyval));
    }
    /* Undefined values: pass the variable to TValue */
    if (!pyval || (pyval == Py_None)) {
        return new (type) TPyValue(var);
    }
    /* Discrete and continuous variables, and we got a number */
    if (PyNumber_Check(pyval)) {
        TValue const val = PyNumber_AsDouble(pyval);
        return new (type) TPyValue(var, val);
    }
    if (!var) {
        PyErr_Format(PyExc_TypeError,
            "Non-numeric values need to be associated with a variable");
        return NULL;
    }
    if (PyUnicode_Check(pyval)) {
        TValue const val = var->str2val(PyUnicode_As_string(pyval));
        return new TPyValue(var, val);
    }
    /* If not a string, we convert it to a string (probably a bad idea, though) */
    PyObject *valAsStr = PyObject_Str(pyval);
    GUARD(valAsStr);
    TValue const val = var->str2val(PyUnicode_As_string(valAsStr));
    return new TPyValue(var, val);
}


PyObject *TPyValue::__getnewargs__() const
{
    if (!variable) {
        if (isnan(value)) {
            return Py_BuildValue("(O)", Py_None);
        }
        else {
            return Py_BuildValue("(d)", value);
        }
    }
    else if (variable->isPrimitive() ? isnan(value) : !object) {
        return Py_BuildValue("(N)", variable.toPython());
    }
    else if (variable->isPrimitive()) {
        return Py_BuildValue("(Nd)", variable.toPython(), value);
    }
    else {
        return Py_BuildValue("(NO)", variable.toPython(), object);
    }
}


// We won't let it have attributes in dict (the user might expect them to be saved!)
int TPyValue::__setattr__(PyObject *self, PyObject *name, PyObject *value)
{
    return Orange_GenericSetAttrNoDict(self, name, value);
}


PyObject *TPyValue::__repr__() const
{
    if (!variable) {
        PyObject *asDouble = PyFloat_FromDouble(value);
        GUARD(asDouble);
        return PyUnicode_FromFormat("%S", asDouble);
    }
    return PyUnicode_FromString(
        variable->isPrimitive() ? variable->val2str(value).c_str()
                                : variable->pyval2str(object).c_str());
}

long TPyValue::__hash__() const
{
    if (variable && !variable->isPrimitive()) {
        return PyObject_Hash(object);
    }
    return _Py_HashDouble(value);
}


PyObject *compare_op(const int cmpr, const int op)
{
    if (   (cmpr < 0) && ((op == Py_LT) || (op == Py_LE) || (op == Py_NE))
        || (cmpr > 0) && ((op == Py_GT) || (op == Py_GE) || (op == Py_NE))
        || (cmpr ==0) && ((op == Py_LE) || (op == Py_GE) || (op == Py_EQ))) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

PyObject *TPyValue::__richcmp__(PyObject *other, int op) const
{
    PVariable otherVar = OrPyValue_Check(other)
        ? ((OrPyValue *)other)->orange.variable
        : PVariable();
    if (OrPyValue_Check(other) && (variable != otherVar)) {
        return PyErr_Format(PyExc_TypeError,
            "Cannot compare values of two different variables ('%s' and '%s')",
            variable ? variable->cname() : "",
            otherVar ? otherVar->cname() : "");
    }
    if (!variable || variable->isPrimitive()) {
        if (OrPyValue_Check(other)) {
            return compare_op(values_compare(value,
                ((OrPyValue *)other)->orange.value), op);
        }
        else if (PyNumber_Check(other)) {
            TValue const oval = PyNumber_AsDouble(other);
            return compare_op(values_compare(value, oval), op);
        }
        else if (variable) {
            PyObject *args = PyTuple_Pack(2, variable.borrowPyObject(), other);
            GUARD(args);
            PyObject *otherval = OrPyValue_Type.tp_new(&OrPyValue_Type, args, NULL);
            if (!otherval) {
                return NULL;
            }
            TValue const oval = ((OrPyValue *)otherval)->orange.value;
            Py_DECREF(otherval);
            int cmpr = values_compare(value, oval);
            return compare_op(cmpr, op);
        }
        else {
            return PyErr_Format(PyExc_TypeError,
                "Cannot compare an instance of '%s' with a value that is not "
                "associated with a variable", other->ob_type->tp_name);
        }
    }
    else { /* Variable is not primitive */
        PyObject *deref = NULL;
        if (!OrPyValue_Check(other)) {
            PyObject *args = PyTuple_Pack(2, variable.borrowPyObject(), other);
            GUARD(args);
            other = deref = OrPyValue_Type.tp_new(&OrPyValue_Type, args, NULL);
            if (!other) {
                return NULL;
            }
        }
        GUARD(deref);
        PyObject *myvalue = object;
        PyObject *hisvalue = ((OrPyValue *)other)->orange.object;
        if (!myvalue || !hisvalue) {
            int cmpr;
            if (myvalue) {
                cmpr = -1;
            }
            else if (hisvalue) {
                cmpr = 1;
            }
            else {
                cmpr = 0;
            }
            return compare_op(cmpr, op);
        }
        PyObject *res = PyObject_RichCompare(myvalue, hisvalue, op);
        return res;
    }
}    


extern TNamedConstantsFieldClosure Variable_varType_member_closure;

PyObject *TPyValue::__get__varType(TPyValue *me)
{
    if (!me->variable) {
        raiseError(PyExc_ValueError, "unknown value type");
    }
    return getter_namedConstant(AS_PyObject(me),
        &Variable_varType_member_closure);
}


PyObject *TPyValue::native() const
{
    if (!variable) {
        return PyFloat_FromDouble(value);
    }
    else if (variable->isPrimitive()) {
        return variable->val2py(value);
    }
    else {
        Py_INCREF(object);
        return object;
    }
}


PyObject *TPyValue::isUndefined() const
{
    return PyBool_FromBool((!variable || variable->isPrimitive()) ?
        isnan(value)!=0 : object==NULL);
}

PyObject *TPyValue::isSpecial() const
{
    return isUndefined();
}

PyObject *TPyValue::is_DC() const
{
    return isUndefined();
}

PyObject *TPyValue::is_DK() const
{
    return isUndefined();
}


PyObject *TPyValue::__int__() const
{
    if (variable && !variable->isPrimitive()) {
        return PyErr_Format(PyExc_TypeError,
            "Variable '%s' is not primitive", variable->cname());
    }
    if (isnan(value)) {
        if (variable) {
            return PyErr_Format(PyExc_TypeError,
                "Value of '%s' is undefined", variable->cname());
        }
        else {
            return PyErr_Format(PyExc_TypeError, "Value is undefined");
        }
    }
    return PyLong_FromLong(value);
}


PyObject *TPyValue::__float__() const
{
    if (variable && !variable->isPrimitive()) {
        return PyErr_Format(PyExc_TypeError,
            "Variable '%s' is not primitive", variable->cname());
    }
    if (isnan(value)) {
        if (variable) {
            return PyErr_Format(PyExc_TypeError,
                "Value of '%s' is undefined", variable->cname());
        }
        else {
            return PyErr_Format(PyExc_TypeError, "Value is undefined");
        }
    }
    return PyFloat_FromDouble(value);
}


bool toDouble(PyObject *obj, double &d, PyObject *&o, char *msg)
{
    if (OrPyValue_Check(obj)) {
        TPyValue &val = ((OrPyValue *)obj)->orange;
        if (!val.variable || val.variable->isPrimitive()) {
            d = val.value;
            o = NULL;
            return true;
        }
        else {
            o = val.object;
            return false;
        }
    }
    else if (PyNumber_Check(obj)) {
        d = PyNumber_AsDouble(obj, msg);
        o = NULL;
        return true;
    }
    else {
        raiseError(PyExc_TypeError, msg, obj->ob_type->tp_name);
    }
    return true;
}

PyObject *TPyValue::__add__(PyObject *me, PyObject *other)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot add %s and Value"))
            != toDouble(other, v2, o2, "cannot add Value and %s")) {
            raiseError(PyExc_TypeError, "cannot add primitive and non-primitive values");
        }
        if (isDouble) {
            return PyFloat_FromDouble(v1+v2);
        }
        else {
            return PyNumber_Add(o1, o2);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__sub__(PyObject *me, PyObject *other)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot subtract Value from %s"))
            != toDouble(other, v2, o2, "cannot add subtract %s from Value")) {
            raiseError(PyExc_TypeError, "cannot subtract primitive and non-primitive values");
        }
        if (isDouble) {
            return PyFloat_FromDouble(v1-v2);
        }
        else {
            return PyNumber_Subtract(o1, o2);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__mul__(PyObject *me, PyObject *other)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot multiple %s with Value"))
            != toDouble(other, v2, o2, "cannot add subtract Value with %s")) {
            raiseError(PyExc_TypeError, "cannot multiply primitive and non-primitive values");
        }
        if (isDouble) {
            return PyFloat_FromDouble(v1*v2);
        }
        else {
            return PyNumber_Multiply(o1, o2);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__mod__(PyObject *me, PyObject *other)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot divide %s with Value"))
            != toDouble(other, v2, o2, "cannot add divide Value with %s")) {
            raiseError(PyExc_TypeError, "cannot divide primitive and non-primitive values");
        }
        if (isDouble) {
            if (fabs(v2) < 1e-6) {
                raiseError(PyExc_ZeroDivisionError, "division by zero");
            }
            return PyFloat_FromDouble(fmod(v1, v2));
        }
        else {
            return PyNumber_Remainder(o1, o2);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__divmod__(PyObject *me, PyObject *other)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot divide %s with Value"))
            != toDouble(other, v2, o2, "cannot add divide Value with %s")) {
            raiseError(PyExc_TypeError, "cannot divide primitive and non-primitive values");
        }
        if (isDouble) {
            if (fabs(v2) < 1e-6) {
                raiseError(PyExc_ZeroDivisionError, "division by zero");
            }
            double d = floor(v1/v2);
            return Py_BuildValue("(dd)", d, v1-d*v2);
        }
        else {
            return PyNumber_Divmod(o1, o2);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__pow__(PyObject *me, PyObject *other, PyObject *mod)
{
    try {
        double v1, v2;
        PyObject *o1, *o2;
        bool isDouble;
        if ((isDouble = toDouble(me, v1, o1, "cannot divide %s with Value"))
            != toDouble(other, v2, o2, "cannot add divide Value with %s")) {
            raiseError(PyExc_TypeError, "cannot divide primitive and non-primitive values");
        }
        if (isDouble) {
            if (mod != Py_None) {
                raiseError(PyExc_ValueError, "PyValue.__pow__ accepts only two arguments");
            }
            if (fabs(v2) < 1e-6) {
                raiseError(PyExc_ZeroDivisionError, "division by zero");
            }
            return PyFloat_FromDouble(pow(v1, v2));
        }
        else {
            return PyNumber_Power(o1, o2, mod);
        }
    }
    PyCATCH
}


PyObject *TPyValue::__abs__() const
{
    if (!variable || variable->isPrimitive()) {
        return PyFloat_FromDouble(fabs(value));
    }
    else {
        return PyNumber_Absolute(object);
    }
}

PyObject *TPyValue::__bool__() const
{
    return PyBool_FromBool((variable || variable->isPrimitive()) 
        ? (!isnan(value) && (value != 0))
        : (PyObject_IsTrue(object) != 0));
}


int TPyValue::argconverter(PyObject *obj, TValue *addr)
{
    if (OrPyValue_Check(obj)) {
        TPyValue &val = ((OrPyValue *)obj)->orange;
        if (!val.variable || val.variable->isPrimitive()) {
            *addr = val.value;
            return 1;
        }
        else {
            PyErr_Format(PyExc_TypeError,
                "variable '%s' does not have primitive values",
                val.variable->cname());
            return 0;
        }
    }
    if (!PyNumber_ToDouble(obj, *addr)) {
        PyErr_Format(PyExc_TypeError,
            "expected a primitive value, got '%s'", obj->ob_type->tp_name);
        return 0;
    }
    return 1;
}
