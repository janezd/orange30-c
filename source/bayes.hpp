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

#ifndef __BAYES_HPP
#define __BAYES_HPP

#include "bayes.ppp"
#include "probabilityestimator.hpp"

class TBayesLearner : public TLearner {
public:
    __REGISTER_CLASS(BayesLearner);
    NEW_WITH_CALL(Learner);
    enum DistributionMethod {Minimal, Factor, Fixed, Uniform, Maximal} PYCLASSCONSTANTS;
    
    double loessWindowProportion; //P The proportion of points in a window for loess
    int loessPoints; //P The number of points on the curve estimating conditional probabilities
    int loessDistributionMethod; //P(&DistributionMethod) Meaning of the 'nPoints'

    PProbabilityEstimatorConstructor estimatorConstructor; //PN constructs a probability estimator for P(C)
    PProbabilityEstimatorConstructor conditionalEstimatorConstructor; //PN constructs a probability estimator for P(C|A)
    bool adjustThreshold; //P adjust probability thresholds (for binary classes only)

    TBayesLearner();
    TBayesLearner(const TBayesLearner &);

    virtual PClassifier operator()(PExampleTable const &);
    PContingencyAttrClass loessSmoothing(PContingencyAttrClass const &);
};


class TBayesClassifier : public TClassifier {
public:
    __REGISTER_CLASS(BayesClassifier);

    PDiscDistribution distribution; //PN class distributions, P(C)
    PDomainContingency conditionalDistributions; //PN conditional distributions, P(C|A)
    double threshold; //P threshold probability for class 1 (for binary classes only)

    TBayesClassifier(PDomain const &);
    TBayesClassifier(PDomain const &, PDiscDistribution const &,
        PDomainContingency const &, double const thresh = 0.5);

    virtual TValue operator ()(TExample const *const );
    virtual PDistribution classDistribution(TExample const *const);
    virtual void predictionAndDistribution(TExample const *const,
        TValue &, PDistribution &);

    PICKLING_ARGS(domain);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(domain); construct a classifier");
};

#endif
