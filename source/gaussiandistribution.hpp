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

#ifndef __GAUSSIANDISTRIBUTION_HPP
#define __GAUSSIANDISTRIBUTION_HPP

#include "gaussiandistribution.ppp"

class TGaussianDistribution : public TDistribution {
public:
    __REGISTER_CLASS(GaussianDistribution);

    double mean; //P mean
    double sigma; //P variance

    TGaussianDistribution(double const amean=0.0, double const asigma=1.0,
        double const anabs=1.0);
    TGaussianDistribution(PDistribution const &);
    TGaussianDistribution(TGaussianDistribution const &);
    TGaussianDistribution &operator =(TDistribution const &);

    virtual size_t size() const;
    virtual void  normalize();
    virtual unsigned int checkSum() const;

    virtual double const &at(TValue const);
    virtual double const &at(TValue const) const;
    virtual double const &operator[](TValue const);
    virtual double const &operator[](TValue const) const;
    virtual void add(TValue const, double const p = 1.0);
    virtual void set(TValue const, double const p);

    virtual double p(TValue const) const;
    virtual double highestProb() const;
    virtual TValue highestProbValue(long const random=0) const;
    virtual TValue highestProbValue(TExample const *const random) const;
    virtual TValue randomValue(long const random=0) const;

    bool operator ==(TDistribution const &) const;

    virtual double average() const;
    virtual double dev() const;
    virtual double var() const;
    virtual double error() const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(mean, sigma); construct a distribution from data");
    PyObject *__richcmp__(PyObject *other, int op) const;
    long __hash__() const;

    PyObject *py_average() const PYARGS(METH_NOARGS, "(); return distribution average");
    PyObject *py_var() const PYARGS(METH_NOARGS, "(); return distribution variance");
    PyObject *py_dev() const PYARGS(METH_NOARGS, "(); return distribution deviation");
    PyObject *py_error() const PYARGS(METH_NOARGS, "(); return distribution error");
    PyObject *py_modus() const PYARGS(METH_NOARGS, "(); return the modus");
    PyObject *py_density(PyObject *arg) const PYARGS(METH_VARARGS, "(x); return the probability density at value x");
};

#endif
