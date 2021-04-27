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
#include "examplesdistance.hpp"
#include "findnearest.hpp"
#include "knn.px"


TkNNLearner::TkNNLearner(double const ak, PExamplesDistanceConstructor const &edc)
: k(ak),
  rankWeight(true),
  distanceConstructor(edc)
{}


PClassifier TkNNLearner::operator()(PExampleTable const &gen)
{ 
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    PFindNearestConstructor fnc(new TFindNearestConstructor(distanceConstructor));
    PFindNearest findNearest = (*fnc)(gen);
    PyTypeObject *rtype = getReturnType(&OrkNNClassifier_Type);
    return PClassifier(new(rtype) TkNNClassifier(
        gen->domain, k, findNearest,
        rankWeight, gen->totalWeight()));
}


TkNNClassifier::TkNNClassifier(PDomain const &dom)
: TClassifier(dom),
  k(0),
  rankWeight(true),
  nExamples(0)
{}


TkNNClassifier::TkNNClassifier(PDomain const &dom,
                               double const ak,
                               PFindNearest const &fdist,
                               bool const rw,
                               double const nEx)
: TClassifier(dom),
  findNearest(fdist),
  k(ak),
  rankWeight(rw),
  nExamples(nEx)
{}


PDistribution TkNNClassifier::classDistribution(TExample const *const oexam)
{ 
    checkProperty(findNearest);
    PExample wexam = oexam->convertedTo(domain);
    TExample const *const exam = wexam.borrowPtr();
    const double tk = k ? k : sqrt(nExamples);
    PExampleTable neighbours = (*findNearest)(exam, tk, true);
    if (!neighbours->size()) {
        raiseError(PyExc_ValueError, "example has no neighbours (no data given?)");
    }
    PDistribution classDist = TDistribution::create(classVar);
    if (neighbours->size()==1) {
        classDist->add(neighbours->begin().getClass());
        return classDist;
    }
    if (rankWeight) {
        double const &sigma2 = tk*tk / -log(0.001);
        int rank2 = 1, rankp=1; // rank2 is rank^2, rankp = rank^2 - (rank-1)^2; and, voila, we don't need rank :)
        PEITERATE(ei, neighbours) {
            classDist->add(ei.getClass(), ei.getWeight() * exp(-(rank2 += (rankp+=2))/sigma2));
        }
    }
    else {
        int const &distanceID = findNearest->distanceID;
        double lastwei = neighbours->at(neighbours->size()-1, distanceID);
        if (lastwei < 1e-6) {
            PEITERATE(ei, neighbours) {
                classDist->add(ei.getClass());
            }
        }
        else {
            const double &sigma2 = sqr(lastwei) / -log(0.001);
            PEITERATE(ei, neighbours) {
                const double &wei = ei.getMeta(distanceID);
                classDist->add(ei.getClass(), ei.getWeight() * exp(-wei*wei/sigma2));
            }
        }
    }
    classDist->normalize();
    return classDist;
}

TOrange *TkNNClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PDomain domain;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "O&:kNNClassifier", kNNClassifier_keywords,
        &PDomain::argconverter, &domain)) {
        return NULL;
    }
    return new(type) TkNNClassifier(domain);
}
