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
#include "contdistribution.px"

TContDistribution::TContDistribution() 
: sum(0.0),
  sum2(0.0)
{
  supportsContinuous = true;
}


TContDistribution::TContDistribution(PVariable const &var) 
: TDistribution(var),
  sum(0.0),
  sum2(0.0)
{ 
    supportsContinuous = true;
    if (var->varType != TVariable::Continuous) {
        raiseError(PyExc_SystemError,
            "variable '%s' is not continuous", var->cname());
    }
}


TContDistribution::TContDistribution(map<double, double> const &f) 
: distribution(f),
  sum(0.0),
  sum2(0.0)
{ 
    supportsContinuous = true;
    abs = 0.0;
    const_this_ITERATE(fi) {
        abs += fi->second;
        sum += fi->first * fi->second;
        sum2 += fi->second * sqr(fi->first);
    }
    cases = abs;
}


TContDistribution::TContDistribution(TContDistribution const &other) 
: TDistribution(other),
  distribution(other.distribution),
  sum(other.sum),
  sum2(other.sum2)
{}


TContDistribution &TContDistribution::operator =(TContDistribution const &other) 
{
    TDistribution::operator=(other);
    distribution = other.distribution;
    sum = other.sum;
    sum2 = other.sum2;
    return *this;
}    


double const &TContDistribution::at(TValue const v)
{ 
    iterator vi = lower_bound(v);
    if ((vi == end()) || (vi->first != v)) {
        vi = distribution.insert(vi, value_type(v, 0.0));
    }
    return vi->second; 
}


double const &TContDistribution::at(TValue const v) const
{ 
    const_iterator vi = find(v);
    if (vi == end()) {
        raiseError(PyExc_KeyError, "value %5.3f not found", v);
    }
    return vi->second; 
}


void TContDistribution::add(TValue const v, double const w)
{ 
    if (isnan(v)) {
        unknowns += w;
        cases += w;
    }
    else {
        double &val = const_cast<double &>(at(v)); // I now what I'm doing ;)
        val += w;
        abs += w;
        cases += w;
        sum += w*v;
        sum2 += w*sqr(v);
        normalized = false;
    }
}


void TContDistribution::set(TValue const v, double const w)
{ 
    if (isnan(v)) {
        raiseError(PyExc_ValueError,
            "cannot set the probability of unknown value");
    }
    double &val = const_cast<double &>(at(v));
    const double diff = w-val;
    val = w;
    abs += diff;
    cases += diff;
    sum += diff * v;
    sum2 += diff * sqr(v);
    normalized = false;
}


void TContDistribution::normalize()
{ 
    if (!normalized) {
        if (abs) {
            this_ITERATE(dvi) {
                dvi->second /= abs;
            }
            sum /= abs;
            sum2 /= abs;
            abs = 1.0;
        }
        else {
            if (size()) {
                sum  = sum2 = 0;
                const double p = 1.0 / double(size());
                this_ITERATE(dvi) {
                    dvi->second = p;
                    sum += dvi->first;
                    sum2 += sqr(dvi->first);
                }
                abs = 1.0;
                sum *= p;
                sum2 *= p;
            }
        }
        normalized = true;
    }
}


unsigned int TContDistribution::checkSum() const
{ 
    unsigned int crc;
    INIT_CRC(crc);
    const_this_ITERATE(dvi) {
        add_CRC(dvi->first, crc);
        add_CRC(dvi->second, crc);
    }
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


double TContDistribution::p(double const x) const
{ 
    const_iterator rb = lower_bound(x);
    if (rb == end()) {
        return 0.0;
    }
    if ((*rb).first == x) {
        return (*rb).second;
    }
    if (rb == begin()) {
        return 0.0;
    }
    const_iterator lb = rb;
    lb--;
    return lb->second + 
        (x-lb->first) * (rb->second-lb->second)/(rb->first-lb->first);
}


double TContDistribution::highestProb() const
{
    double maxe = 0;
    const_this_ITERATE(dvi) {
        if (dvi->second > maxe) {
            maxe = dvi->second;
        }
    }
    return maxe;
}



TValue TContDistribution::highestProbValue(long const random) const
{
    if (!size()) {
        return 0.0;
    }
    int wins = 1;
    double best = 0;
    double bestP = distribution.begin()->second-1;
    const_this_ITERATE(dvi) {
        if (dvi->second > bestP) {
            best = dvi->first;
            bestP = dvi->second;
            wins = 1;
        }
        else if (dvi->second == bestP) {
            wins++;
        }
    }
    if (wins==1) {
        return TValue(best);
    }
    long which = random ? random : long(checkSum() % wins);
    const_this_ITERATE(wi) {
        if ((wi->second == bestP) && !which--) {
            return TValue(wi->first);
        }
    }
    return 0.0; // cannot come here, but the compiler does not know that
}


TValue TContDistribution::highestProbValue(TExample const *exam) const
{
    if (!size()) {
        return 0.0;
    }
    int wins = 1;
    double best = 0;
    double bestP = distribution.begin()->second-1;
    const_this_ITERATE(dvi) {
        if (dvi->second > bestP) {
            best = dvi->first;
            bestP = dvi->second;
            wins = 1;
        }
        else if (dvi->second == bestP) {
            wins++;
        }
    }
    if (wins==1) {
        return TValue(best);
    }
    long which = exam->checkSum() % wins;
    const_this_ITERATE(wi) {
        if ((wi->second == bestP) && !which--) {
            return TValue(wi->first);
        }
    }
    return 0.0; // cannot come here, but the compiler does not know that
}


TValue TContDistribution::randomValue(const long random) const
{ 
    if (!size() || !abs) {
        raiseError(PyExc_ValueError,
            "cannot return random values from empty distribution");
    }
    double ri;
    if (random) {
        ri = fmod(random & 0x7fffffff, abs);
    }
    else {
        if (!randomGenerator) {
            randomGenerator = PRandomGenerator(new TRandomGenerator());
        }
        ri = randomGenerator->randfloat(abs);
    }
    const_iterator di(begin());
    while (ri > di->second) {
        ri -= di->second;
        di++;
    }
    return TValue(di->first);
}


bool TContDistribution::operator ==(TDistribution const &other) const
{
    TContDistribution const *cont =
        dynamic_cast<TContDistribution const *>(&other);
    if (!cont) {
        raiseError(PyExc_TypeError,
            "cannot compare distributions of different types");
    }
    return distribution == cont->distribution;
}


TDistribution &TContDistribution::operator +=(TDistribution const &other)
{ 
    const TContDistribution *mother =
        dynamic_cast<const TContDistribution *>(&other);
    if (!mother) {
        raiseError(PyExc_TypeError,
            "wrong type of distribution for addition");
    }
    const_PITERATE(TContDistribution, oi, mother)  {
        add((*oi).first, (*oi).second);
    }
    unknowns += mother->unknowns;
    return *this;
}


TDistribution &TContDistribution::operator -=(TDistribution const &other)
{ 
    const TContDistribution *mother =
        dynamic_cast<const TContDistribution *>(&other);
    if (!mother) {
        raiseError(PyExc_TypeError,
            "wrong type of distribution for addition");
    }
    const_PITERATE(TContDistribution, oi, mother)  {
        add((*oi).first, -(*oi).second);
    }
    unknowns -= mother->unknowns;
    return *this;
}


TDistribution &TContDistribution::operator *=(double const weight)
{
    this_ITERATE(dvi) {
        dvi->second *= weight;
    }
    abs *= weight;
    sum *= weight;
    sum2 *= weight;
    normalized = false;
    return *this;
}


double TContDistribution::average() const
{
    if (abs < 1) {
        raiseError(PyExc_ValueError,
            "cannot compute average of an empty distribution");
    }
    return sum/abs ; 
}


double TContDistribution::dev() const
{ 
    if (abs < 2) {
        raiseError(PyExc_ValueError,
            "cannot compute the standard deviation on a sample with less than two instances");
    }
    const double var1 = (sum2-sqr(sum)/abs)/abs;
    return var1 > 0 ? sqrt(var1) : 0.0;
}
  

double TContDistribution::var() const
{ 
    if (abs < 2) {
        raiseError(PyExc_ValueError,
            "cannot compute the variance on a sample with less than two instances");
    }
    const double var1 = (sum2-sqr(sum)/abs)/abs;
    return var1 > 0 ? var1 : 0.0;
}
  
double TContDistribution::error() const
{
    if (abs < 2) {
        raiseError(PyExc_ValueError,
            "cannot compute the standard error from a sample with less than two instances");
    }
    const double var1 = (sum2-sqr(sum)/abs);
    return var1 > 0 ? sqrt(var1/(abs-1) / abs) : 0.0;
}


double TContDistribution::percentile(const double perc) const
{ 
    if ((perc<0) || (perc>100)) {
        raiseError(PyExc_ValueError,
            "invalid percentile (should be between 0 and 100)");
    }
    if (!size() || (abs < 1e-6)) {
        raiseError(PyExc_ValueError,
            "cannot compute percentiles from an empty sample");
    }
    if (perc==0.0) {
        return begin()->first;
    }
    if (perc==100.0) {
        const_iterator i = end();
        return (--i)->first;
    }
    double togo = abs*perc/100.0;
    const_iterator ths(begin()), prev, ee(end());
    while ((ths != ee) && (togo > 0)) {
        togo -= ths->second;
        prev = ths;
        ths++;
    }
    if ((togo < 0) || (ths == ee)) {
        return prev->first;
    }
    else {
        return (prev->first+ths->first) / 2.0;
    }
}


TOrange *TContDistribution::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
Represents a distribution of a continuous variable.

Constructor expects either a distribution given as a list of tuples
(value, frequency) or data from which to compute the distribution. The
latter form requires a variable and, optionally, data. If the variable is
the only argument, it must be an
instance of `Orange.data.variable.Variable`. In that case, an empty
distribution is constructed. If data is given as well, the variable can
also be specified by name or index in the domain. Constructor then computes
the distribution of the specified variable on the given data.
*/
{
    if (!args || !PyTuple_GET_SIZE(args)) {
        return new(type) TContDistribution();
    }
    if ((PyTuple_GET_SIZE(args) == 1) &&
        PyBytes_Check(PyTuple_GET_ITEM(args, 0))) {
            return unpickle(type, args);
    }
    if (PyTuple_GET_SIZE(args) == 1) {
        PyObject *arg = PyTuple_GET_ITEM(args, 0);
        if (OrContDistribution_Check(arg)) {
            return new(type) TContDistribution(*PContDistribution(arg));
        }
        map<double, double> dd;
        if (convertFromPython(arg, dd)) {
            return new(type) TContDistribution(dd);
        }
    }
    return TDistribution::__new__(type, args, kw);
}


PyObject *TContDistribution::__richcmp__(PyObject *other, int op) const
{
    if ((op != Py_EQ) && (op != Py_NE)) {
        raiseError(PyExc_TypeError, "cannot compare two distributions");
    }

    if (OrContDistribution_Check(other)) {
        PContDistribution dist(other);
        return PyBool_FromBool((*this == *dist) == (op == Py_EQ));
    }

    map<double, double> dd;
    if (convertFromPython(other, dd)) {
        return PyBool_FromBool((distribution == dd) == (op == Py_EQ));
    }

    raiseError(PyExc_TypeError,
        "cannot compare continuous distribution with '%s'",
        other->ob_type->tp_name);
    return NULL;
}


long TContDistribution::__hash__() const
{
    return checkSum();
}


PyObject *TContDistribution::__str__() const
{
    string s;
    char buf[128];
    const_this_ITERATE(di) {
        if (s.size()) {
            s += ", ";
        }
        if (variable) {
            snprintf(buf, 128, "%s: %.3f", 
                variable->val2str(di->first).c_str(), di->second);
        }
        else {
            snprintf(buf, 128, "%.3f: %.3f", di->first, di->second);
        }
        s += buf;
    }
    return PyUnicode_FromString(("<"+s+">").c_str());
}


PyObject *TContDistribution::__repr__() const
{
    return __str__();
}


PyObject *TContDistribution::keys() const
{
    PyObject *nl = PyList_New(size());
    Py_ssize_t i = 0;
    const_this_ITERATE(ci) {
        PyList_SetItem(nl, i++, PyFloat_FromDouble(ci->first));
    }
    return nl;
}


PyObject *TContDistribution::values() const
{
    PyObject *nl = PyList_New(size());
    Py_ssize_t i = 0;
    const_this_ITERATE(ci) {
        PyList_SetItem(nl, i++, PyFloat_FromDouble(ci->second));
    }
    return nl;
}


PyObject *TContDistribution::items() const
{
    PyObject *nl = PyList_New(size());
    Py_ssize_t i = 0;
    const_this_ITERATE(ci) {
        PyList_SetItem(nl, i++, Py_BuildValue("ff", ci->first, ci->second));
    }
    return nl;
}


PyObject *TContDistribution::native() const
{
    PyObject *nd = PyDict_New();
    const_this_ITERATE(ci) {
        PyObject *key = PyFloat_FromDouble((double)((*ci).first));
        PyObject *val = PyFloat_FromDouble((double)((*ci).second));
        PyDict_SetItem(nd, key, val);
        Py_DECREF(key);
        Py_DECREF(val);
    }
    return nd;
}

PyObject *TContDistribution::__getnewargs__() const
{
    TByteStream buf(2*size()*sizeof(double) + sizeof(size_t));
    buf.write(distribution.size());
    const_this_ITERATE(ci) {
        buf.write((*ci).first);
        buf.write((*ci).second);
    }
    return Py_BuildValue("(y#)", buf.buf, buf.length());
}

TOrange *TContDistribution::unpickle(PyTypeObject *cls, PyObject *args)
{
    TByteStream buf;
    if (!PyArg_ParseTuple(args, "O&:ContDistribution.unpickle",
        &TByteStream::argconverter, &buf)) {
            return NULL;
    }
    TContDistribution *cdi = new(cls) TContDistribution();
    // if you change this, carefully follow what happens with the reference:
    // unpickle must return a TOrange * to a reference it owns!
    for(int size = buf.readSizeT(); size--; ) {
      // cannot call buf.readFloat() in the make_pair call since we're not sure about the
      // order in which the arguments are evaluated
      const double p1 = buf.readDouble();
      const double p2 = buf.readDouble();
      cdi->insert(cdi->end(), make_pair(p1, p2));
    }
    return cdi;
}

PyObject *TContDistribution::py_average() const
{
    return PyFloat_FromDouble(average());
}

PyObject *TContDistribution::py_var() const
{
    return PyFloat_FromDouble(var());
}

PyObject *TContDistribution::py_dev() const
{
    return PyFloat_FromDouble(dev());
}

PyObject *TContDistribution::py_error() const
{
    return PyFloat_FromDouble(error());
}

PyObject *TContDistribution::py_percentile(PyObject *arg) const
{
    const double p = PyNumber_AsDouble(arg,
        "percentile(p) expects a number, not '%s'");
    return PyFloat_FromDouble(percentile(p));
}

PyObject *TContDistribution::py_density(PyObject *arg) const
{
    const double val = PyNumber_AsDouble(arg,
        "density(p) expects a number, not '%s'");
    return PyFloat_FromDouble(p(val));
}
