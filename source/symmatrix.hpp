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


#ifndef __SYMMATRIX_HPP
#define __SYMMATRIX_HPP

#include "symmatrix.ppp"

class TSymMatrix : public TOrange
{
public:
    __REGISTER_CLASS(SymMatrix);

    enum Shape { Lower, Upper, Symmetric, LowerFilled, UpperFilled } PYCLASSCONSTANTS;
    enum Transformation { Negate, SubtractFromOne, SubtractFromMax, Invert } PYCLASSCONSTANTS;
    enum Normalization { Bounds, Sigmoid } PYCLASSCONSTANTS;

    int dim; //PR matrix dimension
    // these matrices can be huge, so we want to save space by using float instead of double
    float *elements; 

    TSymMatrix(int const adim, float const init = 0);
    TSymMatrix(TSymMatrix const &);
    ~TSymMatrix();

    inline int storageSize() const;

    int getindex(int const i, int const j) const;
    inline int getindex_noChecking(int const i, int const j) const;

    static inline void index2coordinates(int const index, int &x, int &y);
    inline void index2coordinates(float const *const f, int &x, int &y) const;

    inline float &getref(int const i, int const j);
    inline const float &getref(int const i, int const j) const;
    inline const float getitem(int const i, int const j) const;

    void getknn(int const i, int const k, vector<int> &knn) const;

    inline void negate();
    inline void subtractFromOne();
    inline void subtractFromMax();
    inline void invert();
    void normalize(int const type);

    TSymMatrix *subMatrix(vector<int> const &) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(data, default) | (dim, value); construct a matrix");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    PyObject *flat() const PYARGS(METH_NOARGS, "(); return a matrix as a 1-D list");
    PyObject *getKNN(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(i, k); return indices of k nearest neighbours of i");
    PyObject *avgLinkage(PyObject *clusters) const PYARGS(METH_O, "(clusters); return average linkage distances between the given clusters");
    PyObject *py_negate() PYARGS(METH_NOARGS, "(); negate a matrix");
    PyObject *py_subtractFromOne() PYARGS(METH_NOARGS, "(); subtract matrix from 1");
    PyObject *py_subtractFromMax() PYARGS(METH_NOARGS, "(); subtract a matrix from maximal element");
    PyObject *py_invert(PyObject *trans) PYARGS(METH_VARARGS, "(); invert a matrix");
    PyObject *py_normalize(PyObject *trans) PYARGS(METH_O, "(type); normalize a matrix");

    PyObject *get_items(PyObject *py_indices) const PYARGS(METH_O, "(indices); return selected rows/columns");
    bool getIndex(PyObject *index, int &i, int &j) const;
    PyObject *__subscript__(PyObject *index) const;
    int __ass_subscript__(PyObject *index, PyObject *val);
    PyObject *__str__();

};


int TSymMatrix::storageSize() const
{
    return (dim * (dim+1)) / 2;
}


int TSymMatrix::getindex_noChecking(int const i, int const j) const
{
    return i < j ? ((j*(j+1))>>1) + i : ((i*(i+1))>>1) + j;
}

void TSymMatrix::index2coordinates(int const index, int &x, int &y)
{
    x = int(floor( (sqrt(float(1+8*index)) -1) / 2));
    y = index - (x*(x+1))/2;
}

void TSymMatrix::index2coordinates(float const *const f, int &x, int &y) const
{ index2coordinates(f-elements, x, y); }


float &TSymMatrix::getref(int const i, int const j)
{ 
    return elements[getindex(i, j)]; 
}

float const &TSymMatrix::getref(int const i, int const j) const
{ 
    return elements[getindex(i, j)];
}

float const TSymMatrix::getitem(int const i, int const j) const
{ 
    return elements[getindex(i, j)];
}


void TSymMatrix::negate()
{
    if (dim) {
        float *e = elements;
        for(int i = storageSize(); i--; e++) {
            *e = -*e;
        }
    }
}

void TSymMatrix::subtractFromOne()
{
    if (dim) {
        float *e = elements;
        for(int i = storageSize(); i--; e++) {
            *e = 1 - *e;
        }
    }
}

void TSymMatrix::subtractFromMax()
{
    if (dim) {
        float *e = elements;
        int i = storageSize();
        float const maxVal = *max_element(e, e+i);
        for(; i--; e++) {
            *e = maxVal - *e;
        }
    }
}

void TSymMatrix::invert()
{
    if (dim) {
        float *e = elements;
        for(int i = storageSize(); i--; e++) {
            if (*e < 1e-6) {
                int i, j;
                index2coordinates(e, i, j);
                raiseError(PyExc_ZeroDivisionError,
                    "division by zero at element [%i, %i]", i, j);
            }
            *e = 1 / *e;
        }
    }
}



#endif
