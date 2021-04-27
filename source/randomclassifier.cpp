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
#include "basicattrstat.hpp"
#include "gaussiandistribution.hpp"
#include "randomclassifier.px"


TRandomLearner::TRandomLearner()
{}

TRandomLearner::TRandomLearner(PDistribution const &dist)
: probabilities(dist)
{}


PClassifier TRandomLearner::operator()(PExampleTable const &gen)
{
    PVariable classVar = gen->domain->classVar;
    if (!classVar) {
        raiseError(PyExc_ValueError, "data has no class");
    }
    if (probabilities) {
        if (classVar->varType == TVariable::Discrete) {
            if (!dynamic_cast<TDiscDistribution *>(probabilities.borrowPtr())) {
                raiseError(PyExc_ValueError,
                    "discrete class '%s' requires discrete distribution",
                    classVar->cname());
            }
        }
        else {
            if (!dynamic_cast<TContDistribution *>(probabilities.borrowPtr())) {
                raiseError(PyExc_ValueError,
                    "continuous class '%s' requires continuous distribution",
                    classVar->cname());
            }
        }
        PDistribution probc(CLONE(PDistribution, probabilities));
        probc->normalize();
        return PClassifier(new TRandomClassifier(classVar, probc));
    }

    if (classVar->varType == TVariable::Discrete) {
        PDistribution dist(getClassDistribution(gen));
        return PClassifier(new TRandomClassifier(classVar, dist));
    }
    else {
        PBasicAttrStat stat(new TBasicAttrStat(gen, PContinuousVariable(classVar)));
        PGaussianDistribution ga(new TGaussianDistribution(stat->avg, stat->dev));
        ga->variable = classVar;
        return PClassifier(new TRandomClassifier(classVar, ga));
    }
}


TRandomClassifier::TRandomClassifier(PVariable const &acv) 
: TClassifier(acv),
probabilities(TDistribution::create(acv))
{}


TRandomClassifier::TRandomClassifier(PVariable const &acv,
                                     PDistribution const &defDis)
: TClassifier(acv),
probabilities(defDis ? defDis : TDistribution::create(acv))
{}


TRandomClassifier::TRandomClassifier(const TRandomClassifier &old)
: TClassifier(dynamic_cast<const TClassifier &>(old)),
probabilities(CLONE(PDistribution, old.probabilities))
{}


TValue TRandomClassifier::operator ()(TExample const *const exam)
{ 
    return probabilities->randomValue(exam->checkSum());
}


PDistribution TRandomClassifier::classDistribution(TExample const *const)
{ 
    return CLONE(PDistribution, probabilities);
}


void TRandomClassifier::predictionAndDistribution(TExample const *const exam,
                                                  TValue &val,
                                                  PDistribution &dist)
{ 
    val = operator()(exam);
    dist = CLONE(PDistribution, probabilities);
}


TOrange *TRandomClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    PDistribution dist;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "|O&O&:RandomClassifier", RandomClassifier_keywords,
        &PVariable::argconverter_n, &var,
        &PDistribution::argconverter_n, &dist)) {
            return NULL;
    }
    if (dist) {
        if (var) {
            if (dist->variable && (dist->variable != var)) {
            raiseError(PyExc_ValueError,
                "probabilities correspond to variable '%s', not '%s'",
                dist->variable->cname(), var->cname());
            }
        }
        else {
            var = dist->variable;
        }
    }
    if (!var) {
        raiseError(PyExc_ValueError, "cannot deduce variable from arguments");
    }
    return new (type)TRandomClassifier(var, dist);
}

