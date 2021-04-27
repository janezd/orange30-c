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

#ifndef _CLASSIFIER_HPP
#define _CLASSIFIER_HPP

#include "classifier.ppp"

class TDistribution;

/*! Classifier is a class whose instances can predict a class
    attribute or a continuos value for the given example. More
    generally, they compute a (primitive) value from an example, so
    they are also used in other contexts, such as discretization, that
    is, returning a discrete value for the give continuous value of a
    feature.

    The name TClassifier is a bit of misnomer since instances of this
    class can also be regressors.

    Most classifiers can also return a distribution instead of a
    single value; those that don't, give a distribution with a single
    value having 1.0 probability.

    Classifiers can store a domain descriptor. In this case, an
    example is converted to that domain prior to classification.

    Although the class has no pure virtual methods, it is abstract in
    the sense that either the call operator or #classDistribution must
    be overload in derived classes.
*/
class TClassifier : public TOrange {
public:
    __REGISTER_CLASS(Classifier);

    /*! The feature descriptor for the returned value. In the context
      of ordinary classification, this is the class attribute. */
    PVariable classVar; //PN class variable

    /*! Domain to which the example is converted before
        classification; ignored if \c NULL */
    PDomain domain; //PN domain (ignored if NULL)

    TClassifier(PVariable const &);
    TClassifier(PDomain const &);

    virtual TValue operator ()(TExample const *const);
    virtual PDistribution classDistribution(TExample const *const);
    virtual void predictionAndDistribution(TExample const *const,
                                           TValue &, PDistribution &);

    inline TValue operator ()(PExample const &);
    inline PDistribution classDistribution(PExample const &);
    inline void predictionAndDistribution(PExample const &,
                                         TValue &, PDistribution &);

    /// @cond Python
    enum ReturnType {GetValue, GetProbabilities, GetBoth} PYCLASSCONSTANTS_UP;

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(example, what) -> value or distribution");
    /// @endcond
};

typedef TOrangeVector<PClassifier, TWrappedReferenceHandler<PClassifier>,
    &OrClassifierList_Type> TClassifierList;

PYVECTOR(ClassifierList, Classifier)

#define TClassifierFD TClassifier

/*! Convenience function; calls (*this)(TExample const *const). */
TValue TClassifier::operator ()(PExample const &ex)
{ 
    return (*this)(ex.borrowPtr());
}

/*! Convenience function; calls classDistribution)(TExample const *const). */
PDistribution TClassifier::classDistribution(PExample const &ex)
{
    return classDistribution(ex.borrowPtr());
}

/*! Convenience function; calls
    predictionAndDistribution)(TExample const *const,
                               TValue &, PDistribution &). */
void TClassifier::predictionAndDistribution(PExample const &ex,
                                            TValue &val, PDistribution &dist)
{
    predictionAndDistribution(ex.borrowPtr(), val, dist);
}

#endif
