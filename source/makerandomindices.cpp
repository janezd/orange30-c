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

// to include Python.h before STL defines a template set (doesn't work with VC 6.0)
#include "common.hpp" 
#include "makerandomindices.px"


class rsrgen {
public:
  PRandomGenerator randomGenerator;

  rsrgen(int const seed)
  : randomGenerator(PRandomGenerator(
        new TRandomGenerator((unsigned long)(seed>=0 ? seed : 0))))
  {}

  rsrgen(PRandomGenerator const &rgen)
  : randomGenerator(rgen ? rgen : PRandomGenerator(new TRandomGenerator()))
  {}

  rsrgen(PRandomGenerator const &rgen, int const seed)
  : randomGenerator(rgen ? 
        rgen
        : PRandomGenerator(
              new TRandomGenerator((unsigned long)(seed>=0 ? seed : 0))))
  {}

  int operator()(int n)
  { 
      return randomGenerator->randint(n);
  }
};


/*! Construct an instance of MakeRandomIndices, set the stratification
    and random seed. */ 
TMakeRandomIndices::TMakeRandomIndices(
    int const astratified, int const arandseed)
: stratified(astratified),
  randseed(arandseed)
{}


/*! Construct an instance of MakeRandomIndices, set the stratification
    and random generator. */ 
TMakeRandomIndices::TMakeRandomIndices(
    int const astratified, PRandomGenerator const &randgen)
: stratified(astratified),
  randseed(-1),
  randomGenerator(randgen)
{}


PyObject *TMakeRandomIndices::__call__(PyObject *args, PyObject *kw)
{
    // This function would be simple if it didn't have to deal with
    // compatibility with ridiculous arguments from the previous version
    PExampleTable data;
    int n = -1;
    if (kw) {
        PyObject *pydata = PyDict_GetItemString(kw, "data");
        PyObject *pyn = PyDict_GetItemString(kw, "n");
        if (pydata || pyn) {
            if (args && PyTuple_Size(args)) {
                raiseError(PyExc_TypeError,
                    "invalid arguments; positional and keywords arguments are given");
            }
            if (pydata && pyn) {
                raiseError(PyExc_TypeError,
                    "MakeRandomIndices must not be given both 'data' and 'n'");
            }
            if (pydata) {
                if (!OrExampleTable_Check(pydata)) {
                    raiseError(PyExc_TypeError,
                        "argument 'data' must be an ExampleTable, not '%s'",
                        pydata->ob_type->tp_name);
                }
                data = PExampleTable(pydata);
            }
            else {
                if (!PyLong_Check(pyn)) {
                    raiseError(PyExc_TypeError,
                        "number of examples must be integer, not '%s'",
                        pydata->ob_type->tp_name);
                }
                n = PyLong_AsLong(pyn);
                if (n < 0) {
                    raiseError(PyExc_ValueError,
                        "number of examples must not be negative");
                }
            }
            if (PyDict_Size(kw) > 1) {
                PyObject *wargs = PyDict_Copy(kw);
                PyDict_DelItemString(wargs, pydata ? "data" : "n");
                try {
                    setAttr_FromDict(THIS_AS_PyObject, wargs);
                }
                catch (...) {
                    Py_DECREF(wargs);
                    throw;
                }
                Py_DECREF(wargs);
            }
        }
        else {
            setAttr_FromDict(THIS_AS_PyObject, kw);
        }
    }
    if (!data && (n < 0)) {
        if (!args || (PyTuple_Size(args) != 1)) {
            raiseError(PyExc_TypeError,
                "MakeRandomIndices takes one argument (%i given)",
                PyTuple_Size(args));
        }
        PyObject *arg = PyTuple_GET_ITEM(args, 0);
        if (OrExampleTable_Check(arg)) {
            data = PExampleTable(arg);
        }
        else if (PyLong_Check(arg)) {
            n = PyLong_AsLong(arg);
            if (n < 0) {
                raiseError(PyExc_ValueError,
                    "number of examples must not be negative");
            }
        }
        else {
            raiseError(PyExc_TypeError,
                "MakeRandomIndices takes ExampleTable or integer, not '%s'",
                arg->ob_type->tp_name);
        }
    }
    if (data) {
        return (*this)(data).getPyObject();
    }
    else 
        return (*this)(n).getPyObject();
}


/*! Construct and instance of TMakeRandomIndices2. */
TMakeRandomIndices2::TMakeRandomIndices2(
    double const ap0, int const astratified, int const arandseed)
: TMakeRandomIndices(astratified, arandseed),
  p0(ap0)
{}


/*! Construct and instance of TMakeRandomIndices2. */
TMakeRandomIndices2::TMakeRandomIndices2(
    double const ap0, int const astratified, PRandomGenerator const &randgen)
: TMakeRandomIndices(astratified, randgen),
  p0(ap0)
{}


PIntList TMakeRandomIndices2::operator()(int const n) const
{ 
    if (stratified == Stratified) {
        raiseError(PyExc_ValueError,
            "cannot prepare stratified indices without data");
    }
    int no = (p0 <= 1.0) ? int(p0*n+0.5) : int(p0+0.5);
    if (no > n) {
        no=n;
    }
    PIntList indices(new TIntList(no, 0));
    indices->resize(n, 1);
    rsrgen rg(randomGenerator, randseed);
    or_random_shuffle(indices->begin(), indices->end(), rg);
    return indices;
}


PIntList TMakeRandomIndices2::operator()(PExampleTable const &gen) const
{
    if (   (stratified == NotStratified) 
        || (stratified == StratifiedIfPossible) &&
              (!gen->domain->classVar ||
              (gen->domain->classVar->varType != TVariable::Discrete))) {
        return (*this)(gen->size());
    }
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError,
            "cannot prepare stratified indices for data without the class");
    }
    if (gen->domain->classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError, 
            "cannot prepare stratified indices for data without a discrete class");
    }
    double ap0;
    if (p0 > 1.0) {
        ap0 = p0/double(gen->size());
        if (ap0 > 1) {
            raiseError(PyExc_ValueError,
                "p0 exceeds the number of examples (%i > %i)",
                int(ap0), gen->size());
        }
    }
    else {
        ap0 = p0;
    }
    double ap1 = 1 - ap0;

    TExampleIterator ri(gen->begin());
    if (!ri) {
        return PIntList(new TIntList());
    }
    typedef pair<int, int> pii; // index of example, class value
    vector<pii> ricv;
    for(int in=0; ri; ++ri) {
        TValue const classVal = ri.getClass();
        if (isnan(classVal)) {
            if (stratified == StratifiedIfPossible) {
                return (*this)(gen->size());
            }
            else {
                raiseError(PyExc_ValueError,
                    "cannot prepare stratified indices due to unknown classes");
            }
        }
        ricv.push_back(pii(in++, int(classVal)));
    }
    random_sort(ricv.begin(), ricv.end(),
        predOn2nd<pair<int, int>, less<int> >(),
        predOn2nd<pair<int, int>, equal_to<int> >(),
        rsrgen(randomGenerator, randseed));
    PIntList indices(new TIntList());
    indices->resize(gen->size());
    double rem = 0;
    ITERATE(vector<pii>, ai, ricv) {
        if (rem <= 0) { 
            indices->at(ai->first) = 1;
            rem += ap0;
        }
        else {
            indices->at(ai->first) = 0;
            rem -= ap1;
        }
        // E.g., if p0 is two times p1, two 0's will cancel one 1.
    }
    return indices;
}



TMakeRandomIndicesN::TMakeRandomIndicesN(
    int const astrat, int const randseed)
: TMakeRandomIndices(astrat, randseed)
{}


TMakeRandomIndicesN::TMakeRandomIndicesN(
    int const astrat, PRandomGenerator const &randgen)
: TMakeRandomIndices(astrat, randgen)
{}


TMakeRandomIndicesN::TMakeRandomIndicesN(
    PFloatList const &ap, int const astrat, int const randseed)
: TMakeRandomIndices(astrat, randseed),
  p(ap)
{}


TMakeRandomIndicesN::TMakeRandomIndicesN(
    PFloatList const &ap, int const astrat, PRandomGenerator const &randgen)
: TMakeRandomIndices(astrat, randgen),
  p(ap)
{}


PIntList TMakeRandomIndicesN::operator()(PExampleTable const &gen) const
{ 
    return (*this)(gen->size());
}


PIntList TMakeRandomIndicesN::operator()(int const n) const
{ 
    if (!p || !p->size()) {
        raiseError(PyExc_ValueError, "'p' not defined or empty");
    }
    if (stratified == Stratified) {
        raiseError(PyExc_ValueError, "stratification not implemented");
    }
    double sum = 0;
    bool props = true;
    PITERATE(TFloatList, pis, p) {
        sum += *pis;
        if (*pis > 1.0) {
            props = false;
        }
    }
    PIntList indices(new TIntList);
    indices->reserve(n);
    int ss=0;
    if (props) {
        if (sum >= 1.0) {
            raiseError(PyExc_ValueError,
                "sum of 'p' must be below 1, not '%.3f'", sum);
        }
        PITERATE(TFloatList, pi, p) {
            indices->resize(indices->size()+int(*pi*n+0.5), ss++);
        }
    }
    else {
        if (sum > n) {
            raiseError(PyExc_ValueError,
                "sum of 'p', %.3f, exceeds the number of examples", sum);
        }
        PITERATE(TFloatList, pi, p) {
            indices->resize(indices->size()+int(*pi+0.5), ss++);
        }
    }
    indices->resize(n, ss);
    rsrgen rg(randomGenerator, randseed);
    or_random_shuffle(indices->begin(), indices->end(), rg);
    return indices;
}


// Prepares a vector of indices for f-fold cross validation with n examples
TMakeRandomIndicesCV::TMakeRandomIndicesCV(
    int const afolds, int const astratified, int const arandseed)
: TMakeRandomIndices(astratified, arandseed),
  folds(afolds)
{}


TMakeRandomIndicesCV::TMakeRandomIndicesCV(
    int const afolds, int const astratified, PRandomGenerator const &randgen)
: TMakeRandomIndices(astratified, randgen),
  folds(afolds)
{}


PIntList TMakeRandomIndicesCV::operator()(int const n) const
{ 
    if (stratified == Stratified) {
        raiseError(PyExc_ValueError,
            "cannot prepare stratified indices without data");
    }
    if (folds <= 0) {
        raiseError(PyExc_ValueError, "invalid number of folds (%i)", folds);
    }
    PIntList indices(new TIntList);
    indices->reserve(n);
    int const infold = n/folds;
    int ss;
    for(ss=0; ss<folds; ss++) {
        indices->resize((ss+1)*infold, ss);
    }
    ss=0;
    while(indices->size() < n) {
        indices->push_back(ss++);
    }
    rsrgen rg(randomGenerator, randseed);
    or_random_shuffle(indices->begin(), indices->end(), rg);
    return indices;
}


PIntList TMakeRandomIndicesCV::operator()(PExampleTable const &gen) const
{
    if (folds <= 0) {
        raiseError(PyExc_ValueError, "invalid number of folds (%i)", folds);
    }
    if (   (stratified == NotStratified) 
        || (stratified == StratifiedIfPossible) &&
              (!gen->domain->classVar ||
              (gen->domain->classVar->varType != TVariable::Discrete))) {
        return (*this)(gen->size());
    }
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError,
            "cannot prepare stratified indices for data without the class");
    }
    if (gen->domain->classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError, 
            "cannot prepare stratified indices for data without a discrete class");
    }
    TExampleIterator ri(gen->begin());
    if (!ri) {
        return PIntList(new TIntList());
    }
    typedef pair<int, int> pii; // index of example, class value
    vector<pii> ricv;
    for(int in=0; ri; ++ri) {
        TValue const classVal = ri.getClass();
        if (isnan(classVal)) {
            if (stratified == StratifiedIfPossible) {
                return (*this)(gen->size());
            }
            else {
                raiseError(PyExc_ValueError,
                    "cannot prepare stratified indices due to unknown classes");
            }
        }
        ricv.push_back(pii(in++, int(classVal)));
    }
    random_sort(ricv.begin(), ricv.end(),
          predOn2nd<pair<int, int>, less<int> >(),
          predOn2nd<pair<int, int>, equal_to<int> >(),
          rsrgen(randomGenerator, randseed));

    PIntList indices(new TIntList);
    indices->resize(ricv.size());
    int gr=0;
    ITERATE(vector<pii>, ai, ricv) {
        indices->at(ai->first) = gr++;
        gr = gr % folds;
    }
    return indices;
};
