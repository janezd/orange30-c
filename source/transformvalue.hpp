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


#ifndef __TRANSFORMVALUE_HPP
#define __TRANSFORMVALUE_HPP

#include "transformvalue.ppp"

/*  Transforms the value to another value. Transforming can be done 'in place' by replacing the old
    value with a new one (function 'transform'). Alternatively, operator () can be used to get
    the transformed value without replacing the original. Transformations can be chained. */
class TTransformValue : public TOrange {
public:
  __REGISTER_CLASS(TransformValue)

  PTransformValue subTransform; //P transformation executed prior to this

  TTransformValue();
  TTransformValue(PTransformValue const &sub);
  TTransformValue(const TTransformValue &old);

  TValue operator()(TValue const val) const;
  virtual void transform(TValue &val) const;

  NEW_NOARGS;
  PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(value); transform a value");
};



#endif

