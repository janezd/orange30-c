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


#ifndef __COSTMATRIX_HPP
#define __COSTMATRIX_HPP

#include "costmatrix.ppp"

class TCostMatrix : public TOrange {
public:
    __REGISTER_CLASS(CostMatrix);

    PVariable classVar; //P attribute to which the matrix applies
    int dimension; //PR dimension (should equal classVar.noOfValues())

    double *costs;

    TCostMatrix(int const dimension, double const defaultCost = 1.0);
    TCostMatrix(PVariable const &, double const defaultCost = 1.0);
    TCostMatrix(TCostMatrix const &);

    inline const double &cost(int const predicted, int const correct) const;
    inline double &cost(int const predicted, int const correct);
    inline const double &cost(TValue const predicted, TValue const correct) const;
    inline double &cost(TValue const predicted, TValue const correct);
protected:
    void init(double const inside);

public:
    bool setRow(int const row, PyObject *pyrow);
    PyObject *getRow(int const row) const;
    int getIndex(PyObject *arg, char *error) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(dimension | variable, data); construct a new cost matrix");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for pickling");
    PyObject *getcost(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(predicted, correct); get cost");
    PyObject *setcost(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(predicted, correct, cost); set cost");
    PyObject *__item__(Py_ssize_t index) const;
    int __ass_subscript__(PyObject *index, PyObject *value);
    PyObject *__subscript__(PyObject *index) const;
    Py_ssize_t __len__() const;

};


double &TCostMatrix::cost(int const predicted, int const correct)
{ 
    if (predicted >= dimension) {
        raiseError(PyExc_IndexError, "value %i out of range", predicted);
    }
    if (correct >= dimension) {
        raiseError(PyExc_IndexError, "value %i out of range", correct);
    }
    return costs[predicted*dimension + correct];
}


const double &TCostMatrix::cost(int const predicted, int const correct) const
{
    // this cast is safe
    return const_cast<TCostMatrix *>(this)->cost(predicted, correct);
}


const double &TCostMatrix::cost(TValue const predicted, TValue const correct) const
{ return cost(int(predicted), int(correct)); }


double &TCostMatrix::cost(TValue const predicted, TValue const correct)
{ return cost(int(predicted), int(correct)); }

#endif

