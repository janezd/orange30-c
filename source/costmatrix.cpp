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
#include "costmatrix.px"

void TCostMatrix::init(double const defaultCost)
{ 
    delete costs;
    const int size = dimension * dimension;
    costs = new double[size];
    fill(costs, costs + size, defaultCost);
    int dim = dimension;
    for(double *ci = costs; dim--; ci += dimension+1) {
        *ci = 0;
    }
}


TCostMatrix::TCostMatrix(int const dim, double const defaultCost)
    : dimension(dim),
    costs(NULL)
{
    if (dimension <= 0) {
        raiseError(PyExc_ValueError, "invalid dimension (%i)", dimension);
    }
    init(defaultCost);
}


TCostMatrix::TCostMatrix(PVariable const &acv, double const defaultCost)
    : classVar(acv),
    dimension(0),
    costs(NULL)
{ 
    if (classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError,
            "attribute '%s' is not discrete", classVar->cname());
    }
    dimension = acv->noOfValues();
    if (!dimension) {
        raiseError(PyExc_ValueError,
            "attribute '%s' has no values", classVar->cname());
    }
    init(defaultCost);
}


TCostMatrix::TCostMatrix(TCostMatrix const &old)
    : classVar(old.classVar),
    dimension(old.dimension)
{
    costs = new double[dimension*dimension];
    copy(old.costs, old.costs + dimension*dimension, costs);
}



bool readCostMatrix(PyObject *arg, TCostMatrix *&matrix)
{
	return true;
}


TOrange *TCostMatrix::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *data = NULL;
    PCostMatrix matrix;

    if (PyTuple_Size(args) == 1) {
        data = PyTuple_GET_ITEM(args, 0);
        if (PyLong_Check(data)) {
            return new(type) TCostMatrix(PyLong_AsLong(data));
        }
        if (OrVariable_Check(data)) {
            return new(type) TCostMatrix(PVariable(data));
        }
        if (OrCostMatrix_Check(data)) {
            return new(type) TCostMatrix(((OrCostMatrix *)data)->orange);
        }
        // if not, data must be a sequence containing the data
    }
    if (PyTuple_Size(args) == 2) {
        PyObject *arg1 = PyTuple_GetItem(args, 0);
        data = PyTuple_GetItem(args, 1);

        double inside;
        if (PyNumber_ToDouble(data, inside)) {
            if (PyLong_Check(arg1)) {
                return new(type) TCostMatrix(PyLong_AsLong(arg1), inside);
            }
            if (OrVariable_Check(arg1)) {
                return new(type) TCostMatrix(PVariable(arg1), inside);
            }
            else {
                raiseError(PyExc_TypeError,
                    "the first argument must be dimension or variable, not '%s'",
                    arg1->ob_type->tp_name);
            }
        }
        if (PyLong_Check(arg1)) {
             matrix = PCostMatrix(
                 new(type) TCostMatrix(PyLong_AsLong(arg1), inside));
        }
        else if (OrVariable_Check(arg1)) {
            matrix = PCostMatrix(new(type) TCostMatrix(PVariable(arg1)));
        }
        else {
            raiseError(PyExc_TypeError,
                "the first argument must be dimension or variable, not '%s'",
                arg1->ob_type->tp_name);
        }
    }

    const int arglength = PyObject_Length(data);
    if (matrix) {
        if ((arglength >= 0) && (arglength != matrix->dimension)) {
            PyErr_Format(PyExc_IndexError,
                "invalid sequence length (expected %i, got %i)",
                matrix->dimension, arglength);
            return NULL;
        }
        else {
            PyErr_Clear();
        }
    }
    else {
        if (arglength < 0) {
            return NULL;
        }
        matrix = PCostMatrix(new(type) TCostMatrix(arglength));
    }
	PyObject *iter = PyObject_GetIter(data);
	if (!iter) {
        PyErr_Format(PyExc_TypeError,
            "expected sequence, not '%s'", data->ob_type->tp_name);
        return NULL;
    }
	int i;
    PyObject *item;
	for(i = 0; i < matrix->dimension; i++) {
		item = PyIter_Next(iter);
		if (!item) {
			PyErr_Format(PyExc_IndexError,
                "data is too short (%i rows expected)", matrix->dimension);
            Py_DECREF(iter);
            return NULL;
		}
        if (!matrix->setRow(i, item)) {
            Py_DECREF(item);
            Py_DECREF(iter);
            return NULL;
        }
        Py_DECREF(item);
    }
    item = PyIter_Next(iter);
    Py_DECREF(iter);
    if (item) {
        Py_DECREF(item);
		PyErr_Format(PyExc_IndexError,
            "data is too long (%i rows expected)", matrix->dimension);
        return NULL;
    }
    return matrix.getPtr();
}


bool TCostMatrix::setRow(int const row, PyObject *pyrow)
{
    PyObject *iter = PyObject_GetIter(pyrow);
    if (!iter) {
        PyErr_Format(PyExc_TypeError, "cost matrix row must be a sequence");
        return false;
    }
    PyObject *item;
    vector<double> data(dimension);
    ITERATE(vector<double>, ci, data) {
        item = PyIter_Next(iter);
        if (!item) {
            PyErr_Format(PyExc_IndexError,
                "row is too short (%i elements expected)", dimension);
            Py_DECREF(iter);
            return false;
        }
        bool ok = PyNumber_ToDouble(item, *ci);
        Py_DECREF(item);
        if (!ok) {
            Py_DECREF(iter);
            PyErr_Format(PyExc_TypeError,
                "element of cost matrix must be a number, not '%s'",
                item->ob_type->tp_name);
            return false;
        }
    }
    item = PyIter_Next(iter);
    Py_DECREF(iter);
    if (item) {
        Py_DECREF(item);
		PyErr_Format(PyExc_IndexError,
            "data is too long (%i columns expected)", dimension);
        return false;
    }
    copy(data.begin(), data.end(), costs + row*dimension);
    return true;
}


PyObject *TCostMatrix::getRow(int const row) const
{
    double *ci = costs + dimension*row;
    PyObject *pyrow = PyList_New(dimension);
    for(int j = 0; j < dimension; j++, ci++) {
        PyList_SetItem(pyrow, j, PyFloat_FromDouble(*ci));
    }
    return pyrow;
}


PyObject *TCostMatrix::__getnewargs__() const
{
    PyObject *data = PyList_New(dimension);
    for(int i = 0; i < dimension; i++) {
        PyList_SetItem(data, i, getRow(i));
    }
    if (classVar) {
        return Py_BuildValue("NN", classVar.getPyObject(), data);
    }
    else {
        return Py_BuildValue("iN", dimension, data);
    }
}


int TCostMatrix::getIndex(PyObject *arg, char *whichVal) const
{
    if (PyLong_Check(arg)) {
        int pred = PyLong_AsLong(arg);
        if ((pred < 0) || (pred >= dimension)) {
            PyErr_Format(PyExc_IndexError,
                "%s value index %i is out of range", whichVal, pred);
            return -1;
        }
        return pred;
    }
    if (classVar) {
        return int(classVar->py2val(arg));
    }
    PyErr_Format(PyExc_IndexError,
        "%s value index should be a number, not '%s'",
        whichVal, arg->ob_type->tp_name);
    return -1;
}


PyObject *TCostMatrix::getcost(PyObject *args, PyObject *kw) const
{
    PyObject *pypred, *pycorr;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO",
        CostMatrix_getcost_keywords, &pypred, &pycorr)) {
            return NULL;
    }
    int pred = getIndex(pypred, "predicted");
    int corr = getIndex(pycorr, "correct");
    if ((pred==-1) || (corr==-1)) {
        return NULL;
    }
    return PyFloat_FromDouble(cost(pred, corr));
}


PyObject *TCostMatrix::setcost(PyObject *args, PyObject *kw)
{
    PyObject *pypred, *pycorr;
    double ncost;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OOd",
        CostMatrix_setcost_keywords, &pypred, &pycorr, &ncost)) {
            return NULL;
    }
    int pred = getIndex(pypred, "predicted");
    int corr = getIndex(pycorr, "correct");
    if ((pred==-1) || (corr==-1)) {
        return NULL;
    }
    cost(pred, corr) = ncost;
    Py_RETURN_NONE;
}


PyObject *TCostMatrix::__item__(Py_ssize_t index) const
{
    if ((index < 0) || (index >= dimension)) {
        return PyErr_Format(PyExc_IndexError, "index %i out of range", index);
    }
    return getRow(index);
}


PyObject *TCostMatrix::__subscript__(PyObject *index) const
{
    if (PyTuple_Check(index)) {
        // Better raise exception here; getcost's message would be confusing
        if (PyTuple_Size(index) != 2) {
            return PyErr_Format(PyExc_IndexError,
                "cost matrix is two dimensional");
        }
        return getcost(index, NULL);
    }
    int pred = getIndex(index, "predicted");
    if (pred == -1) {
        return NULL;
    }
    return getRow(pred);
}

int TCostMatrix::__ass_subscript__(PyObject *index, PyObject *value)
{
    if (PyTuple_Check(index)) {
        if (PyTuple_Size(index) != 2) {
            PyErr_Format(PyExc_IndexError, "cost matrix is two dimensional");
            return -1;
        }
        PyObject *args = PyTuple_Pack(3,
            PyTuple_GET_ITEM(index, 0), PyTuple_GET_ITEM(index, 1), value);
        try {
            setcost(args, NULL);
        }
        catch (...) {
            Py_DECREF(args);
            throw;
        }
        Py_DECREF(args);
        return 0;
    }
    int pred = getIndex(index, "predicted");
    setRow(pred, value);
    return 0;
}


Py_ssize_t TCostMatrix::__len__() const
{
    return dimension;
}
