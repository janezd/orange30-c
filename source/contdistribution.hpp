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

#ifndef __CONTDISTRIBUTION_HPP
#define __CONTDISTRIBUTION_HPP

#include "contdistribution.ppp"

class TContDistribution : public TDistribution {
public:
    MAP_INTERFACE_WOUT_OP(double, double, distribution, typedef)

    __REGISTER_CLASS(ContDistribution);
    double sum; //PR weighted sum of elements (i.e. N*average)
    double sum2; //PR weighted sum of squares of elements

    TContDistribution();
    TContDistribution(PVariable const &);
    TContDistribution(map<double, double> const &f);
    TContDistribution(TContDistribution const &);
    TContDistribution &operator =(TContDistribution const &);

    virtual double const &at(TValue const v); // returns const to prevent modifying without changing abs, cases, normalized...
    virtual double const &at(TValue const v) const;
    inline virtual double const &operator[](TValue const val) { return at(val); }
    inline virtual double const &operator[](TValue const val) const { return at(val); }
    virtual void  add(TValue const v, double const w = 1.0);
    virtual void  set(TValue const v, double const w);

    virtual void normalize();
    virtual unsigned int checkSum() const;

    virtual double p(double const) const;
    virtual double highestProb() const;
    virtual TValue highestProbValue(long const random = 0) const;
    virtual TValue highestProbValue(TExample const *const random) const;
    virtual TValue randomValue(long const random=0) const;

    bool operator ==(TDistribution const &) const;
    TDistribution &operator +=(TDistribution const &);
    TDistribution &operator -=(TDistribution const &);
    TDistribution &operator *=(double const weight);

    virtual double average() const;
    virtual double dev() const;
    virtual double var() const;
    virtual double error() const;
    virtual double percentile(double const) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable[, examples]); construct a distribution from data");
    static TOrange *unpickle(PyTypeObject *cls, PyObject *args);
    PyObject *__richcmp__(PyObject *other, int op) const;
    long __hash__() const;
    PyObject *__str__() const;
    PyObject *__repr__() const;

    PyObject *keys() PYARGS(METH_NOARGS, "") const;
    PyObject *values() PYARGS(METH_NOARGS, "") const;
    PyObject *items() PYARGS(METH_NOARGS, "") const;
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepares arguments for unpickling");

    PyObject *native() const PYARGS(METH_NOARGS, "() -> list; tuples representing the frequencies");
    PyObject *py_average() const PYARGS(METH_NOARGS, "distribution average");
    PyObject *py_var() const PYARGS(METH_NOARGS, "distribution variance");
    PyObject *py_dev() const PYARGS(METH_NOARGS, "distribution deviation");
    PyObject *py_error() const PYARGS(METH_NOARGS, "distribution error");
    PyObject *py_percentile(PyObject *arg) const PYARGS(METH_O, "(p) -> float; the value at p-th percentile (0 < p < 100)");
    PyObject *py_density(PyObject *arg) const PYARGS(METH_O, "(p) -> float; the probability density at value p");
};

#endif
