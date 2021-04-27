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
#include "orvector.px"

/* This function is stolen from Python 2.3 (file listobject.c):
   n between 2^m-1 and 2^m, is round up to a multiple of 2^(m-5) */
int _RoundUpSize(const int &n)
{
	unsigned int nbits = 0;
	unsigned int n2 = (unsigned int)n >> 5;
	do {
		n2 >>= 3;
		nbits += 3;
	} while (n2);
	return ((n >> nbits) + 1) << nbits;
}

bool checkIndex(Py_ssize_t &index, Py_ssize_t max)
{ 
    if ((index<0) || (index>=max)) {
        PyErr_Format(PyExc_IndexError, "index %i out of range", index);
        return false;
    }
    return true;
}


bool checkIndices(Py_ssize_t &start, Py_ssize_t &stop, Py_ssize_t max)
{ 
    if (start > max)
        start = max;
    if (stop > max)
        stop = max;
    return true;
}


bool compareItems(const TOrange *ob1, const TOrange *ob2)
{
    const int res = PyObject_RichCompareBool(AS_PyObject(ob1), AS_PyObject(ob2), Py_LT);
    if (res==-1) {
        throw;
    }
    return res!=0;
}

