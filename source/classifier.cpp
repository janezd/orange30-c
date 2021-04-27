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
#include "classifier.px"


/*! Construct the classifier, set the #classVar; #domain is set to \c NULL.
    \param var Class variable
*/
TClassifier::TClassifier(PVariable const &var)
: classVar(var)
{ 
    if (var && !var->isPrimitive()) {
        raiseError(PyExc_ValueError,
            "classifiers can predict only primitive values");
    }
}


/*! Construct the classifier, set the #domain. If #domain is not \c
    NULL, #classVar is set to the #domain's class, which must be
    present and primitive.
    \param dom Domain
*/ 
TClassifier::TClassifier(PDomain const &dom)
: classVar(dom ? dom->classVar : PVariable()),
  domain(dom)
{
    if (dom && !dom->classVar) {
        raiseError(PyExc_ValueError,
            "classifier cannot use domain without a class");
    }
    if (classVar && !classVar->isPrimitive()) {
        raiseError(PyExc_ValueError,
            "classifiers can predict only primitive values");
    }
}


/*! Return the class prediction or a continuous value for the
    example. The default implementation calls #classDistribution,
    which in turn calls this operator, so derived classes must
    overload one of these two to avoid infinite recursion.

    \param example Example to be classified or regressed
 */
TValue TClassifier::operator ()(TExample const *const example)
{
    return classDistribution(example)->predict(example);
}


/*! Return the predicted class distribution or continuous distribution
    for the given example. The default implementation calls the call
    operator, which in turn calls this function, so derived classes
    must overload one of these two to avoid infinite recursion.

    Most regressors do not return any meaningful distributions so they
    should in general overload the call operator.

    \param example Example to be classified or regressed
 */
PDistribution TClassifier::classDistribution(TExample const *const example)
{
    TValue cls = operator()(example);
    PDistribution dist = TDistribution::create(classVar);
    dist->add(cls);
    return dist;
}


/*! Return the predicted class or continuous value and its
    distribution for the given example. The default implementation
    calls #classDistribution and then uses the most probable value as
    the class prediction. Derived classes should overload the method
    if the predicted class is not necessarily the value with the
    highest probability or if the method can be implemented more
    efficiently.

    \param example Example to be classified or regressed
    \param val Predicted value
    \param dist Predicted distribution
*/
void TClassifier::predictionAndDistribution(TExample const *const example,
                                            TValue &val, PDistribution &dist)
{
    dist = classDistribution(example);
    val = classVar->varType == TVariable::Discrete
        ? dist->highestProbValue(example) : dist->average();
}


/// @cond Python
PyObject *TClassifier::__call__(PyObject *args, PyObject *kw)
/*
Return a class prediction, predicted distribution or both. The first
argument, an example, can be given either as an instance of `Example` or
as a list of values. The optional second argument specifies the return
type; should be one of `Classifier.GetValue`, `Classifier.GetProbabilities`
or `Classifier.GetBoth`.
*/
{
    PyObject *pyexample;
    int what = TClassifier::GetValue;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|i:Classifier",
        Classifier_call_keywords, &pyexample, &what)) {
            return NULL;
    }
    PExample ex = TExample::fromDomainAndPyObject(domain, pyexample, false);
    TExample *pex = ex.borrowPtr();
    switch (what) {
        case TClassifier::GetValue: {
            TValue const val = (*this)(pex);
            return PyObject_FromNewOrange(new TPyValue(classVar, val));
        }
        case TClassifier::GetProbabilities: {
            return classDistribution(pex).getPyObject();
        }
        case TClassifier::GetBoth: {
            TValue val;
            PDistribution dist;
            predictionAndDistribution(pex, val, dist);
            if (!dist) {
                return NULL;
            }
            return Py_BuildValue("NN",
                PyObject_FromNewOrange(new TPyValue(classVar, val)),
                dist.getPyObject());
        }
        default:
            raiseError(PyExc_ValueError, "invalid value of parameter 'what'");
    }
    return NULL;
}

/// @endcond