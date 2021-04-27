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


#ifndef __PROBABILITYESTIMATOR_HPP
#define __PROBABILITYESTIMATOR_HPP

#include "probabilityestimator.ppp"

class TDistribution;
class TExampleTable;

class TProbabilityEstimator : public TOrange {
public:
    __REGISTER_CLASS(ProbabilityEstimator);
    PDistribution probabilities; //PN estimated probabilities

    TProbabilityEstimator(PDistribution const &);
    virtual double operator()(TValue const val) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(distribution)");
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("([value]); return probability of value or a complete distribution");
};


class TProbabilityEstimatorConstructor : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(ProbabilityEstimatorConstructor);
    virtual PProbabilityEstimator operator()(PDistribution const &,
        PDistribution const &prior=PDistribution()) const = 0;
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(distribution, prior); construct a probability estimator");
};



class TProbabilityEstimatorConstructor_relative : public TProbabilityEstimatorConstructor {
public:
    __REGISTER_CLASS(ProbabilityEstimatorConstructor_relative);
    NEW_WITH_CALL(ProbabilityEstimatorConstructor);
    virtual PProbabilityEstimator operator()(PDistribution const &frequencies,
        PDistribution const &prior=PDistribution()) const;
};


class TProbabilityEstimatorConstructor_Laplace : public TProbabilityEstimatorConstructor {
public:
    __REGISTER_CLASS(ProbabilityEstimatorConstructor_Laplace);
    NEW_WITH_CALL(ProbabilityEstimatorConstructor)
    virtual PProbabilityEstimator operator()(PDistribution const &frequencies,
        PDistribution const &prior=PDistribution()) const;
};


class TProbabilityEstimatorConstructor_m : public TProbabilityEstimatorConstructor {
public:
    __REGISTER_CLASS(ProbabilityEstimatorConstructor_m);
    NEW_WITH_CALL(ProbabilityEstimatorConstructor)

    double m; //P parameter m for m-estimation

    TProbabilityEstimatorConstructor_m(double const=2.0);
    virtual PProbabilityEstimator operator()(PDistribution const &frequencies,
        PDistribution const &prior=PDistribution()) const;
};


class TProbabilityEstimatorConstructor_loess : public TProbabilityEstimatorConstructor {
public:
    __REGISTER_CLASS(ProbabilityEstimatorConstructor_loess);
    NEW_WITH_CALL(ProbabilityEstimatorConstructor)
    enum DistributionMethod {Minimal, Factor, Fixed, Uniform, Maximal} PYCLASSCONSTANTS;

    double windowProportion; //P The proportion of points in a window for LR
    int nPoints; //P The number of points on curve (negative means the given number of points is inserted in each interval)
    int distributionMethod; //P(&DistributionMethod) Meaning of the 'nPoints'

    TProbabilityEstimatorConstructor_loess(double const windowProp=0.5, int const nP=-1);
    virtual PProbabilityEstimator operator()(PDistribution const &frequencies,
        PDistribution const &prior=PDistribution()) const;
};

#endif
