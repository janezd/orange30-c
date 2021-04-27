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
#include "classifierfromvar.px"

/*! Constructor; stores the class variable, the variable and the transformer
    and sets transformUnknowns to \c true and \c lastDomainVersion to -1. */
TClassifierFromVar::TClassifierFromVar(PVariable const &aClassVar,
                                       PVariable const &var, 
                                       PTransformValue const &trans)
: TClassifier(aClassVar),
  variable(var),
  transformer(trans),
  transformUnknowns(true),
  lastDomainVersion(-1)
{}


/*! Copy constructor.
*/
TClassifierFromVar::TClassifierFromVar(const TClassifierFromVar &old)
: TClassifier(old),
  variable(old.variable),
  transformer(old.transformer),
  transformUnknowns(old.transformUnknowns),
  lastDomainVersion(-1)
{}


/*! Returns the value of #variable for the given example; transformation
    is applied if given. If the domain version is not the same as in the
    last call or the #variable has been changed, the variable is searched
    for in the domain and its index is stored in #position. */
TValue TClassifierFromVar::operator ()(TExample const *const example)
{ 
    if ((lastDomainVersion != example->domain->version) ||
        (lastVariable != variable)) {
            checkProperty(variable);
            lastVariable = variable;
            lastDomainVersion = example->domain->version;
            position = example->domain->getVarNum(variable, false);
    }
    TValue val = position == ILLEGAL_INT ?
        variable->computeValue(example) : example->getValue(position);
    if (transformer && (!isnan(val) || transformUnknowns)) {
        return (*transformer)(val);
    }
    else {
        return val;
    }
}

/// @cond Python

TOrange *TClassifierFromVar::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    PTransformValue transformer;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O&O&:ClassifierFromVar",
        ClassifierFromVar_keywords,
        &PVariable::argconverter_n, &var,
        &PTransformValue::argconverter_n, &transformer)) {
            return NULL;
    }
    return new(type) TClassifierFromVar(var, var, transformer);
}

/// @endcond