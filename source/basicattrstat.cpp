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
#include "basicattrstat.px"

TBasicAttrStat::TBasicAttrStat(PContinuousVariable const &var, bool const ahold)
: variable(var),
  holdRecomputation(ahold)
{ 
    reset(); 
}


/*! Constructs an instance and computes the data about \c var from the given
    \c data. If the variable is not present in the data it must have
    #TVariable::getValueFrom that can compute the values from the existing
    attributes.

    For computing data on multiple attributes, use #TDomainBasicAttrStat. */
TBasicAttrStat::TBasicAttrStat(PExampleTable const &data,
                               PContinuousVariable const &var)
: variable(var),
  holdRecomputation(true)
{
    reset();
    int pos = data->domain->getVarNum(var, false);

    // Separate the cases to get faster code
    if (pos != ILLEGAL_INT) {
        if (!data->hasWeights()) {
            if (pos >= 0) {
                PEITERATE(ei, data) {
                    const TValue &val = ei.value_at(pos);
                    if (!isnan(val)) {
                        add(val);
                    }
                }
            }
            else {
                PEITERATE(ei, data) {
                    const TValue &val = ei.getMeta(pos);
                    if (!isnan(val)) {
                        add(val);
                    }
                }
            }
        }
        else {
            if (pos >= 0) {
                PEITERATE(ei, data) {
                    const TValue &val = ei.value_at(pos);
                    if (!isnan(val)) {
                        add(val, ei.getWeight());
                    }
                }
            }
            else {
                PEITERATE(ei, data) {
                    const TValue &val = ei.getMeta(pos);
                    if (!isnan(val)) {
                        add(val, ei.getWeight());
                    }
                }
            }
        }
    }
    else {
        if (var->getValueFrom) {
            PEITERATE(ei, data) {
                const TValue &val = var->computeValue(*ei);
                if (!isnan(val)) {
                    add(val, ei.getWeight());
                }
            }
        }
        else {
            raiseError(PyExc_ValueError,
                "data does not contain variable '%s'", var->cname());
        }
    }
    holdRecomputation = false;
    recompute();
}


/// Computes statistics from continuous distribution.
TBasicAttrStat::TBasicAttrStat(PContDistribution const &dist)
{
    if (dist->size()) {
        holdRecomputation = false;
        for(TContDistribution::iterator di(dist->begin()), de = dist->end(); di != de; di++) {
            add(di->first, di->second);
        }
        holdRecomputation = true;
        recompute();
    }
    else {
        reset();
    }
}


/*! Adds a value to #sum and #sum2, updates #min and #max, and recomputes
    #avg and #dev if #holdRecomputation is \c false.

    \todo
    The algorithm is numerically unstable. A better one, which also handles
    higher moments, is here:
    http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
*/
void TBasicAttrStat::add(const TValue val, const double weight)
{ 
    sum += weight*val;
    sum2 += weight*val*val;
    n += weight;
    if (!holdRecomputation && (n > 0)) {
        avg = sum/n;
        dev = sqrt(std::max(sum2/n - avg*avg, 0.0));
    }

    if (val < min) {
        min = val;
    }
    if (val > max) {
        max = val;
    }
}


/*! Computes #avg and #dev. This function needs to be called after reenabling
    computation. */
void TBasicAttrStat::recompute()
{ 
    if (n > 0) {
        avg = sum/n;
        dev = sqrt(std::max(sum2/n - avg*avg, 0.0));
    }
    else {
        avg = dev = -1;
    }
}


/*! Resets the statistics */
void TBasicAttrStat::reset()
{ 
    sum = sum2 = n = avg = dev = 0.0;
    min = numeric_limits<float>::max();
    max = -numeric_limits<float>::max();
}

/// @cond Python

TOrange *TBasicAttrStat::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
The class computes and store minimal, maximal, average and standard
deviation of a variable. It does not include the median or any other
statistics that can be computed on the fly, without remembering the data;
such statistics can be obtained by classes from module
`Orange.statistics.distribution`.

Instances of this class are seldom constructed manually; they are more often
returned for entire domain at once.

Constructor expects a variable to which the statistics will apply. If
followed by the optional example table, the constructor will compute the
statistics of the given attribute.
*/
{
    PyObject *pyvar;
    PExampleTable egen;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O&:BasicAttrStat",
        BasicAttrStat_keywords, 
        &pyvar, &PExampleTable::argconverter_n, &egen)) {
        return NULL;
    }
    if (!egen) {
	    if (!OrContinuousVariable_Check(pyvar)) {
            raiseError(PyExc_TypeError,
                "BasicAttrStat expects a ContinuousVariable, not a '%s'",
                pyvar->ob_type->tp_name);
	    }
	    return new (type)TBasicAttrStat(PContinuousVariable(pyvar));
    }
    PVariable var = TDomain::varFromArg_byDomain(pyvar, egen->domain, false);
    if (!var) {
      return NULL;
    }
    PContinuousVariable cvar = PContinuousVariable::cast(var);
    if (!cvar) {
        raiseError(PyExc_TypeError,
            "BasicAttrStat expects a continuous variable");
    }
    return new(type) TBasicAttrStat(egen, cvar);
}


PyObject *TBasicAttrStat::py_add(PyObject *args, PyObject *kw)
/*
The value should be given as a float. The optional second argument can
specify the weight which is used in computation of average and deviation. */
{
    double value, weight = 1.0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "d|d:add", BasicAttrStat_add_keywords,
        &value, &weight)) {
        return NULL;
    }
    add(value, weight);
    Py_RETURN_NONE;
}


PyObject *TBasicAttrStat::py_reset()
{ 
    reset();
    Py_RETURN_NONE;
}

/// @endcond