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
#include "constantclassifier.px"


TConstantClassifier::TConstantClassifier(PVariable const &acv) 
: TClassifier(acv),
  defaultVal(UNDEFINED_VALUE),
  defaultDistribution(TDistribution::create(acv))
{}


TConstantClassifier::TConstantClassifier(PVariable const &acv,
                                         PDistribution const &defDis)
: TClassifier(acv ? acv : defDis->variable),
  defaultVal(UNDEFINED_VALUE),
  defaultDistribution(defDis)
{}


TConstantClassifier::TConstantClassifier(PVariable const &acv,
                                         TValue const defVal,
                                         PDistribution const &defDis)
: TClassifier(acv),
  defaultVal(defVal),
  defaultDistribution(defDis)
{
    if (!defDis) {
        defaultDistribution = TDistribution::create(acv);
        if (!isnan(defVal)) {
            defaultDistribution->add(defVal);
        }
    }
}


TConstantClassifier::TConstantClassifier(const TConstantClassifier &old)
: TClassifier(dynamic_cast<const TClassifier &>(old)),
  defaultVal(old.defaultVal),
  defaultDistribution(CLONE(PDistribution, old.defaultDistribution))
{}


TValue TConstantClassifier::operator ()(TExample const *const exam)
{ 
    return isnan(defaultVal) ? defaultDistribution->predict(exam) : defaultVal;
}


PDistribution TConstantClassifier::classDistribution(TExample const *const)
{ 
    return CLONE(PDistribution, defaultDistribution);
}


void TConstantClassifier::predictionAndDistribution(TExample const *const exam,
                                                    TValue &val,
                                                    PDistribution &dist)
{ 
    val = operator()(exam);
    dist = CLONE(PDistribution, defaultDistribution);
}


TOrange *TConstantClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    // Legacy arguments
    PyObject *arg1 = NULL, *arg2 = NULL;
    if ((!kw || !PyDict_Size(kw))
        && PyArg_UnpackTuple(args, "ConstantClassifier.__new__", 1, 2, &arg1, &arg2)) {
        if (!arg2) {
            if (OrVariable_Check(arg1)) {
                PVariable var(arg1);
                return new (type)TConstantClassifier(var);
            }
            if (OrPyValue_Check(arg1)) {
                TPyValue const &pyvalue = (TPyValue &)((OrOrange *)arg1)->orange;
                if (!pyvalue.variable) {
                    raiseError(PyExc_ValueError,
                        "cannot construct a ConstantClassifier with a value without a variable");
                }
                return new (type)TConstantClassifier(pyvalue.variable, (TValue)pyvalue.value);
            }
            if (OrDistribution_Check(arg1)) {
                PDistribution dist = PDistribution(arg1);
                if (!dist->variable) {
                    raiseError(PyExc_ValueError,
                        "cannot construct a ConstantClassifier with a value without a variable");
                }
                return new (type)TConstantClassifier(dist->variable, dist);
            }
        }
        else {
            if (OrVariable_Check(arg1)) {
                PVariable var(arg1);
                if (!var->isPrimitive()) {
                    raiseError(PyExc_ValueError,
                        "classifiers can predict only primitive values");
                }
                TValue val = var->py2val(arg2);
                return new (type)TConstantClassifier(var, val);
            }
        }
    }
    PyErr_Clear();

    PVariable var;
    PyObject *pyvalue = NULL;
    PDistribution dist;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O&OO&:ConstantClassifier",
        ConstantClassifier_keywords,
        &PVariable::argconverter_n, &var,
        &pyvalue,
        &PDistribution::argconverter_n, &dist)) {
            return NULL;
    }
    if (pyvalue && OrPyValue_Check(pyvalue)) {
        TPyValue const &value = (TPyValue &)((OrOrange *)pyvalue)->orange;
        if (var) {
            if (value.variable && (value.variable != var)) {
                raiseError(PyExc_ValueError,
                    "value corresponds to variable '%s', not '%s'",
                    value.variable->cname(), var->cname());
            }
        }
        else {
            var = value.variable;
        }
    }
    if (dist) {
        if (var) {
            if (dist->variable && (dist->variable != var)) {
                raiseError(PyExc_ValueError,
                    "distribution corresponds to variable '%s', not '%s'",
                    dist->variable->cname(), var->cname());
            }
        }
        else {
            var = dist->variable;
        }
    }
    if (!var) {
        raiseError(PyExc_ValueError, "cannot deduce variable from arguments");
    }
    if (!var->isPrimitive()) {
        raiseError(PyExc_ValueError,
            "classifiers can predict only primitive values");
    }
    TValue val = pyvalue ? var->py2val(pyvalue) : UNDEFINED_VALUE;
    return new (type)TConstantClassifier(var, val, dist);
}


PyObject *TConstantClassifier::__get__defaultVal(PyObject *self)
{
    try {
        PConstantClassifier me(self);
        return PyObject_FromNewOrange(new TPyValue(me->classVar, me->defaultVal));
    }
    PyCATCH;
}
    
int TConstantClassifier::__set__defaultVal(PyObject *self, PyObject *pyvalue)
{
    try {
        PConstantClassifier me(self);
        if (me->classVar) {
            me->defaultVal = me->classVar->py2val(pyvalue);
        }
        else {
            me->defaultVal = PyNumber_AsDouble(pyvalue);
        }
        return 0;
    }
    PyCATCH_1;
}
