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
#include "discdistribution.px"

TDiscDistribution::TDiscDistribution() 
: nValues(0),
  distribution(distribution_)
{}


TDiscDistribution::TDiscDistribution(PVariable const &var) 
: TDistribution(var)
{ 
    if (var->varType != TVariable::Discrete) {
        raiseError(PyExc_SystemError,
            "variable '%s' is not discrete", var->cname());
    }
    nValues = var->noOfValues();
    distribution = nValues <= DD_STORAGE_SIZE ? distribution_ : new double[nValues];
    fill_n(distribution, nValues, 0);
}


TDiscDistribution::TDiscDistribution(ssize_t const values, double const value) 
: nValues(values),
  distribution(nValues <= DD_STORAGE_SIZE ? distribution_ : new double[nValues])
{ 
    fill_n(distribution, nValues, value);
    cases = abs = value*values;
}


TDiscDistribution::TDiscDistribution(vector<double> const &f) 
: nValues(f.size()),
  distribution(f.size() <= DD_STORAGE_SIZE ? distribution_ : new double[f.size()])
{ 
    abs = 0.0;
    double *dp = distribution;
    const_ITERATE(vector<double>, fi, f) {
        abs += (*dp++ = *fi);
    }
    cases = abs;
}


TDiscDistribution::TDiscDistribution(double const *f, ssize_t len)
: nValues(len),
  distribution(len <= DD_STORAGE_SIZE ? distribution_ : new double[len])
{ 
    abs = 0.0;
    for(double *dp = distribution; len--; abs += (*dp++ = *f++));
    cases = abs;
}


TDiscDistribution::TDiscDistribution(TDiscDistribution const &other) 
: TDistribution(other),
  nValues(other.nValues),
  distribution(other.nValues <= DD_STORAGE_SIZE ? distribution_ : new double[other.nValues]),
  variances(other.variances ? new TFloatList(*other.variances) : NULL)
{
    copy(other.distribution, other.distribution+nValues, distribution);
}


TDiscDistribution &TDiscDistribution::operator =(TDiscDistribution const &other) 
{
    TDistribution::operator=(other);
    resize(other.nValues);
    copy(other.distribution, other.distribution+nValues, distribution);
    variances =
        PFloatList(other.variances ? new TFloatList(*other.variances) : NULL);
    return *this;
}    


TDiscDistribution::~TDiscDistribution()
{
    if (distribution != distribution_) {
        delete distribution;
    }
}


void TDiscDistribution::resize(size_t const newSize)
{
    if (newSize <= nValues) {
        // never returns memory (too rare and inconsequential, although implementation would be cheap)
        nValues = newSize;
        return;
    }
    if (newSize <= DD_STORAGE_SIZE) {
        for(double *dp = distribution_+nValues, *de = distribution_+newSize;
            dp != de; *dp++ = 0);
    }
    else {
        double *newPtr = new double[newSize];
        double *dp = newPtr, *de = newPtr + newSize;
        this_ITERATE(ni) {
            *dp++ = *ni;
        }
        for(; dp != de; *dp++ = 0);
        distribution = newPtr;
    }
    nValues = newSize;
}


double const &TDiscDistribution::at(TValue const v)
{ 
    int const i = int(v);
    if (i < 0) {
        raiseError(PyExc_IndexError, "value %i out of range", i);
    }
    if (i >= size()) {
        resize(i+1);
    }
    return distribution[i]; 
}


double const &TDiscDistribution::at(TValue const v) const
{ 
    int const i = int(v);
    if ((i < 0) || (i >= int(size()))) {
        raiseError(PyExc_IndexError, "value %i out of range", i);
    }
    return distribution[i]; 
}


void TDiscDistribution::add(TValue const v, double const w)
{ 
    if (isnan(v)) {
        unknowns += w;
        cases += w;
    }
    else {
        double &val = const_cast<double &>(at(v)); // I know what I'm doing ;)
        val += w;
        abs += w;
        cases += w;
        normalized = false;
    }
}


void TDiscDistribution::set(TValue const v, double const w)
{ 
    if (isnan(v)) {
        raiseError(PyExc_ValueError,
            "cannot set the probability of unknown value");
    }
    double &val = const_cast<double &>(at(v));
    abs += w-val;
    cases += w-val;
    val = w;
    normalized = false;
}


void TDiscDistribution::normalize()
{ 
    if (!normalized) {
        if (abs) {
            this_ITERATE(dvi) {
                *dvi /= abs;
            }
            abs = 1.0;
        }
        else {
            if (size()) {
                fill_n(distribution, nValues, 1.0/double(size()));
                abs = 1.0;
            }
        }
        normalized = true;
    }
}


unsigned int TDiscDistribution::checkSum() const
{ 
    unsigned int crc;
    INIT_CRC(crc);
    const_this_ITERATE(dvi) {
        add_CRC(*dvi, crc);
    }
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


double TDiscDistribution::p(TValue const x) const
{
    if (!abs) {
        return size() ? 1.0/size() : 0.0;
    }
    const int i = int(x);
    return i < size() ? distribution[i]/abs : 0.0;
}


double TDiscDistribution::highestProb() const
{
    return size() ? *max_element(begin(), end()) : 0.0;
}



TValue TDiscDistribution::highestProbValue(long const random) const
{
    if (!size()) {
        return 0.0;
    }

    int wins = 1;
    int best = 0;
    double bestP = *distribution;
    const_this_ITERATE(pi) {
        if (*pi > bestP) {
            best = pi - begin();
            bestP = *pi;
            wins = 1;
        }
        else if (*pi == bestP) {
            wins++;
        }
    }
    if (wins==1) {
        return TValue(best);
    }
    long which = random ? random : long(checkSum() % wins);
    const_this_ITERATE(wi) {
        if ((*wi == bestP) && !which--) {
            return TValue(wi - begin());
        }
    }
    return 0.0; // cannot come here, but the compiler does not know that
}


TValue TDiscDistribution::highestProbValue(TExample const *const exam) const
{
    if (!size()) {
        return 0.0;
    }
    int wins = 1;
    int best = 0;
    double bestP = *distribution;
    const_this_ITERATE(pi) {
        if (*pi > bestP) {
            best = pi - begin();
            bestP = *pi;
            wins = 1;
        }
        else if (*pi == bestP) {
            wins++;
        }
    }
    if (wins==1) {
        return TValue(best);
    }
    long which = exam->checkSum() % wins;
    const_this_ITERATE(wi) {
        if ((*wi == bestP) && !which--) {
            return TValue(wi - begin());
        }
    }
    return 0.0; // cannot come here, but the compiler does not know that
}


TValue TDiscDistribution::randomValue(const long random) const
{ 
    if (!size())
        return 0;
    double ri;
    if (random) {
        if (!abs) {
            return TValue((random >> 3) % size());
        }
        ri = double(random & 0x7fffffff) / 0x7fffffff * abs;
    }
    else {
        if (!randomGenerator) {
            randomGenerator = PRandomGenerator(new TRandomGenerator());
        }
        if (!abs) {
            return TValue(randomGenerator->randint(size()));
        }
        ri = randomGenerator->randfloat(abs);
    }
    const_iterator di(begin());
    while (ri > *di) {
        ri -= *di++;
    }
    return TValue(di - begin());
}


TDistribution &TDiscDistribution::adddist(TDistribution const &other, double const factor)
{
    const TDiscDistribution *mother =
        dynamic_cast<const TDiscDistribution *>(&other);
    if (!mother) {
        raiseError(PyExc_TypeError,
            "wrong type of distribution addition/subtraction");
    }
    if (mother->size() > size()) {
        resize(mother->size());
    }
    iterator ti = begin();
    const_PITERATE(TDiscDistribution, oi, mother) {
        *ti++ += *oi * factor;
    }
    abs += mother->abs * factor;
    cases += mother->cases * factor;
    unknowns += mother->unknowns * factor;
    normalized = false;
    return *this;
}


bool TDiscDistribution::operator ==(TDistribution const &other) const
{
    TDiscDistribution const *disc =
        dynamic_cast<TDiscDistribution const *>(&other);
    if (!disc) {
        raiseError(PyExc_TypeError,
            "cannot compare distributions of different types");
    }
    return distribution == disc->distribution;
}


TDistribution &TDiscDistribution::operator +=(TDistribution const &other)
{ 
    return adddist(other, 1.0);
}


TDistribution &TDiscDistribution::operator -=(TDistribution const &other)
{ 
    return adddist(other, -1.0);
}


TDistribution &TDiscDistribution::operator *=(double const weight)
{
    this_ITERATE(pi) {
        *pi *= weight;
    }
    abs *= weight;
    normalized = false;
    return *this;
}


TDistribution &TDiscDistribution::operator *=(TDistribution const &other)
{
    const TDiscDistribution *mother =
        dynamic_cast<const TDiscDistribution *>(&other);
    if (!mother) {
        raiseError(PyExc_TypeError,
            "wrong type of distribution for multiplication");
    }
    abs = 0.0;
    iterator ti = begin(), te = end();
    const_iterator oi = mother->begin(), oe = mother->end();
    for(; (ti!=te) && (oi!=oe); ti++, oi++) {
        *ti *= *oi;
        abs += *ti;
    }
    if (ti != te) {
        resize(ti - begin());
    }
    normalized = false;
    return *this;
}


TOrange *TDiscDistribution::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
Represents a distribution of a discrete variable.

Constructor expects either a distribution given as a list of frequencies or
data from which to compute the distribution. The latter form requires a
variable and, optionally, data. If the
variable is the only argument, it must be an instance of
`Orange.data.variable.Variable`. In that case, an empty distribution is
constructed. If data is given as well, the variable can also be specified
by name or index in the domain. Constructor then computes the distribution
of the specified variable on the given data.
*/
{
    if (!args || !PyTuple_GET_SIZE(args)) {
        return new(type) TDiscDistribution();
    }
    if ((PyTuple_GET_SIZE(args) == 1) &&
        PyBytes_Check(PyTuple_GET_ITEM(args, 0))) {
            return unpickle(type, args);
    }
    if (PyTuple_GET_SIZE(args) == 1) {
        PyObject *arg = PyTuple_GET_ITEM(args, 0);
        vector<double> dd;
        if (convertFromPython(arg, dd)) {
            return new(type) TDiscDistribution(dd);
        }
    }
    return TDistribution::__new__(type, args, kw);
}


PyObject *TDiscDistribution::__getnewargs__() const
{
    TByteStream buf(size()*sizeof(double) + sizeof(size_t));
    buf.write(nValues);
    const_this_ITERATE(di) {
        buf.write(*di);
    }
    return Py_BuildValue("(y#)", buf.buf, buf.length());
}


TOrange *TDiscDistribution::unpickle(PyTypeObject *cls, PyObject *args)
{
    TByteStream buf;
    if (!PyArg_ParseTuple(args, "O&:DiscDistribution.unpickle",
        &TByteStream::argconverter, &buf)) {
        return NULL;
    }
    int nValues;
    buf.read(nValues);
    TDiscDistribution *cdi = new(cls) TDiscDistribution(nValues);
    ITERATE(TDiscDistribution, di, *cdi) {
        buf.read(*di);
    }
    return cdi;
}

PyObject *TDiscDistribution::__richcmp__(PyObject *other, int op) const
{
    if ((op != Py_EQ) && (op != Py_NE)) {
        raiseError(PyExc_TypeError, "cannot compare two distributions");
    }

    if (OrDiscDistribution_Check(other)) {
        PDiscDistribution dist(other);
        return PyBool_FromBool(
            ((size() == dist->size()) && equal(begin(), end(), dist->begin()))
            == (op == Py_EQ)
        );
    }
    vector<double> dd;
    if (convertFromPython(other, dd)) {
        return PyBool_FromBool(
            ((size() == dd.size()) && equal(begin(), end(), dd.begin()))
            == (op == Py_EQ)
        );
    }
    raiseError(PyExc_TypeError,
        "cannot compare discrete distribution with '%s'",
        other->ob_type->tp_name);
    return NULL;
}


long TDiscDistribution::__hash__() const
{
    return checkSum();
}


PyObject *TDiscDistribution::__str__() const
{
    string s;
    char buf[128];
    const_this_ITERATE(di) {
        if (s.size()) {
            s += ", ";
        }
        snprintf(buf, 128, "%.3f", *di);
        s += buf;
    }
    return PyUnicode_FromString(("<"+s+">").c_str());
}


PyObject *TDiscDistribution::__repr__() const
{
    return __str__();
}


PyObject *TDiscDistribution::__item__(Py_ssize_t index) const
{
    if (index >= size()) {
        raiseError(PyExc_IndexError, "index %i out of range", index);
    }
    return PyFloat_FromDouble(at(TValue(index)));
}


TDiscreteVariable *TDiscDistribution::getDiscVar() const
{
    if (!variable) {
        raiseError(PyExc_TypeError,
            "invalid distribution (no variable)");
    }
    TDiscreteVariable *discvar =
        dynamic_cast<TDiscreteVariable *>(variable.borrowPtr());
    if (!discvar) {
        raiseError(PyExc_TypeError,
            "invalid distribution: variable is not discrete");
    }
    return discvar;
}

PyObject *TDiscDistribution::keys() const
{
    TDiscreteVariable *var = getDiscVar();
    PyObject *nl = PyList_New(var->noOfValues());
    Py_ssize_t i = 0;
    const_PITERATE(TStringList, ii, var->values) {
        PyList_SetItem(nl, i++, PyUnicode_FromString(ii->c_str()));
    }
    return nl;
}


PyObject *TDiscDistribution::items() const
{ 
    TDiscreteVariable *var = getDiscVar();
    PyObject *nl = PyList_New(var->noOfValues());
    TDiscDistribution::const_iterator ci(begin());
    Py_ssize_t i = 0;
    const_PITERATE(TStringList, ii, var->values) {
        PyList_SetItem(nl, i++, Py_BuildValue("sf", ii->c_str(), *(ci++)));
    }
    return nl;
}


PyObject *TDiscDistribution::values() const
{ 
    PyObject *nl = PyList_New(size());
    Py_ssize_t i = 0;
    const_this_ITERATE(ci) {
        PyList_SetItem(nl, i++, PyFloat_FromDouble(*ci));
    }
    return nl;
}


PyObject *TDiscDistribution::native() const
{
    return values();
}
