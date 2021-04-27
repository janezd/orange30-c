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

#ifndef __KNN_PROJECTED_HPP
#define __KNN_PROJECTED_HPP

#include "common.hpp"
#include "knn_projected.ppp"


class TP2NN : public TClassifier {
public:
    __REGISTER_CLASS(P2NN);

    PFloatList offsets; //P offsets to subtract from the attribute values
    PFloatList normalizers; //P number to divide the values by
    PFloatList averages; //P numbers to use instead of the missing
    bool normalizeExamples; //P if true, attribute values are divided to sum up to 1
    double *bases; // eg x1, y1,  x2, y2,  x3, y3, ... x_dimensions, y_dimensions
    double *radii; // eg sqrt(x1^2+y1^2) ...

    int nExamples; //PR the number of examples
    double *projections; // projections of examples + class
    double minClass, maxClass; //PR the minimal and maximal class value (for regression problems only)

    enum Law { InverseLinear, InverseSquare, InverseExponential, KNN, Linear } PYCLASSCONSTANTS_UP;
    int law; //P law


    TP2NN(PDomain const &, PExampleTable const &egen,
          PFloatList const &basesX, PFloatList const &basesY,
          const int law=InverseSquare, const bool normalizeExamples=true);

    TP2NN(PDomain const &, double *projections, const int nExamples,
        double *bases,
        PFloatList const &off, PFloatList const &norm, PFloatList const &avgs,
        const int law=InverseSquare, const bool normalizeExamples=true);

     // used for pickling: only allocates the memory for the (double *) fields
    TP2NN(PDomain const &, const int nAttrs, const int nExamples);

    TP2NN(TP2NN const &);
    TP2NN &operator =(TP2NN const &);

    ~TP2NN();

    virtual TValue operator ()(TExample const *const);
    virtual PDistribution classDistribution(TExample const *const);

    virtual void classDistribution(
        const double, const double, 
        double *distribution, const int nClasses) const;

    double averageClass(const double x, const double y) const;

    virtual void project(TExample const *const, double &x, double &y);

    inline void getProjectionForClassification(
        TExample const *const, double &x, double &y);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("((data, bases[, normalize_examples, domain]); Construct a new classifier");
    static TOrange *unpickle(PyTypeObject *type, PyObject *args);
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
};


void TP2NN::getProjectionForClassification(
    TExample const *const example, double &x, double &y)
{
    if (example->domain == domain)
        project(example, x, y);
    else {
        PExample nex = example->convertedTo(domain);
        project(nex.borrowPtr(), x, y);
    }
}

#endif
