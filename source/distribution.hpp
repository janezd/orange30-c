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

#ifndef __DISTRIBUTION_HPP
#define __DISTRIBUTION_HPP

#include "distribution.ppp"

class TVariable;
class TRandomGenerator;
class TExample;
class TExampleTable;

class TDistribution : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Distribution);

    PVariable variable; //P attribute descriptor (optional!)
    mutable PRandomGenerator randomGenerator; //P generator used for choosing random values
    double unknowns; //P number of unknown values
    double abs; //P sum of frequencies (not including unknown values!)
    double cases; //P number of cases; as abs, but doesn't change at *= and normalize()
    bool normalized; //P distribution is normalized

    bool supportsContinuous; //PR distribution supports continuous interface

    TDistribution();
    TDistribution(PVariable const &);
    TDistribution(TDistribution const &);
    TDistribution &operator =(TDistribution const &);

    // Create either TDiscDistribution or TContDistribution
    static PDistribution create(PVariable const &);
    static PDistribution fromExamples(PExampleTable const &, TAttrIdx const);
    static PDistribution fromExamples(PExampleTable const &, PVariable const &);
    virtual size_t size() const =0;
    virtual void  normalize() =0;
    virtual unsigned int checkSum() const =0;

    virtual double const &at(TValue const) =0;
    virtual double const &at(TValue const) const =0;
    virtual double const &operator[](TValue const) =0;
    virtual double const &operator[](TValue const) const =0;
    virtual void set(TValue const, double const) =0;
    virtual void add(TValue const, double const = 1.0) =0;
    
    virtual double p(TValue const) const =0;
    virtual double highestProb() const =0;
    virtual TValue highestProbValue(long const random=0) const =0;
    virtual TValue highestProbValue(TExample const *const random) const =0;
    virtual TValue randomValue(long const random=0) const =0;
    inline TValue predict(TExample const *const random) const;

    /* Derived classes must define those if they make sense */
    virtual bool operator ==(TDistribution const &) const =0;
    virtual TDistribution &operator += (TDistribution const &other);
    virtual TDistribution &operator -= (TDistribution const &other);
    virtual TDistribution &operator *= (double const);

    /* Those that have supportContinuous == true must redefine those */
    virtual double average() const;
    virtual double dev() const;
    virtual double var() const;
    virtual double percentile(double const &) const;
    virtual double error() const;

    static TDistribution *setterConversion(PyObject *);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable[, data]); construct a distribution from data");
    Py_ssize_t __len__() const;
    PyObject *__subscript__(PyObject *index);
    int __ass_subscript__(PyObject *index, PyObject *value);

    PyObject *py_add(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(value[, weight]); add the value to the sample");
    PyObject *py_normalize() PYARGS(METH_NOARGS, "normalize the distribution to sum up to 1");
    PyObject *modus() const PYARGS(METH_NOARGS, "return distribution modus/the most frequent value");
    PyObject *random() const PYARGS(METH_NOARGS, "return random value according to the distribution");

private:
    TValue valueFromPyObject(PyObject *index) const;
};

typedef TOrangeVector<PDistribution, TWrappedReferenceHandler<PDistribution>,
    &OrDistributionList_Type> TDistributionList;

PYVECTOR(DistributionList, Distribution)

PDistribution getClassDistribution(PExampleTable const &);
PYMODULEFUNCTION PyObject *py_getClassDistribution(PyObject *, PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(data) -> Distribution; compute the distribution of the class variable");

TValue TDistribution::predict(TExample const *random) const
{
    return supportsContinuous ? average() : highestProbValue(random);
}

#endif
