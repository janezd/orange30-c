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


#ifndef __KNN_HPP
#define __KNN_HPP

#include "knn.ppp"

class TFindNearest;
class TExamplesDistanceConstructor;

class TkNNLearner : public TLearner {
public:
    __REGISTER_CLASS(kNNLearner);
    NEW_WITH_CALL(Learner);

    double k; //P number of neighbours (0 for sqrt of #examples)
    bool rankWeight; //P enable weighting by ranks
    PExamplesDistanceConstructor distanceConstructor; //P metrics constructor

    TkNNLearner(double const ak=0,
        PExamplesDistanceConstructor const & =PExamplesDistanceConstructor());
    virtual PClassifier operator()(PExampleTable const &);
};


class TkNNClassifier : public TClassifier {
public:
    __REGISTER_CLASS(kNNClassifier);

    PFindNearest findNearest; //PN metrics
    double k; //P number of neighbours (0 for square root of number of examples)
    bool rankWeight; //P enable weighting by ranks
    double nExamples; //P the number of learning examples 

    TkNNClassifier(PDomain const &);
    TkNNClassifier(PDomain const &, double const ak,
        PFindNearest const &, bool const rankWeight=true, double const nEx=0);
    virtual PDistribution classDistribution(TExample const *const);

    PICKLING_ARGS(domain);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(domain); construct a classifier");
};

#endif
