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

#ifndef __DISCDISTRIBUTION_HPP
#define __DISCDISTRIBUTION_HPP

#include "discdistribution.ppp"

#define DD_STORAGE_SIZE 6

/* For efficiency, DiscDistribution has a local storage for up to
   DD_STORAGE_SIZE numbers. If the size of distribution fits into this,
   pointer 'distribution' points to this, local storage. Otherwise it points to
   the heap and the local storage is unused.
   This way we avoid to use allocating a large number of small chunks (typically
   two doubles) on the heap and increase the locality of memory access. */

class TDiscDistribution : public TDistribution {
public:
    __REGISTER_CLASS(DiscDistribution);

    int nValues;
    double distribution_[DD_STORAGE_SIZE];
    double *distribution;

    PFloatList variances; //P a field used for arbitraty purposes

    TDiscDistribution();
    TDiscDistribution(const PVariable &);
    TDiscDistribution(ssize_t const values, double const value=0.0);
    TDiscDistribution(vector<double> const &f);
    TDiscDistribution(double const *, ssize_t);
    TDiscDistribution(TDiscDistribution const &);
    TDiscDistribution &operator =(TDiscDistribution const &);
    ~TDiscDistribution();

    inline virtual size_t size() const;
    void resize(size_t const newSize);
    typedef double *iterator;
    typedef double const *const_iterator;
    inline iterator begin();
    inline iterator end();
    inline const_iterator begin() const;
    inline const_iterator end() const;
    virtual double const &at(TValue const v); // returns const to prevent modifying without changing abs, cases, normalized...
    virtual double const &at(TValue const v) const;
    inline virtual double const &operator[](TValue const val) { return at(val); }
    inline virtual double const &operator[](TValue const val) const { return at(val); }
    virtual void add(TValue const, double const w = 1.0);
    virtual void set(TValue const, double const w);

    virtual void normalize();
    virtual unsigned int checkSum() const;

    virtual double p(TValue const) const;
    virtual double highestProb() const;
    virtual TValue highestProbValue(long const random=0) const;
    virtual TValue highestProbValue(TExample const *const random) const;
    virtual TValue randomValue(long const random=0) const;

    TDistribution &adddist(TDistribution const &, double const factor);

    bool operator ==(TDistribution const &other) const;
    TDistribution &operator +=(TDistribution const &);
    TDistribution &operator -=(TDistribution const &);
    TDistribution &operator *=(double const weight);
    TDistribution &operator *=(TDistribution const &);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable[, examples]); construct a distribution from data");
    static TOrange *unpickle(PyTypeObject *cls, PyObject *args);
    PyObject *__item__(Py_ssize_t index) const;
    PyObject *__richcmp__(PyObject *other, int op) const;
    long __hash__() const;
    PyObject *__str__() const;
    PyObject *__repr__() const;
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepares arguments for unpickling");

    PyObject *keys() const PYARGS(METH_NOARGS, "");
    PyObject *values() const PYARGS(METH_NOARGS, "");
    PyObject *items() const PYARGS(METH_NOARGS, "");
    PyObject *native() const PYARGS(METH_NOARGS, "return distribution represented with as a list of frequencies");

private:
    TDiscreteVariable *getDiscVar() const;
};

size_t TDiscDistribution::size() const
{
    return nValues;
}

TDiscDistribution::iterator TDiscDistribution::begin()
{
    return distribution;
}

TDiscDistribution::iterator TDiscDistribution::end()
{
    return distribution + nValues;
}

TDiscDistribution::const_iterator TDiscDistribution::begin() const
{
    return distribution;
}

TDiscDistribution::const_iterator TDiscDistribution::end() const
{
    return distribution + nValues;
}

#endif
