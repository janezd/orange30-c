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

#ifndef _CONSTANTCLASSIFIER_HPP
#define _CONSTANTCLASSIFIER_HPP

#include "constantclassifier.ppp"

class TConstantClassifier : public TClassifier {
public:
    __REGISTER_CLASS(ConstantClassifier);

    TValue defaultVal; // get setter is defined below!!!
    PDistribution defaultDistribution; //PN distribution returned by the classifier

    TConstantClassifier(PVariable const &);
    TConstantClassifier(PVariable const &, PDistribution const &);
    TConstantClassifier(PVariable const &, TValue const defVal,
                        PDistribution const &defDis=PDistribution());
    TConstantClassifier(TConstantClassifier const &old);

    virtual TValue operator ()(TExample const *const);
    virtual PDistribution classDistribution(TExample const *const);
    virtual void predictionAndDistribution(TExample const *const, TValue &, PDistribution &);

    PICKLING_ARGS(class_var default_val default_distribution);
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, value, distribution); classifier that always gives the same prediction");
    static PyObject *__get__defaultVal(PyObject *self) PYDOC("value returned by the classifier");
    static int __set__defaultVal(PyObject *self, PyObject *pyvalue);
};

#endif
