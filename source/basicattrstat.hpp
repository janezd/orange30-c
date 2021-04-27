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


#ifndef __BASICATTRSTAT_HPP
#define __BASICATTRSTAT_HPP

#include "basicattrstat.ppp"

/*! A class that contains basic statistics for a continuous variable: minimal
    and maximal value, average, deviation etc. */
class TBasicAttrStat : public TOrange {
public:
    __REGISTER_CLASS(BasicAttrStat);

    /// Weighted sum of encountered values
    TValue sum; //P sum of values

    /// Weighted sum of squared values
    TValue sum2; //P sum of squares of values

    /// The weighted number of values (ie, the sum of weights)
    double n; //P number of examples for which the attribute is defined

    /// The lowest value of the attribute
    TValue min; //P the lowest value of the attribute

    /// The highest value of the attribute
    TValue max; //P the highest value of the attribute

    /// The average value of the attribute
    TValue avg; //P the average value of the attribute

    /// The deviation of attribute's values
    TValue dev; //P the deviation of the value of the attribute

    /// The variable to which the data applies
    PContinuousVariable variable; //PN the variable to which the data applies

    /// Holds recomputation of #avg and #dev while adding data
    bool holdRecomputation;

    TBasicAttrStat(PContinuousVariable const &var, bool const ahold=false);
    TBasicAttrStat(PContDistribution const &dist);
    TBasicAttrStat(PExampleTable const &gen, PContinuousVariable const &var);
    
    void add(TValue const f, double const p=1);
    void recompute();

    void reset();

    /// @cond Python
    PICKLING_ARGS(variable);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable[, data])");
    PyObject *py_add(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(value[, weight]); add the value to the statistics");
    PyObject *py_reset() PYARGS(METH_NOARGS, "(); reset the statistics");
    /// @endcond
};

#endif

