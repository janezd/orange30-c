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


#ifndef __VALUES_HPP
#define __VALUES_HPP


#ifdef _MSC_VER
#define isnan _isnan
#endif

/*! \file 

    Definitions of global types, constants and functions related to primitive
    values.
*/

/*! Type \c TValue in Orange 3.0 is a typedef'd to \c double. It represents
    a value of an ordinary (primitive) attribute. For discrete attributes,
    it contains round numbers that are interpreted as integers. Undefined
    (missing, unknown) values are represented by \c NaN.
*/
typedef double TValue;

/*! A constant representing unknown values. For testing, use function \c isnan. */
const double undefined_value = std::numeric_limits<double>::quiet_NaN();
#define UNDEFINED_VALUE undefined_value

/*! Compare two values; they are considered the same if the are both
    undefined or they differ by less than 1e-6. Undefined values are
    considered greater than the defined.
 */
inline int values_compare(TValue const u, TValue const v)
{
    if (isnan(u) && isnan(v) ||
        !isnan(u) && !isnan(v) && (fabs(u-v) < 1e-6)) {
        return 0;
    }
    return (isnan(u) || (u>v)) ? 1 : -1;
}

/*! Return \c true if values are compatible, that is, the differ by less
    than 1e-6 or at least one of them is undefined.
 */
inline bool values_compatible(TValue const u, TValue const v)
{
    return isnan(u) || isnan(v) || (fabs(u-v) < 1e-6);
}

#endif

