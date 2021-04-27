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
#include "distribution.px"


TDistribution::TDistribution()
: unknowns(0.0),
  abs(0.0),
  cases(0.0),
  normalized(false),
  supportsContinuous(false)
{}


TDistribution::TDistribution(PVariable const &var)
: variable(var),
  unknowns(0.0),
  abs(0.0),
  cases(0.0),
  normalized(false),
  supportsContinuous(false)
{}


TDistribution::TDistribution(TDistribution const &other)
: variable(other.variable),
  unknowns(other.unknowns),
  abs(other.abs),
  cases(other.cases),
  normalized(other.normalized),
  supportsContinuous(other.supportsContinuous)
{}


TDistribution &TDistribution::operator =(TDistribution const &other)
{
    variable = other.variable;
    unknowns = other.unknowns;
    abs = other.abs;
    cases = other.cases;
    normalized = other.normalized;
    supportsContinuous = other.supportsContinuous;
    return *this;
}

PDistribution TDistribution::create(PVariable const &var)
{ 
    if (!var) {
        raiseError(PyExc_ValueError,
            "variable is not given");
    }
    if (!var->isPrimitive()) {
        raiseError(PyExc_TypeError,
            "cannot construct distributions of non-primitive variable '%s'",
            var->cname());
    }
    if (var->varType==TVariable::Discrete) {
        return PDistribution(new TDiscDistribution(var));
    }
    else {
        return PDistribution(new TContDistribution(var));
    }
}


PDistribution TDistribution::fromExamples(PExampleTable const &gen,
                                          TAttrIdx const position)
{
    if (position >= gen->domain->variables->size()) {
        raiseError(PyExc_IndexError, "index %i out of range", position);
    }
    PVariable const &var = gen->domain->variables->at(position);
    PDistribution dist;
    if (var->varType == TVariable::Discrete) {
        dist = PDistribution(new TDiscDistribution(var));
    }
    else {
        dist = PDistribution(new TContDistribution(var));
    }
    TDistribution *const ddist = dist.borrowPtr();
    PEITERATE(ei, gen) {
        ddist->add(ei.value_at(position), ei.getWeight());
    }
    return dist;
}


PDistribution TDistribution::fromExamples(PExampleTable const &gen,
                                          PVariable const &var)
{
    int position = gen->domain->getVarNum(var, false);
    if (position == ILLEGAL_INT) {
        raiseError(PyExc_NotImplementedError,
            "domain conversion is not implemented yet");
    }
    PDistribution dist = create(var);
    TDistribution *const ddist = dist.borrowPtr();
    PEITERATE(ei, gen) {
        ddist->add(ei.value_at(position), ei.getWeight());
    }
    return dist;
}


#define NOT_IMPLEMENTED(x) { raiseError(PyExc_SystemError, "'%s' is not implemented", x); throw 0; /*just to avoid warnings*/ }

TDistribution &TDistribution::operator += (TDistribution const &)
NOT_IMPLEMENTED("+=")

TDistribution &TDistribution::operator -= (TDistribution const &)
NOT_IMPLEMENTED("-=")

TDistribution &TDistribution::operator *= (double const)
NOT_IMPLEMENTED("*=")

double TDistribution::average() const
NOT_IMPLEMENTED("average()")

double TDistribution::dev() const
NOT_IMPLEMENTED("dev()")

double TDistribution::var() const
NOT_IMPLEMENTED("dev()")

double TDistribution::error() const
NOT_IMPLEMENTED("error()")

double TDistribution::percentile(double const &) const
NOT_IMPLEMENTED("percentile(double)")


TDistribution *TDistribution::setterConversion(PyObject *obj)
{
    if (PyDict_Check(obj)) {
        map<double, double> dd;
        if (convertFromPython(obj, dd)) {
            return new TContDistribution(dd);
        }
    }
    vector<double> dd;
    if (convertFromPython(obj, dd)) {
        return new TDiscDistribution(dd);
    }
    return NULL;
}


TOrange *TDistribution::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
Represents a distribution of a values of a discrete or continuous variable.

The class is abstract and the constructor acutally return an instance of
either DiscDistribution or ContDistribution, depending on the variable type.
If the variable is the only argument, it must be an instance of
Orange.data.variable.Variable. In that case, an empty distribution is
constructed. If data is given as well, the variable can also be specified
by name or index in the domain. Constructor then computes the distribution
of the specified variable on the given data.

If variable is given by descriptor, it doesn't need to exist in the domain,
but it must be computable from given instances. For example, the variable
can be a discretized version of a variable from data.
*/
{
    if (args && (PyTuple_GET_SIZE(args)==1)) {
        PyObject *arg = PyTuple_GET_ITEM(args, 0);
        if (OrDistribution_Check(arg)) {
            if ((type != &OrDistribution_Type) &&
                !PyType_IsSubtype(type, arg->ob_type)) {
                    raiseError(PyExc_TypeError,
                        "Cannot convert from '%s' to '%s'",
                        arg->ob_type->tp_name, type->tp_name);
            }
            return PDistribution(arg)->clone();
        }
    }
    PyObject *pyvar;
    PExampleTable table;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O&:Distribution",
        Distribution_keywords,
        &pyvar, &PExampleTable::argconverter, &table)) {
            return NULL;
    }
    PDistribution dist;
    if (table) {
        TAttrIdx pos = table->domain->getVarNum(pyvar, false);
        if (pos != ILLEGAL_INT) {
            if (pos == -1) {
                if (!table->domain->classVar) {
                    raiseError(PyExc_ValueError,
                        "data set has no class attribute");
                }
                pos = table->domain->attributes->size();
            }
            dist = fromExamples(table, pos);
        }
        else {
            PVariable var = table->domain->getVar(pyvar, true, true, false);
            dist = fromExamples(table, var);
        }
    }
    else {
        if (!OrVariable_Check(pyvar)) {
            raiseError(PyExc_TypeError,
                "expected an instance of Variable, got '%s'",
                pyvar->ob_type->tp_name);
        }
        dist = create(PVariable(pyvar));
    }
    return dist.getPtr();
}


Py_ssize_t TDistribution::__len__() const
{
    return size();
}


TValue TDistribution::valueFromPyObject(PyObject *index) const
{
    if (variable) {
        return variable->py2val(index);
    }
    return PyNumber_AsDouble(index, "indices must be values, not '%s'");
}


PyObject *TDistribution::__subscript__(PyObject *index)
{
    const TValue val = valueFromPyObject(index);
    return PyFloat_FromDouble(at(val));
}


int TDistribution::__ass_subscript__(PyObject *index, PyObject *value)
{
    const TValue val = valueFromPyObject(index);
    const double weight = 
        PyNumber_AsDouble(value, "distribution contains numbers, not '%s'");
    set(val, weight);
    return 0;
}


PyObject *TDistribution::py_add(PyObject *args, PyObject *kw)
{
    PyObject *index;
    double weight = 1.0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|d:add",
        Distribution_add_keywords, &index, &weight)) {
            return NULL;
    }
    const TValue val = valueFromPyObject(index);
    add(val, weight);
    Py_RETURN_NONE;
}


PyObject *TDistribution::py_normalize()
{
    normalize();
    Py_RETURN_NONE;
}


PyObject *TDistribution::modus() const
{
    return PyObject_FromNewOrange(new TPyValue(variable, highestProbValue()));
}

PyObject *TDistribution::random() const
{
    return PyObject_FromNewOrange(new TPyValue(variable, randomValue()));
}


PDistribution getClassDistribution(PExampleTable const &gen)
{ 
    if (!gen->size()) {
        raiseError(PyExc_ValueError, "no examples");
    }
    PVariable classVar = gen->domain->classVar;
    if (!classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    PDistribution classDist = TDistribution::create(classVar);
    TDistribution *const udist = classDist.borrowPtr();
    PEITERATE(ei, gen) {
        udist->add(ei.getClass(), ei.getWeight());
    }
    return classDist;
}


PyObject *py_getClassDistribution(PyObject *, PyObject *args, PyObject *kw)
{
    PExampleTable table;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:get_class_distribution",
        getClassDistribution_keywords, &PExampleTable::argconverter, &table)) {
            return NULL;
    }
    return getClassDistribution(table).toPython();
}
