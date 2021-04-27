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
#include "symmatrix.px"

TSymMatrix::TSymMatrix(int const adim, float const init)
: dim(adim),
  elements(NULL)
{ 
    if (dim) {
        int const sze = storageSize();
        elements = new float[sze];
        fill_n(elements, sze, init);
    }
}


TSymMatrix::TSymMatrix(TSymMatrix const &other)
: dim(other.dim),
  elements(NULL)
{
    if (dim) {
        int const sze = storageSize();
        elements = new float[sze];
        memcpy(elements, other.elements, sze*sizeof(float));
    }
}


TSymMatrix::~TSymMatrix()
{ 
    if (elements) {
        delete elements;
    }
}


int TSymMatrix::getindex(int const i, int const j) const
{
    if (i == j) {
        if ((i >= dim) || (i < 0)) {
            raiseError(PyExc_IndexError,
                "index [%i, %i] out of range", i, j);
        }
        return (i*(i+3))>>1;
    }
    if (i > j) {
        if ((i >= dim) || (j < 0)) {
            raiseError(PyExc_IndexError,
                "index [%i, %i] out of range", i, j);
        }
        return ((i*(i+1))>>1) + j;
    }
    else {
        if ((j >= dim) || (i < 0))
            raiseError(PyExc_IndexError,
            "index [%i, %i] out of range", i, j);
        return  ((j*(j+1))>>1) + i;
    }
}


typedef std::pair<int, float> coord_t;
struct pkt_less {
    inline bool operator ()(const coord_t &e1, const coord_t &e2) const {
        return (e1.second < e2.second);
    }
};


void TSymMatrix::getknn(int const i, int const k, vector<int> &knn) const
{   
    knn.clear();
    if (!dim) {
        raiseError(PyExc_IndexError, "index %i out of range", i);
    }
    vector<coord_t> knn_tmp;
    int j;
    for (j = 0; j < dim; j++) {
        if (j != i) {
            knn_tmp.push_back(coord_t(j, elements[getindex_noChecking(i, j)]));
        }
    }
    sort(knn_tmp.begin(), knn_tmp.end(), pkt_less());
    vector<coord_t>::const_iterator ci(knn_tmp.begin());
    vector<coord_t>::const_iterator ce(
        k < knn_tmp.size() ? ci+k : ci+knn_tmp.size());
    while(ci != ce) {
        knn.push_back(ci++->first);
    }
}


void TSymMatrix::normalize(int const type)
{
    if (!dim) {
        return;
    }
    if (type == Bounds) {
        int i, sze = storageSize();
        float minVal = *elements;
        float maxVal = *elements;
        float *e;
        for(i = sze, e = elements; i--; e++) {
            if (*e < minVal) {
                minVal = *e;
            }
            else if (*e > maxVal) {
                maxVal = *e;
            }
        }
        if (minVal == maxVal) {
            fill_n(elements, storageSize(), float(0));
            return;
        }
        else {
            float fact = 1 / (maxVal - minVal);
            for(i = sze, e = elements; i--; e++) {
                *e = (*e - minVal) * fact;
            }
        }
    }
    else if (type == Sigmoid) {
        int i = storageSize();
        for(float *e = elements; i--; e++) {
            *e = 1 / (1 + exp(-*e));
        }
    }
    else {
        raiseError(PyExc_ValueError, "invalid normalization type");
    }
}


TSymMatrix *TSymMatrix::subMatrix(vector<int> const &rows) const
{
    const_ITERATE(vector<int>, ri, rows) {
        if ((*ri < 0) || (*ri >= dim)) {
            raiseError(PyExc_IndexError, "index %i out of range", *ri);
        }
    }
    int const newdim = rows.size();
    TSymMatrix *newMatrix = new TSymMatrix(newdim);
    float *newEl = newMatrix->elements;
    vector<int>::const_iterator rb(rows.begin()), re(rows.end());
    for(vector<int>::const_iterator ri1(rb); ri1 != re; ri1++) {
        for(vector<int>::const_iterator ri2(rb); ri2 <= ri1; ri2++) {
            *newEl++ = elements[getindex_noChecking(*ri1, *ri2)];
        }
    }
    return newMatrix;
}



char *keywords_dim[] = {"dim", "value", NULL};
char *keywords_data[] = {"data", "default", NULL};

TOrange *TSymMatrix::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    int dim;
    float init = 0;
    if (PyArg_ParseTupleAndKeywords(args, kw, "i|f:SymMatrix", keywords_dim, &dim, &init)) {
        if (dim < 0) {
            PyErr_Format(PyExc_ValueError,
                "matrix dimension must be positive, not %i", dim);
            return NULL;
        }
        return new(type) TSymMatrix(dim, init);
    }
    PyErr_Clear();

    TByteStream buf;
    if (PyArg_ParseTuple(args, "O&:SymMatrix", &TByteStream::argconverter, &buf)) {
        size_t sze = buf.bufe - buf.buf;
        int dim = int(floor(0.4 + (sqrt(double(1 + 8*sze/sizeof(float))) - 1) / 2)) ;
        TSymMatrix *symmatrix = new(type) TSymMatrix(dim);
        buf.readBuf(symmatrix->elements, sze);
        return symmatrix;
    }
    PyErr_Clear();

    PSymMatrix old;
    if (PyArg_ParseTuple(args, "O&:SymMatrix", &PSymMatrix::argconverter, &old)) {
        return new(type) TSymMatrix(*old);
    }
    PyErr_Clear();

    PyObject *arg;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O|f:SymMatrix", keywords_data, &arg, &init)) {
        dim = PySequence_Size(arg);
        PyObject *iter = PyObject_GetIter(arg);
        if ((dim < 0) || !iter) {
            Py_XDECREF(iter);
            PyErr_Format(PyExc_TypeError,
                "SymMatrix expects data or the dimension, and optional default value");
            return NULL;
        }
        GUARD(iter);
        TSymMatrix *symmatrix = 
            new TSymMatrix(dim, numeric_limits<float>::quiet_NaN());
        int i, j;
        for(i = 0; i < dim; i++) {
            PyObject *item = PyIter_Next(iter);
            if (!item) {
                delete symmatrix;
                PyErr_Format(PyExc_SystemError,
                    "matrix is shorter than expected (%i < %i", i, dim);
                return NULL;
            }
            PyObject *subiter = PyObject_GetIter(item);
            Py_DECREF(item);
            if (!subiter) {
                delete symmatrix;
                PyErr_Format(PyExc_TypeError, "row %i is not a sequence", i);
                return NULL;
            }
            GUARD(subiter);
            for(j = 0; j < dim; j++) {
                float &mae = symmatrix->elements[symmatrix->getindex_noChecking(i, j)];
                PyObject *subitem = PyIter_Next(subiter);
                if (!subitem) {
                    if (j <= i) {
                        fill_n(&mae, i-j+1, init);
                    }
                    break;
                }
                GUARD(subitem);
                double f;
                if (!PyNumber_ToDouble(subitem, f)) {
                    delete symmatrix;
                    PyErr_Format(PyExc_TypeError,
                        "element at (%i, %i) is an instance of '%s' instead of number",
                        i, j, subitem->ob_type->tp_name);
                    return NULL;
                }
                if (isnan(mae) && (mae != f)) {
                    delete symmatrix;
                    PyErr_Format(PyExc_TypeError,
                        "the element at (%i, %i) is asymmetric (%f != %f)",
                        i, j, mae, f);
                        return NULL;
                }
                mae = f;
            }
        }
        return symmatrix;
    }
    PyErr_Clear();

    PyErr_Format(PyExc_TypeError,
        "SymMatrix expects data or dimension, and optional default value");
    return NULL;
}


PyObject *TSymMatrix::__getnewargs__() const
{
    size_t const sze = storageSize()*sizeof(float);
    TByteStream buf(sze);
    buf.writeBuf(elements, sze);
    return Py_BuildValue("(y#)", buf.buf, buf.length());
}
    

PyObject *TSymMatrix::flat() const
{
    int sze = storageSize();
    PyObject *complist = PyList_New(sze);
    Py_ssize_t ind = 0;
    float *ei = elements;
    while(ind < sze) {
        PyList_SetItem(complist, ind++, PyFloat_FromDouble(*ei++));
    }
    return complist;
}


PyObject *TSymMatrix::getKNN(PyObject *args, PyObject *kw) const
{
    int i, k;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "ii:getKNN",
        SymMatrix_getKNN_keywords, &i, &k)) {
            return NULL;
    }
    vector<int> neighbours;
    getknn(i, k, neighbours);
    return convertToPython(neighbours);
}


PyObject *TSymMatrix::avgLinkage(PyObject *clusters) const
{
    // All checks first to avoid having to do it later
    if (!PyList_Check(clusters)) {
        return PyErr_Format(PyExc_TypeError,
            "avg_linkage expects a list, not '%s'", clusters->ob_type->tp_name);
    }
    int const nClusters = PyList_Size(clusters);
    for(int i = 0; i < nClusters; i++) {
        PyObject *cluster = PyList_GetItem(clusters, i);
        if (!PyList_Check(cluster)) {
            return PyErr_Format(PyExc_TypeError,
                "element at index %i must be a list, not an instance of %s",
                i, cluster->ob_type->tp_name);
        }
        Py_ssize_t sze = PyList_Size(cluster);
        if (!sze) {
            return PyErr_Format(PyExc_ValueError, "cluster %i is empty", i);
        }
        for(Py_ssize_t j = 0; j < sze; j++) {
            PyObject *el = PyList_GetItem(cluster, j);
            if (!PyLong_Check(el)) {
                return PyErr_Format(PyExc_TypeError,
                    "element [%i, %i] must be an int, not an instance of %s",
                    i, j, el->ob_type->tp_name);
            }
            int const val = (int)PyLong_AsLong(el);
            if (val >= dim) {
                return PyErr_Format(PyExc_IndexError,
                    "element [%i, %i] is out of range (%i >= %i)",
                    i, j, val, dim);
            }
        }
    }
    TSymMatrix *symmatrix = new TSymMatrix(nClusters);
    for (int i = 0; i < nClusters; i++) {
        PyObject *const cluster_i = PyList_GetItem(clusters, i);
        int const size_i = PyList_Size(cluster_i);
        for (int j = i; j < nClusters; j++) {
            PyObject *const cluster_j = PyList_GetItem(clusters, j);
            int const size_j = PyList_Size(cluster_j);
            float &sum = symmatrix->elements[getindex_noChecking(i, j)];
            for (int k = 0; k < size_i; k++) {
                int const item_k = (int)PyLong_AsLong(PyList_GetItem(cluster_i, k));
                for (int l = 0; l < size_j; l++) {
                    int const item_l = (int)PyLong_AsLong(PyList_GetItem(cluster_j, l));
                    sum += elements[getindex_noChecking(item_k, item_l)];
                }
            }
            sum /= (size_i * size_j);
        }
    }
    return PyObject_FromNewOrange(symmatrix);
}


PyObject *TSymMatrix::py_negate()
{
    negate();
    Py_RETURN_NONE;
}


PyObject *TSymMatrix::py_subtractFromOne()
{
    subtractFromOne();
    Py_RETURN_NONE;
}


PyObject *TSymMatrix::py_subtractFromMax()
{
    subtractFromMax();
    Py_RETURN_NONE;
}


PyObject *TSymMatrix::py_invert(PyObject *args)
{
    int trans = Invert;
    if (!PyArg_ParseTuple(args, "|i:invert", &trans)) {
        return NULL;
    }
    switch (trans) {
        case Negate: negate(); break;
        case SubtractFromOne: subtractFromOne(); break;
        case SubtractFromMax: subtractFromMax(); break;
        case Invert: invert(); break;
        default:
            return PyErr_Format(PyExc_ValueError, "invalid transformation type");
    }
    Py_RETURN_NONE;
}


PyObject *TSymMatrix::py_normalize(PyObject *trans)
{
    if (!PyLong_Check(trans)) {
        return PyErr_Format(PyExc_TypeError,
            "normalization type should be a number, not an instance of '%s'",
            trans->ob_type->tp_name);
    }
    normalize(PyLong_AsLong(trans));
    Py_RETURN_NONE;
}


PyObject *TSymMatrix::get_items(PyObject *py_indices) const
{
    vector<int> indices;
    convertFromPython(py_indices, indices);
    return PyObject_FromNewOrange(subMatrix(indices));
}


bool TSymMatrix::getIndex(PyObject *index, int &i, int &j) const
{
    if (!PyTuple_Check(index)) {
        PyErr_Format(PyExc_TypeError,
            "indices should be tuples, not instances of '%s'",
            index->ob_type->tp_name);
        return false;
    }
    if (PyTuple_GET_SIZE(index) != 2) {
        PyErr_Format(PyExc_TypeError, "symmetric matrices are two-dimensional");
        return false;
    }
    i = (int)PyLong_AsLong(PyTuple_GET_ITEM(index, 0));
	j = (int)PyLong_AsLong(PyTuple_GET_ITEM(index, 1));
    return true;
}


PyObject *TSymMatrix::__subscript__(PyObject *index) const
{
    int i, j;
    if (!getIndex(index, i, j)) {
        return NULL;
    }
    return PyFloat_FromDouble(getitem(i, j));
}


int TSymMatrix::__ass_subscript__(PyObject *index, PyObject *val)
{
    int i, j;
    if (!getIndex(index, i, j)) {
        return -1;
    }
    getref(i, j) = val ? PyNumber_AsDouble(val) : 0;
    return 0;
}


PyObject *TSymMatrix::__str__()
{ 
    if (!dim) {
        return PyUnicode_FromString("()");
    }

    float *ei = elements;
    float matmax = *ei;
    for(int i = storageSize(); i--; ei++) {
        float const tei = *ei<0 ? fabs(10.0 * *ei) : *ei;
        if (tei > matmax)
            matmax = tei;
    }
    int const plac = 4 + (fabs(matmax) < 1 ? 1 : int(ceil(log10((double)matmax))));
    string res = "(";
    char buf[32];
    int i = 0, j;
    ei = elements;
    while(i < dim) {
        res += "(";
        for(j = 0;; j++) {
            sprintf(buf, "%*.3f", plac, *ei++);
            res += buf;
            if (j != i) {
                res += ", ";
            }
            else {
                if (++i < dim) {
                    res += "),\n";
                }
                else {
                    res += "))";
                }
                break;
            }
        }
    }
    return PyUnicode_FromString(res.c_str());
}