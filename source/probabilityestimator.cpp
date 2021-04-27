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
#include "simplerandomgenerator.hpp"
#include "probabilityestimator.px"



TProbabilityEstimator::TProbabilityEstimator(PDistribution const &af)
: probabilities(af)
{}


double TProbabilityEstimator::operator()(TValue const classVal) const
{ 
    if (isnan(classVal)) {
        raiseError(PyExc_ValueError,
            "cannot estimate probability of an undefined value");
    }
    return probabilities->p(classVal);
}


PProbabilityEstimator TProbabilityEstimatorConstructor_relative::operator()
    (PDistribution const &frequencies, PDistribution const &prior) const
{ 
    PDistribution probs(CLONE(PDistribution, frequencies));
    probs->normalize();
    return PProbabilityEstimator(new TProbabilityEstimator(probs));
}


PProbabilityEstimator TProbabilityEstimatorConstructor_Laplace::operator()
    (PDistribution const &frequencies, PDistribution const &prior) const
{ 
    PDistribution probs(CLONE(PDistribution, frequencies));
    PDiscDistribution ddist(probs);
    if (ddist && ddist->size()) {
        double const &abs = ddist->abs;
        double const &cases = ddist->cases;
        double const div = cases + ddist->size();
        if ((cases == abs) || (abs < 1e-6)) {
            PITERATE(TDiscDistribution, di, ddist) {
                *di = (*di + 1) / div;
            }
        }
        else {
            PITERATE(TDiscDistribution, di, ddist) {
                *di = (*di/abs*cases + 1) / div;
            }
        }
        ddist->abs = 1;
    }
    else {
        probs->normalize();
    }
    return PProbabilityEstimator(new TProbabilityEstimator(probs));
}


TProbabilityEstimatorConstructor_m::TProbabilityEstimatorConstructor_m(double const am)
: m(am)
{}


PProbabilityEstimator TProbabilityEstimatorConstructor_m::operator()
    (PDistribution const &frequencies, PDistribution const &prior) const
{
    PDistribution probs(CLONE(PDistribution, frequencies));
    PDiscDistribution ddist(probs);
    if (ddist && ddist->size()) {
        if (!prior) {
            raiseError(PyExc_ValueError, "prior distribution is not given");
        }
        PDiscDistribution dprior(prior);
        if (!dprior) {
            raiseError(PyExc_ValueError, "prior distribution must be discrete");
        }
        if (dprior->size() != ddist->size()) {
            raiseError(PyExc_ValueError, 
                "prior distribution must have the same size as the relative frequency distribution");
        }
        const double mabs = m / dprior->abs;
        const double &abs = ddist->abs;
        const double &cases = ddist->cases;
        const double div = cases + m;
        if ((cases == abs) || (abs < 1e-6)) {
            TDiscDistribution::iterator di(ddist->begin()), de(ddist->end());
            TDiscDistribution::iterator pi(dprior->begin());
            for(; di!=de; di++, pi++) {
                *di = (*di + *pi*mabs) / div;
            }
        }
        else {
            TDiscDistribution::iterator di(ddist->begin()), de(ddist->end());
            TDiscDistribution::iterator pi(dprior->begin());
            for(; di!=de; di++, pi++) {
                *di = (*di/abs*cases + *pi*mabs) / div;
            }
        }
        ddist->abs = 1;
    }
    else {
        probs->normalize();
    }
    return PProbabilityEstimator(new TProbabilityEstimator(probs));
}



TProbabilityEstimatorConstructor_loess::TProbabilityEstimatorConstructor_loess
    (double const windowProp, const int ak)
: windowProportion(windowProp),
  nPoints(ak),
  distributionMethod(DISTRIBUTE_MAXIMAL)
{}



PProbabilityEstimator TProbabilityEstimatorConstructor_loess::operator()
    (PDistribution const &frequencies, PDistribution const &prior) const
{ 
    PContDistribution cdist(frequencies);
    if (!cdist) {
        raiseError(PyExc_TypeError, "LOESS needs continuous distribution");
    }
    if (!cdist->size()) {
        raiseError(PyExc_ValueError, "distribution is empty");
    }
    map<double, double> loesscurve;
    loess(cdist->distribution, nPoints, windowProportion,
        loesscurve, distributionMethod);
    PDistribution probs(new TContDistribution(loesscurve));
    return PProbabilityEstimator(new TProbabilityEstimator(probs));
}


PyObject *TProbabilityEstimator::__call__(PyObject *args, PyObject *kw) const
{
    PyObject *pyvalue = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "|O:ProbabilityEstimator", ProbabilityEstimator_call_keywords,
        &pyvalue)) {
            return NULL;
    }
    if (!pyvalue) {
        return CLONE(PDistribution, probabilities).getPyObject();
    }
    TValue val;
    if (probabilities->variable) {
        val = probabilities->variable->py2val(pyvalue);
    }
    else if (PyNumber_Check(pyvalue)) {
        val = PyNumber_AsDouble(pyvalue);
    }
    else if (OrPyValue_Check(pyvalue)) {
        if (((TPyValue *)pyvalue)->isPrimitive()) {
            raiseError(PyExc_ValueError,
                "cannot compute probabilities of non-primitive variables");
        }
        val = ((TPyValue *)pyvalue)->value;
    }
    else {
        raiseError(PyExc_ValueError,
            "ProbabilityEstimator expects a value, not '%s'",
            pyvalue->ob_type->tp_name);
    }
    return PyFloat_FromDouble(operator ()(val));
}


TOrange *TProbabilityEstimator::__new__(PyTypeObject *type,
                                        PyObject *args, PyObject *kw)
{
    PDistribution dist;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "O&:ProbabilityEstimator", ProbabilityEstimator_keywords,
        &PDistribution::argconverter, &dist)) {
            return NULL;
    }
    return new(type) TProbabilityEstimator(dist);
}


PyObject *TProbabilityEstimatorConstructor::__call__(
    PyObject *args, PyObject *kw) const
{
    PDistribution dist;
    PDistribution prior;
    if (!PyArg_ParseTupleAndKeywords(args, kw,"O&|O&:ProbabilityEstimatorConstructor",
        ProbabilityEstimatorConstructor_call_keywords,
        &PDistribution::argconverter, &dist,
        &PDistribution::argconverter_n, &prior)) {
            return NULL;
    }
    return (*this)(dist, prior).getPyObject();
}
