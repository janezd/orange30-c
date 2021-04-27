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

#ifndef __CLASSIFIERFROMVAR_HPP
#define __CLASSIFIERFROMVAR_HPP

#include "classifierfromvar.ppp"

/*! A helper classifier that returns the value of the specified attribute.
    The value can be transformed by the given transformer.

    This classifier is used for data transformations like discretization.

    The classifier stores the version id of the last example's domain, the
    last value of #variable and its position (index) in that domain. At the next
    call, if the domain and the variable match, the last index is used instead
    of looking for the variable again.
*/
class TClassifierFromVar : public TClassifier {
public:
    __REGISTER_CLASS(ClassifierFromVar);

    /// The variable whose value is returned
    PVariable variable; //P variable whose value is used as class

    /// Transformation applied to the value
    PTransformValue transformer; //P transformer applied to the value

    /// Indicates whether to also transform unknown values
    bool transformUnknowns; //P if false (default is true), unknowns do not get transformed

    TClassifierFromVar(PVariable const &classVar =PVariable(),
                       PVariable const &whichVar =PVariable(),
                       PTransformValue const & =PTransformValue());
    TClassifierFromVar(const TClassifierFromVar &);
    virtual TValue operator ()(TExample const *const);

    /// @cond Python
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("([variable, transformer]); construct the classifier");
    /// @endcond

private:
    int lastDomainVersion; ///< The domain version of the last example
    PVariable lastVariable; ///< The value of #variable at the last call
    /*! The position of #variable in the domain at the last call. A value
        of \c ILLEGAL_INT indicates that the variable is not present in the
        domain, so #TVariable::getValueFrom will be used to retrieve the value. */
    int position; 
};

#endif
