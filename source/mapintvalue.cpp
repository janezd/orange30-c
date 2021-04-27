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
#include "mapintvalue.px"


TMapIntValue::TMapIntValue()
{}


TMapIntValue::TMapIntValue(PIntList const &al)
: mapping(al)
{}


TMapIntValue::TMapIntValue(TIntList const &al)
: mapping(new TIntList(al))
{}


void TMapIntValue::transform(TValue &val) const
{ 
    checkProperty(mapping);

    if (isnan(val)) {
        return;
    }
    int const ival = TDiscreteVariable::toInt(val,
        "MapIntValue.transform expects discrete value");
    if (ival >= mapping->size()) {
        raiseError(PyExc_ValueError, "value %i out of range", ival);
    }
    val = mapping->at(ival);
    if (val < 0) {
        val = UNDEFINED_VALUE;
    }
}
