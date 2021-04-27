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

/* Classifiers have three methods for classification.
   - operator() returns TValue
   - classDistribution return PDistribution
   - predictionAndDistribution returns both

   In TClassifier, the first calls the second and vice versa, so at least
   one of them needs to be overloaded. predictionAndDistribution calls
   classDistribution.

*/

#ifndef _RANDOMCLASSIFIER_HPP
#define _RANDOMCLASSIFIER_HPP

#include "randomclassifier.ppp"

class TRandomLearner : public TLearner {
public:
    __REGISTER_CLASS(RandomLearner);
    NEW_WITH_CALL(Learner);

    PDistribution probabilities; //P predicted probabilities (used instead of data, if given)

    TRandomLearner();
    TRandomLearner(PDistribution const &);
    virtual PClassifier operator()(PExampleTable const &);
};


class TRandomClassifier : public TClassifier {
public:
    __REGISTER_CLASS(RandomClassifier);

    PDistribution probabilities; //PN class probabilities

    TRandomClassifier(PVariable const &);
    TRandomClassifier(PVariable const &, PDistribution const &);
    TRandomClassifier(TRandomClassifier const &);

    virtual TValue operator ()(TExample const *const );
    virtual PDistribution classDistribution(TExample const *const);
    virtual void predictionAndDistribution(TExample const *const, TValue &val, PDistribution &);

    PICKLING_ARGS(classVar probabilities);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, probabilities); construct a classifier that gives random predictions");
};

#endif
