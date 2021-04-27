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
#include "knn_projected.px"


TP2NN::TP2NN(
    PDomain const &domain, PExampleTable const &egen,
    PFloatList const &basesX, PFloatList const &basesY,
    const int alaw, const bool normalize)
: TClassifier(domain),
  offsets(new TFloatList()),
  normalizers(new TFloatList()),
  averages(new TFloatList()),
  normalizeExamples(normalize),
  bases(new double[2 * domain->attributes->size()]),
  radii(new double[domain->attributes->size()]),
  nExamples(egen->size()),
  projections(new double[3 * egen->size()]),
  law(alaw),
  minClass(0),
  maxClass(0)
{ 
    const int nAttrs = domain->attributes->size();
    if ((basesX->size() != nAttrs) || (basesY->size() != nAttrs)) {
        raiseError(PyExc_ValueError,
            "the number of attributes and of x- and y-anchors coordinates mismatch");
    }
    double *bi, *radiii;
    TFloatList::const_iterator bxi(basesX->begin()), bxe(basesX->end());
    TFloatList::const_iterator byi(basesY->begin());
    for(radiii = radii, bi = bases;
        bxi != bxe;
        *radiii++ = sqrt(sqr(*bxi) + sqr(*byi)), *bi++ = *bxi++, *bi++ = *byi++);
    const TDomain &gendomain = *egen->domain;
    vector<int> attrIdx;
    attrIdx.reserve(nAttrs);
    TDomainDistributions ddist(egen, false, true); // skip continuous
    if (domain->hasContinuousAttributes()) {
        TDomainBasicAttrStat basstat(egen);
        const_PITERATE(TVarList, ai, domain->attributes) {
            const int aidx = gendomain.getVarNum(*ai);
            attrIdx.push_back(aidx);
            if ((*ai)->varType == TVariable::Discrete) {
                offsets->push_back(0);
                normalizers->push_back((*ai)->noOfValues() - 1);
                averages->push_back(ddist[aidx]->highestProbValue());
            }
            else if ((*ai)->varType == TVariable::Continuous) { // can be meta in data
                if (aidx < 0) {
                    raiseError(PyExc_TypeError,
                        "P2NN does not support continuous meta attributes");
                }
                offsets->push_back(basstat[aidx]->min);
                normalizers->push_back(basstat[aidx]->max - basstat[aidx]->min);
                averages->push_back(basstat[aidx]->avg);
            }
            else {
                raiseError(PyExc_TypeError,
                    "P2NN can only handle discrete and continuous attributes");
            }
        }
    }
    else {
        const_PITERATE(TVarList, ai, domain->attributes) {
            if ((*ai)->varType != TVariable::Discrete) {
                raiseError(PyExc_TypeError,
                    "P2NN can only handle discrete and continuous attributes");
            }
            const int aidx = gendomain.getVarNum(*ai);
            attrIdx.push_back(aidx);
            offsets->push_back(0.0);
            normalizers->push_back((*ai)->noOfValues()-1);
            averages->push_back(ddist[aidx]->highestProbValue());
        }
    }
    const int &classIdx = gendomain.getVarNum(domain->classVar);
    const bool contClass = domain->classVar->varType == TVariable::Continuous;
    fill(projections, projections + 3*nExamples, 0);
    double *pi = projections;
    TFloatList::const_iterator const offb(offsets->begin());
    TFloatList::const_iterator const norb(normalizers->begin());
    TFloatList::const_iterator const avgb(averages->begin());
    vector<int>::const_iterator const ab(attrIdx.begin());
    vector<int>::const_iterator const ae(attrIdx.end());
    TFloatList::const_iterator offi, nori, avgi;
    vector<int>::const_iterator ai;
    TExampleTable::iterator ei(egen->begin());
    if (ei) {
        minClass = maxClass = ei.getClass();
    }
    for(; ei; ++ei) {
        TValue cval = ei.getClass();
        if (isnan(cval)) {
            continue;
        }
        double *base = bases;
        double sumex = 0.0;
        for(radiii = radii, offi = offb, nori = norb, avgi = avgb, ai = ab;
            ai!=ae;
            offi++, nori++, avgi++, ai++) {
            TValue val = ei.value_at(*ai);
            if (isnan(val)) {
                val = *avgi;
            }
            val = (val - *offi) / *nori;
            pi[0] += val * *base++;
            pi[1] += val * *base++;
            sumex += val * *radiii++;
        }
        if (normalizeExamples && (sumex > 0.0)) {
            pi[0] /= sumex;
            pi[1] /= sumex;
        }
        pi[2] = cval;
        pi += 3;
        if (contClass) {
            if (cval < minClass) {
                minClass = cval;
            }
            else if (cval > maxClass) {
                maxClass = cval;
            }
        }
    }
}


TP2NN::TP2NN(PDomain const &dom, double *aprojections, const int nEx,
    double *ba,
    PFloatList const &off, PFloatList const &norm, PFloatList const &avgs,
    const int alaw, const bool normalize)
: TClassifier(dom),
  offsets(new TFloatList(off)),
  normalizers(new TFloatList(norm)),
  averages(new TFloatList(avgs)),
  normalizeExamples(normalize),
  bases(new double[2 * domain->attributes->size()]),
  radii(new double[domain->attributes->size()]),
  nExamples(nEx),
  projections(new double[3*nEx]),
  law(alaw),
  minClass(0),
  maxClass(0)
{
    if (ba) {
        memcpy(bases, ba, 2*domain->attributes->size());
        for(double *radiii = radii,
                   *radiie = radii + domain->attributes->size(),
                   *bi = bases;
               radiii != radiie;
               *radiii++ = sqrt(sqr(*bi++) + sqr(*bi++)));
    }
    else {
        fill(ba, ba + 2*domain->attributes->size(), 0);
    }
    memcpy(projections, aprojections, 3*nEx);
    if (nEx && (dom->classVar->varType == TVariable::Continuous)) {
        double *proj = projections+2, *proje = proj + 3*nEx;
        minClass = maxClass = *proj;
        while( (proj+=3) != proje ) {
            if (*proj < minClass) {
                minClass = *proj;
            }
            else if (*proj > maxClass) {
                maxClass = *proj;
            }
        }
    }
}


TP2NN::TP2NN(PDomain const &dom, const int nAtt, const int nEx)
    : TClassifier(dom),
      bases(new double[2*nAtt]),
      radii(new double[2*nAtt]),
      nExamples(nEx),
      projections(new double[3*nExamples])
{}


TP2NN::TP2NN(const TP2NN &old)
: TClassifier(old),
  bases(NULL),
  radii(NULL),
  nExamples(0),
  projections(NULL)
{
    *this = old;
}


TP2NN &TP2NN::operator =(const TP2NN &old)
{
    nExamples = old.nExamples;
    law = old.law;
    normalizeExamples = old.normalizeExamples;
    minClass = old.minClass;
    maxClass = old.maxClass;
    const int nAttrs = domain->attributes->size();
    delete bases;
    if (old.bases) {
        bases = new double[2*nAttrs];
        copy(old.bases, old.bases+2*nAttrs, bases);
    }
    else {
        bases = NULL;
    }
    delete radii;
    if (old.radii) {
        radii = new double[nAttrs];
        copy(old.radii, old.radii+nAttrs, radii);
    }
    else {
        radii = NULL;
    }
    delete projections;
    if (old.projections) {
        projections = new double[3*nExamples];
        copy(old.projections, old.projections+3*nExamples, projections);
    }
    else {
        projections = NULL;
    }
    offsets = PFloatList(old.offsets ? new TFloatList(*old.offsets) : NULL);
    normalizers = PFloatList(old.normalizers ? new TFloatList(old.normalizers) : NULL);
    return *this;
}


TP2NN::~TP2NN()
{
    delete bases;
    delete projections;
    delete radii;
}



void TP2NN::project(TExample const *const example, double &x, double &y)
{
    x = y = 0.0;
    double sumex = 0.0;
    double *base = bases, *radius = radii;
    TFloatList::const_iterator offi(offsets->begin());
    TFloatList::const_iterator nori(normalizers->begin());
    TFloatList::const_iterator avgi(averages->begin());
    TExample::const_iterator ei(example->begin());
    TExample::const_iterator const ee(example->end());
    for(; ei != ee; ei++, avgi++, offi++, nori++) {
        double av = isnan(*ei) ? *avgi : *ei;
        av = (av - *offi) / *nori;
        x += av * *(base++);
        y += av * *(base++);
        if (normalizeExamples) {
            sumex += av * *radius++;
        }
    }
    if (normalizeExamples && (sumex > 1e-6)) {
        x /= sumex;
        y /= sumex;
    }
}


TValue TP2NN::operator ()(TExample const *const example)
{
    checkProperty(offsets);
    checkProperty(normalizers);
    checkProperty(averages);
    checkProperty(bases);
    if (normalizeExamples) {
        checkProperty(radii);
    }
    if (classVar->varType == TVariable::Continuous) {
        return TClassifier::operator()(example);
    }
    double x, y;
    getProjectionForClassification(example, x, y);
    return averageClass(x, y);
}



PDistribution TP2NN::classDistribution(TExample const *const example)
{
    checkProperty(offsets);
    checkProperty(normalizers);
    checkProperty(averages);
    checkProperty(bases);
    if (normalizeExamples) {
        checkProperty(radii);
    }
    double x, y;
    getProjectionForClassification(example, x, y);
    if (classVar->varType == TVariable::Continuous) {
        PContDistribution cont(new TContDistribution(classVar));
        cont->add(averageClass(x, y));
        return cont;
    }
    else {
        const int nClasses = domain->classVar->noOfValues();
        double *cprob = new double [nClasses];
        try {
            classDistribution(x, y, cprob, nClasses);
            PDiscDistribution wdist(new TDiscDistribution(cprob, nClasses));
            wdist->normalize();
            return wdist;
        }
        catch (...) {
            delete cprob;
            throw;
        }
    }
    return PDistribution();
}


void TP2NN::classDistribution(
    double const x, double const y, double *distribution, int const nClasses) const
{
    fill(distribution, distribution + nClasses, 0);
    double *proj = projections, *proje = projections + 3*nExamples;
    switch(law) {
    case InverseLinear:
    case Linear:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            distribution[int(proj[2])] += dist<1e-8 ? 1e4 : 1.0/sqrt(dist);
        }
        return;

    case InverseSquare:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            distribution[int(proj[2])] += dist<1e-8 ? 1e8 : 1.0/dist;
        }
        return;

    case InverseExponential:
    case KNN:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            distribution[int(proj[2])] += exp(-sqrt(dist));
        }
        return;
    }
}


double TP2NN::averageClass(double const x, double const y) const
{
    double sum = 0.0;
    double N = 0.0;
    double *proj = projections, *proje = projections + 3*nExamples;
    switch(law) {
    case InverseLinear:
    case Linear:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            const double w = dist<1e-8 ? 1e4 : 1.0/sqrt(dist);
            sum += w * proj[2]; 
            N += w;
        }
        break;

    case InverseSquare:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            const double w = dist<1e-8 ? 1e4 : 1.0/dist;
            sum += w * proj[2]; 
            N += w;
        }
        break;

    case InverseExponential:
    case KNN:
        for(; proj != proje; proj += 3) {
            const double dist = sqr(proj[0] - x) + sqr(proj[1] - y);
            const double w = dist<1e-8 ? 1e4 : exp(-sqrt(dist));
            sum += w * proj[2]; 
            N += w;
        }
        break;
    }
    return N > 1e-7 ? sum/N : 0.0;
}




TOrange *TP2NN::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && !kw && (PyTuple_Size(args) == 7)) {
        return unpickle(type, args);
    }

    PDomain domain;
    PExampleTable examples;
    PyObject *pybases;
    int normalizeExamples = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&O|iO&:P2NN", P2NN_keywords,
        &PExampleTable::argconverter, &examples, &pybases,
        &normalizeExamples, &PDomain::argconverter_n, &domain)) {
            return NULL;
    }
    if (!domain) {
        domain = examples->domain;
    }
    PyObject *iter = PyObject_GetIter(pybases);
    if (!iter) {
        PyErr_Format(PyExc_TypeError,
            "invalid 'bases' (%s)", pybases->ob_type->tp_name);
        return NULL;
    }
    GUARD(iter);\
    int natts = domain->attributes->size();
    PFloatList basesX(new TFloatList(natts));
    PFloatList basesY(new TFloatList(natts));
    TFloatList::iterator bxi(basesX->begin());
    TFloatList::iterator byi(basesY->begin());
    PyObject *item;
    for(item = PyIter_Next(iter); item && natts--; item = PyIter_Next(iter)) {
        PyObject *it2 = item;
        GUARD(it2);
        PyObject *iter2 = PyObject_GetIter(item);
        if (!iter2) {
            PyErr_Format(PyExc_TypeError,
                "invalid 'bases' (a two-dimensional structure expected");
            return NULL;
        }
        PyObject *item0 = PyIter_Next(iter);
        if (!item0) {
            PyErr_Format(PyExc_TypeError,
                "invalid 'bases'; two-dimensional projections expected");
            return NULL;
        }
        GUARD(item0);
        PyObject *item1 = PyIter_Next(iter);
        if (!item1) {
            PyErr_Format(PyExc_TypeError,
                "invalid 'bases'; two-dimensional projections expected");
            return NULL;
        }
        GUARD(item1);
        PyObject *item_ex = PyIter_Next(iter);
        if (item_ex) {
            PyErr_Format(PyExc_TypeError,
                "invalid 'bases'; two-dimensional projections expected");
            return NULL;
        }
        *bxi++ = PyNumber_AsDouble(item0);
        *byi++ = PyNumber_AsDouble(item1);
    }
    if (natts) { // >0 loop ended due to !item, natts-- was evaluated when natts was already -1
        Py_XDECREF(item);
        PyErr_Format(PyExc_ValueError,
            "the number of bases mismatches the number of attributes");
    }
    return new(type) TP2NN(
        domain, examples, basesX, basesY, -1.0, normalizeExamples != 0);
}


TOrange *TP2NN::unpickle(PyTypeObject *type, PyObject *args)
{
    PDomain dom;
    double *projections;
    Py_ssize_t projsize;
    int nExamples;
    double *bases;
    Py_ssize_t basessize;
    PFloatList offsets, normalizers, averages;
    if (!PyArg_ParseTuple(args, "O&y#iy#O&O&O&",
        &PDomain::argconverter,
        &projections, &projsize,
        &nExamples,
        &bases, &basessize,
        PFloatList::argconverter_n, &offsets,
        PFloatList::argconverter_n, &normalizers,
        PFloatList::argconverter_n, &averages)) {
            return NULL;
    }
    return new(type) TP2NN(dom, projections, nExamples, bases,
        offsets, normalizers, averages);
}


PyObject *TP2NN::__getnewargs__() const
{
    checkProperty(projections);
    checkProperty(bases);
    return Py_BuildValue("Oy#iy#OOO", 
        domain.borrowPyObject(),
        projections, 3*nExamples*sizeof(double),
        nExamples,
        bases, 2*sizeof(double)*domain->attributes->size(),
        offsets.borrowPyObject(),
        normalizers.borrowPyObject(),
        averages.borrowPyObject());
}

