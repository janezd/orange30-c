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
#include "gaussiandistribution.px"


TGaussianDistribution::TGaussianDistribution(double const amean,
                                             double const asigma,
                                             double const anabs)
: mean(amean),
  sigma(asigma)
{
    abs = anabs;
    normalized = true;
    supportsContinuous = true;
}


TGaussianDistribution::TGaussianDistribution(PDistribution const &dist)
: TDistribution(*dist),
  mean(dist->average()),
  sigma(dist->dev())
{
    normalized = true; 
    supportsContinuous = true; 
}


TGaussianDistribution::TGaussianDistribution(TGaussianDistribution const &other)
: TDistribution(other),
mean(other.mean),
sigma(other.sigma)
{}
    

TGaussianDistribution &TGaussianDistribution::operator =(TDistribution const &other)
{
    TDistribution::operator=(other);
    mean = other.average();
    sigma = other.dev();
    return *this;
}


size_t TGaussianDistribution::size() const
{
    return 0;
}


void  TGaussianDistribution::normalize()
{}


unsigned int TGaussianDistribution::checkSum() const
{
    unsigned int crc;
    INIT_CRC(crc);
    add_CRC(mean, crc);
    add_CRC(sigma, crc);
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


#define NOT_IMPLEMENTED(x) { raiseError(PyExc_SystemError, "Gaussian distribution cannot implement '%s'", x); throw 0; /*just to avoid warnings*/ }

double const &TGaussianDistribution::at(TValue const i)
NOT_IMPLEMENTED("at")

double const &TGaussianDistribution::at(TValue const i) const
NOT_IMPLEMENTED("at")

double const &TGaussianDistribution::operator[](TValue const val)
NOT_IMPLEMENTED("operator []")

double const &TGaussianDistribution::operator[](TValue const val) const
NOT_IMPLEMENTED("operator []")

void TGaussianDistribution::add(TValue const i, double const p)
NOT_IMPLEMENTED("add")

void TGaussianDistribution::set(TValue const i, double const p)
NOT_IMPLEMENTED("set")


#define pi 3.1415926535897931

double TGaussianDistribution::p(TValue const x) const
{
    return abs * exp(-sqr((x-mean)/2/sigma)) / (sigma*sqrt(2*pi));
}

double TGaussianDistribution::highestProb() const
{
     return abs / (sigma * sqrt(2*pi)); 
}

TValue TGaussianDistribution::highestProbValue(long const random) const
{
    return mean;
}

TValue TGaussianDistribution::highestProbValue(TExample const *const random) const
{
    return mean;
}

TValue TGaussianDistribution::randomValue(long const random) const
{
    if (random != 0) {
        TRandomGenerator rg(random);
        return gasdev(mean, sigma, rg);
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    return gasdev(mean, sigma, *randomGenerator);
}


bool TGaussianDistribution::operator ==(TDistribution const &other) const
{
    TGaussianDistribution const *disc =
        dynamic_cast<TGaussianDistribution const *>(&other);
    if (!disc) {
        raiseError(PyExc_TypeError, "cannot compare distributions of different types");
    }
    return (mean == disc->mean) && (sigma == disc->sigma);
}


double TGaussianDistribution::average() const
{
    return mean;
}

double TGaussianDistribution::dev() const
{
    return sigma;
}

double TGaussianDistribution::var() const
{
    return sqr(sigma);
}

double TGaussianDistribution::error() const
{
    return sigma;
}


TOrange *TGaussianDistribution::__new__(PyTypeObject *type,
                                        PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_GET_SIZE(args) == 1)) {
        PyObject *arg = PyTuple_GET_ITEM(args, 0);
        if (OrDistribution_Check(arg)) {
            return new(type) TGaussianDistribution(PDistribution(arg));
        }
    }
    double mean = 0, sigma = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|dd:GaussianDistribution", 
        GaussianDistribution_keywords, &mean, &sigma)) {
            return NULL;
    }
    return new(type) TGaussianDistribution(mean, sigma);
}


PyObject *TGaussianDistribution::__richcmp__(PyObject *other, int op) const
{
    if ((op != Py_EQ) && (op != Py_NE)) {
        raiseError(PyExc_TypeError, "cannot compare two distributions");
    }
    if (!OrGaussianDistribution_Check(other)) {
        raiseError(PyExc_TypeError,
            "cannot compare Gaussian distribution with '%s'",
            other->ob_type->tp_name);
    }
    return PyBool_FromBool((*this == ((OrGaussianDistribution *)other)->orange)
        == (op == Py_EQ));
}

long TGaussianDistribution::__hash__() const
{
    return checkSum();
}

PyObject *TGaussianDistribution::py_average() const
{
    return PyFloat_FromDouble(average());
}

PyObject *TGaussianDistribution::py_modus() const
{
    return PyFloat_FromDouble(highestProbValue());
}


PyObject *TGaussianDistribution::py_var() const
{
    return PyFloat_FromDouble(var());
}

PyObject *TGaussianDistribution::py_dev() const
{
    return PyFloat_FromDouble(dev());
}

PyObject *TGaussianDistribution::py_error() const
{
    return PyFloat_FromDouble(error());
}

PyObject *TGaussianDistribution::py_density(PyObject *arg) const
{
    const double val = PyNumber_AsDouble(arg,
        "density(p) expects a number, not '%s'");
    return PyFloat_FromDouble(p(val));
}
