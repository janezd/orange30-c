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
#include "transformvalue.px"


TTransformValue::TTransformValue()
{}


TTransformValue::TTransformValue(PTransformValue const &sub)
: subTransform(sub)
{}


TTransformValue::TTransformValue(TTransformValue const &other)
: TOrange(other),
  subTransform(CLONE(PTransformValue, other.subTransform))
{}


TValue TTransformValue::operator()(TValue const val) const
{
  TValue newval = val;
  transform(newval);
  return newval;
}


void TTransformValue::transform(TValue &val) const
{ 
    if (subTransform) {
        subTransform->transform(val); 
    }
}


PyObject *TTransformValue::__call__(PyObject *args, PyObject *kw) const
{
    PPyValue pyval;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:TransformValue",
        TransformValue_call_keywords, &PPyValue::argconverter, &pyval)) {
            return NULL;
    }
    TValue val = pyval->value;
    transform(val);
    return PyObject_FromNewOrange(new TPyValue(pyval->variable, val));
}