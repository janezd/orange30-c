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
#include "scorefeature.hpp"
#include "discretization.px"


PyObject *TDiscretization::__call__(PyObject *args, PyObject *kw) const
{
    PyObject *pyvar;
    PExampleTable table;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO&:Discretization",
        Discretization_call_keywords, &pyvar,
        &PExampleTable::argconverter, &table)) {
            return NULL;
    }
    PVariable var = table->domain->getVar(pyvar);
    PContinuousVariable cvar(PContinuousVariable::cast(var));
    if (!cvar) {
        return PyErr_Format(PyExc_TypeError,
            "Discretization expects continuous variable; '%s' is discrete",
            var->cname());
    }
    return (*this)(cvar, table).getPyObject();
}


PyObject *TDiscretizer::construct_variable(PyObject *args, PyObject *kw)
{
    PContinuousVariable var;
    TValue minDiff = 1.0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|d:construct_var",
        Discretizer_construct_variable_keywords,
        &PContinuousVariable::argconverter, &var, &minDiff)) {
            return NULL;
    }
    return constructVar(PContinuousVariable(var), minDiff).getPyObject();
}


TEqualWidthDiscretizer::TEqualWidthDiscretizer(
    int const noi, TValue const fv, TValue const st)
: nIntervals(noi),
  firstCut(fv),
  step(st)
{}


// Transform the value;
// result is 1+((val-firstCut)/step) cropped into [0, nIntervals)
void TEqualWidthDiscretizer::transform(TValue &val) const
{ 
    if (isnan(val)) {
        return;
    }
    if (step<0) {
        raiseError(PyExc_ValueError, "'step' is not set");
    }
    if (nIntervals<1) {
        raiseError(PyExc_ValueError,
            "invalid number of intervals (%i)", nIntervals);
    }
    if ((step==0) || (nIntervals==1)) {
        val = 0;
    }
    else {
        val = val<firstCut ? 0 : 1 + floor((val-firstCut)/step);
        if (val >= nIntervals) {
            val = nIntervals-1;
        }
    }
}


inline int numDecs(const TValue &mindiff, TValue &factor)
{ 
    if (mindiff >= 1.0) {
        factor = 100.0;
        return 2;
    }
    int decs = (int)ceil(-log10(mindiff));
    if (decs < 2) {
       decs = 2;
    }
    factor = exp(decs*log(10.0));
    return decs;
}

inline TValue roundFromDecs(const int &decs)
{ 
    return decs <= 0 ? 100.0 : exp(decs*log(10.0));
}

inline void roundToFactor(TValue &f, const TValue &factor)
{
    f = floor(f*factor+0.5)/factor;
}

string mcvt(TValue f, int decs)
{ 
  char buf[64];
  sprintf(buf, "%.*f", decs, f);
  return buf;
}

/*  Constructs a new TDiscreteVariable. Its values represent the intervals
    for values of passed variable var; getValueFrom points to a classifier
    which gets a value of the original variable (var) and transforms it using
    'this' transformer. */
PDiscreteVariable TEqualWidthDiscretizer::constructVar(
    PContinuousVariable const &var, TValue const)
{ 
    PDiscreteVariable evar(new TDiscreteVariable("D_"+var->getName()));
    evar->ordered = true;
    if (nIntervals < 2) {
        evar->addValue("C");
    }
    else {
        double roundfactor;
        int decs = numDecs(step, roundfactor);
        if ((var->adjustDecimals != 2) && (decs < var->numberOfDecimals)) {
            decs = var->numberOfDecimals;
            roundfactor = roundFromDecs(var->numberOfDecimals);
        }
        roundToFactor(firstCut, roundfactor);
        roundToFactor(step, roundfactor);
        double f = firstCut;
        string pval(mcvt(f, decs));
        evar->addValue("<" + pval);
        for(int steps = nIntervals-2; steps--; ) {
            string s("[" + pval);
            f += step;
            pval = mcvt(f, decs);
            s += ", " + pval + ")";
            evar->addValue(s);
        }
        evar->addValue(">" + pval);
    }
    PClassifierFromVar tcfv(new TClassifierFromVar(evar, var));
    tcfv->transformUnknowns = true;
    tcfv->transformer = PTransformValue::fromBorrowedPtr(this);
    evar->getValueFrom = tcfv;
    return evar;
}


void TEqualWidthDiscretizer::getCutoffs(vector<TValue> &cutoffs) const
{
  cutoffs.clear();
  for(int i = 0; i < nIntervals-1; i++) {
      cutoffs.push_back(firstCut+step*i);
  }
}


PyObject *TEqualWidthDiscretizer::__get__points(OrEqualWidthDiscretizer *self)
{
    vector<TValue> cutoffs;
    self->orange.getCutoffs(cutoffs);
    PyObject *pts = PyList_New(cutoffs.size());
    int i = 0;
    const_ITERATE(vector<TValue>, ci, cutoffs) {
        PyList_SetItem(pts, i++, PyFloat_FromDouble(*ci));
    }
    return pts;
}


TThresholdDiscretizer::TThresholdDiscretizer(TValue const athreshold)
: threshold(athreshold)
{}


void TThresholdDiscretizer::transform(TValue &val) const
{ 
    if (!isnan(val)) {
        val = val <= threshold ? 0 : 1;
    }
}


PDiscreteVariable TThresholdDiscretizer::constructVar(
    PContinuousVariable const &var, TValue const)
{ 
    PDiscreteVariable evar(new TDiscreteVariable("D_"+var->getName()));
    evar->ordered = true;
    char s[10];
    sprintf(s, "<= %5.3f", threshold);
    evar->values->push_back(s);
    sprintf(s, "> %5.3f", threshold);
    evar->values->push_back(s);
    PClassifierFromVar tcfv(new TClassifierFromVar(evar, var));
    tcfv->transformUnknowns = true;
    tcfv->transformer = PTransformValue::fromBorrowedPtr(this);
    var->getValueFrom = tcfv;
    return evar;
}


void TThresholdDiscretizer::getCutoffs(vector<TValue> &cutoffs) const
{
    cutoffs.assign(1, threshold);
}


PyObject *TThresholdDiscretizer::__get__points(OrThresholdDiscretizer *self)
{
    PyObject *pts = PyList_New(1);
    PyList_SetItem(pts, 1, PyFloat_FromDouble(self->orange.threshold));
    return pts;
}


TIntervalDiscretizer::TIntervalDiscretizer()
: points(PFloatList(new TFloatList()))
{}


TIntervalDiscretizer::TIntervalDiscretizer(PFloatList const &apoints)
: points(PFloatList(new TFloatList(apoints)))
{}


void TIntervalDiscretizer::transform(TValue &val) const
{
    checkProperty(points);
    if (!isnan(val)) {
        TFloatList::iterator ri(points->begin()), re(points->end());
        for(; (ri != re) && (*ri < val); ri++);
        val = ri - points->begin();
    }
}


PDiscreteVariable TIntervalDiscretizer::constructVar(
    PContinuousVariable const &var, TValue amindiff)
{
    TValue mindiff = amindiff;
    PDiscreteVariable evar(new TDiscreteVariable("D_"+var->getName()));
    evar->ordered = true;
    if (!points->size()) {
        evar->addValue("C");
    }
    else {
        TFloatList::iterator vb(points->begin()), ve(points->end()), vi;
        for(vi = vb+1; vi != ve; vi++) {
            TValue const ndiff = *vi - *(vi-1);
            if (ndiff < mindiff) {
                mindiff = ndiff;
            }
        }
        double roundfactor;
        int decs = numDecs(mindiff, roundfactor);
        if ((var->adjustDecimals != 2) && (decs < var->numberOfDecimals)) {
            decs = var->numberOfDecimals;
            roundfactor = roundFromDecs(var->numberOfDecimals);
        }
        vi = points->begin();
        string ostr;
        roundToFactor(*vi, roundfactor);    
        ostr = mcvt(*vi, decs);
        evar->addValue("<=" + ostr);
        while(++vi != ve) {
            string s('(' + ostr);
            roundToFactor(*vi, roundfactor);
            ostr = mcvt(*vi, decs);
            s += ", " + ostr + ']';
            evar->addValue(s);
        }
        evar->addValue('>'+ostr);
    }
    PClassifierFromVar tcfv(new TClassifierFromVar(evar, var));
    tcfv->transformUnknowns = true;
    tcfv->transformer = PTransformValue::fromBorrowedPtr(this);
    evar->getValueFrom = tcfv; 
    return evar;
}


void TIntervalDiscretizer::getCutoffs(vector<TValue> &cutoffs) const
{
  cutoffs.assign(points->begin(), points->end());
}


TEqualWidthDiscretization::TEqualWidthDiscretization(int const anumber)
: TDiscretization(),
  nIntervals(anumber)
{}


PDiscreteVariable TEqualWidthDiscretization::operator()(
    PContinuousVariable const &var, PBasicAttrStat const &valStat) const
{ 
    if (nIntervals < 1) {
        raiseError(PyExc_ValueError,
            "invalid number of intervals (%i)", nIntervals);
    }
    TValue step = (valStat->max - valStat->min) / nIntervals;
    PEqualWidthDiscretizer discretizer(
        new TEqualWidthDiscretizer(nIntervals, valStat->min+step, step));
    return discretizer->constructVar(var);
}


PDiscreteVariable TEqualWidthDiscretization::operator()(
    PContinuousVariable const &var, PExampleTable const &gen) const
{
    if (nIntervals <= 0) {
        raiseError(PyExc_ValueError,
            "invalid number of intervals (%i)", nIntervals);
    }
    int varPos = gen->domain->getVarNum(var);
    TValue max = numeric_limits<TValue>::min();
    TValue min = numeric_limits<TValue>::max();;
    PEITERATE(ei, gen) {
        const TValue val = ei.value_at(varPos);
        if (!isnan(val)) {
            if (val > max) {
                max = val;
            }
            if (val < min) {
                min = val;
            }
        }
    }
    if (max == numeric_limits<TValue>::min()) {
       raiseError(PyExc_ValueError,
           "variable '%s' has only undefined values", var->cname());
    }
    TValue const step = (max - min) / nIntervals;
    PEqualWidthDiscretizer discretizer(
        new TEqualWidthDiscretizer(nIntervals, min + step, step));
    return discretizer->constructVar(var);
}


TEqualFreqDiscretization::TEqualFreqDiscretization(int const anumber)
: nIntervals(anumber)
{}


PDiscreteVariable TEqualFreqDiscretization::operator()(
    PContinuousVariable const &var, PContDistribution const &distr) const
{ 
    PIntervalDiscretizer discretizer(new TIntervalDiscretizer());
    TValue mindiff;
    if (distr->size() <= nIntervals) {
        cutoffsByMidpoints(discretizer, distr, mindiff);
    }
    else {
        cutoffsByCounting(discretizer, distr, mindiff);
    }
    return discretizer->constructVar(var, mindiff);
}


void TEqualFreqDiscretization::cutoffsByMidpoints(
    PIntervalDiscretizer const &discretizer, 
    PContDistribution const &distr,
    TValue &mindiff) const
{
    mindiff = 1.0;
    TContDistribution::const_iterator cdi(distr->begin()), cde(distr->end());
    if (cdi!=cde) {
        TValue prev = cdi->first;
        while (++cdi != cde) {
            discretizer->points->push_back((prev+cdi->first)/2);
            if (cdi->first - prev < mindiff) {
                mindiff = cdi->first - prev;
            }
        }
    }
}


void TEqualFreqDiscretization::cutoffsByCounting(
    PIntervalDiscretizer const &discretizer, 
    PContDistribution const &distr,
    TValue &mindiff) const
{
    if (nIntervals<=0) {
        raiseError(PyExc_ValueError, 
            "invalid number of intervals (%i)", nIntervals);
    }

    mindiff = 1.0;
    double N = distr->abs;
    int toGo = nIntervals;
    double inthis = 0, prevel = -1; // initialized to avoid warnings
    double inone = N/toGo;
    map<double, double>::const_iterator db(distr->begin()), de(distr->end()),
        di(db), ni;
    for(; (toGo > 1) && (di != de); di++) {
        inthis += di->second;
        if ((inthis < inone) || (di == db)) {
            prevel = di->first;
        }
        else {
            ni = di;
            ni++;
            if ((ni != de) && (inthis-inone < di->second/2)) {
                discretizer->points->push_back((ni->first+di->first)/2);
                if (ni->first - di->first < mindiff) {
                    mindiff = ni->first - di->first;
                }
                N -= inthis;
                inthis = 0;
                prevel = ni->first;
            }
            else {
                discretizer->points->push_back( (prevel + di->first) / 2);
                if (di->first - prevel < mindiff) {
                    mindiff = di->first - prevel;
                }
                N -= (inthis - (di->second));
                inthis = di->second;
                prevel = di->first;
            }
            if (--toGo) {
                inone = N/toGo;
            }
        }
    }
}


PDiscreteVariable TEqualFreqDiscretization::operator()(
    PContinuousVariable const &var, PExampleTable const &gen) const
{ 
    PContDistribution distr(TDistribution::fromExamples(gen, var));
    return operator()(var, distr);
}


TEntropyDiscretization::TEntropyDiscretization()
: forceAttribute(false)
{}


PDiscreteVariable TEntropyDiscretization::operator()(
    PContinuousVariable const &var, PExampleTable const &gen) const
{ 
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if (gen->domain->classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError, "class '%s' is not discrete",
            gen->domain->classVar->cname());
    }
    int varPos = gen->domain->getVarNum(var);
    TS S;
    TDiscDistribution all;
    PEITERATE(ei, gen) {
        TValue const val = ei.value_at(varPos);
        if (!isnan(val)) {
            TValue const &eclass = ei.getClass();
            if (!isnan(eclass)) {
                double weight = ei.getWeight();
                S[val].add(eclass, weight);
                all.add(eclass, weight);
            }
        }
    }
    return (*this)(S, all, var);
}


PDiscreteVariable TEntropyDiscretization::operator()(
    TS const &S, TDiscDistribution const &all, PContinuousVariable const &var) const
{
    /* No need to initialize seed by number of examples.
       Different number of examples will obviously result in different decisions. */
    TSimpleRandomGenerator rgen;
    int k = 0;
    const_ITERATE(TDiscDistribution, ci, all) {
        if (*ci>0) {
            k++;
        }
    }
    if (!k) {
        raiseError(PyExc_ValueError,
            "data set contains no examples with known value for attribute '%s'",
            var->cname());
    }
    double mindiff = 1.0;
    vector<pair<double, double> > points;
    double const entropy = getEntropy(all);
    divide(S.begin(), S.end(), all, entropy, k, points, rgen, mindiff);

    PIntervalDiscretizer discretizer(new TIntervalDiscretizer);
    if (points.size()) {
        vector<pair<double, double> >::const_iterator fi(points.begin()),
            fe(points.end());
        discretizer->points->push_back(fi++->first);
        for(; fi!=fe; fi++) {
            if (fi->first != discretizer->points->back()) {
                discretizer->points->push_back(fi->first);
            }
        }
    }
    return discretizer->constructVar(var, mindiff);
}


void TEntropyDiscretization::divide(
  TS::const_iterator const &first, TS::const_iterator const &last,
  TDiscDistribution const &distr,
  double const entropy,
  int const k,
  vector<pair<double, double> > &points,
  TSimpleRandomGenerator &rgen,
  double &mindiff) const
{
    TDiscDistribution S1dist, S2dist = distr, bestS1, bestS2;
    double bestE = -1.0;
    double N = distr.abs;
    int wins = 0;
    TS::const_iterator bestT;
    for(TS::const_iterator Ti = first; Ti!=last; Ti++) {
        S1dist += Ti->second;
        S2dist -= Ti->second;
        if (S2dist.abs==0) {
            break;
        }
        double entro1 = S1dist.abs * double(getEntropy(S1dist))/N;
        double entro2 = S2dist.abs * double(getEntropy(S2dist))/N;
        double E = entro1+entro2;
        if (   (!wins || (E<bestE)) && ((wins=1)==1)
            || (E==bestE) && rgen.randbool(++wins)) {
                bestS1 = S1dist;
                bestS2 = S2dist;
                bestE = E;
                bestT = Ti;
        }
    }
    if (!wins) {
        return;
    }
    int k1 = 0, k2 = 0;
    ITERATE(TDiscDistribution, ci1, bestS1) {
        if (*ci1>0) {
            k1++;
        }
    }
    ITERATE(TDiscDistribution, ci2, bestS2) {
        if (*ci2>0) {
            k2++;
        }
    }
    double entropy1 = getEntropy(bestS1);
    double entropy2 = getEntropy(bestS2);
    double MDL =  log(double(N-1))/log(2.0)/N +
        (log(exp(k*log(3.0))-2)/log(2.0) - (k*entropy-k1*entropy1-k2*entropy2))/N;
    double gain = entropy-bestE;
    double cutoff = bestT->first;
    bestT++;
    if (bestT->first - cutoff < mindiff) {
        mindiff = bestT->first - cutoff;
    }
    if (gain > MDL) {
        if ((k1 > 1) && (first != bestT)) {
            divide(first, bestT, bestS1, entropy1, k1, points, rgen, mindiff);
        }
        points.push_back(pair<double, double>(cutoff, gain-MDL));
        if ((k2 > 1) && (bestT != last)) {
            divide(bestT, last, bestS2, entropy2, k2, points, rgen, mindiff);
        }
    }
    else if (forceAttribute && !points.size()) {
        points.push_back(pair<double, double>(cutoff, gain-MDL));
    }
}


TDomainDiscretization::TDomainDiscretization(PDiscretization const &adisc)
: discretization(adisc)
{}


PDomain TDomainDiscretization::equalWidthDomain(PExampleTable const &gen) const
{
    PDomain newDomain(new TDomain);
    newDomain->metas = gen->domain->metas;
    TDomainBasicAttrStat valStats(gen);
    TVarList::iterator vi=gen->domain->variables->begin();
    TEqualWidthDiscretization const &disc = 
        (TEqualWidthDiscretization &)(*discretization.borrowPtr());
    ITERATE(TDomainBasicAttrStat, si, valStats) {
        if (*si) {
            PVariable evar(disc(PContinuousVariable(*vi), *si));
            newDomain->variables->push_back(evar);
            newDomain->attributes->push_back(evar);
        }
        else {
            newDomain->variables->push_back(*vi);
            newDomain->attributes->push_back(*vi);
        }
        vi++;
    }
    if (gen->domain->classVar) {
        newDomain->classVar = newDomain->variables->back();
        newDomain->attributes->erase(newDomain->attributes->end()-1);
    }
    return newDomain;
}


PDomain TDomainDiscretization::equalFreqDomain(PExampleTable const &gen) const
{
    PDomain newDomain(new TDomain);
    newDomain->metas = gen->domain->metas;
    TDomainDistributions valDs(gen, true, false);
    TVarList::iterator vi=gen->domain->variables->begin();
    TEqualFreqDiscretization const &disc = 
        (TEqualFreqDiscretization &)(*discretization.borrowPtr());
    ITERATE(TDomainDistributions, si, valDs) {
        if (*si) {
            PVariable evar(disc(PContinuousVariable(*vi), PContDistribution(*si)));
            newDomain->variables->push_back(evar);
            newDomain->attributes->push_back(evar);
        }
        else {
            newDomain->variables->push_back(*vi);
            newDomain->attributes->push_back(*vi);
        }
        vi++;
    }
    if (gen->domain->classVar) {
        newDomain->classVar = newDomain->variables->back();
        newDomain->attributes->erase(newDomain->attributes->end()-1);
    }
    return newDomain;
}


PDomain TDomainDiscretization::otherDomain(PExampleTable const &gen) const
{
    PDomain newDomain(new TDomain);
    newDomain->metas = gen->domain->metas;
    PITERATE(TVarList, vi, gen->domain->variables) {
        if ((*vi)->varType == TVariable::Discrete) {
            PVariable evar((*discretization)(PContinuousVariable(*vi), gen));
            newDomain->variables->push_back(evar);
            newDomain->attributes->push_back(evar);
        }
        else {
            newDomain->variables->push_back(*vi);
            newDomain->attributes->push_back(*vi);
        }
    }
    if (gen->domain->classVar) {
        newDomain->classVar=newDomain->variables->back();
        newDomain->attributes->erase(newDomain->attributes->end()-1);
    }
    return newDomain;
}


PDomain TDomainDiscretization::operator()(PExampleTable const &gen) const
{ 
    checkProperty(discretization);
    TDiscretization *disc = discretization.borrowPtr();
    if (dynamic_cast<TEqualWidthDiscretization *>(disc)) {
        return equalWidthDomain(gen);
    }
    if (dynamic_cast<TEqualFreqDiscretization *>(disc)) {
        return equalFreqDomain(gen);
    }
    return otherDomain(gen);
}


TOrange *TDomainDiscretization::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PDiscretization discretization;
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|O&:DomainDiscretization",
        DomainDiscretization_keywords,
        &PDiscretization::argconverter, &discretization,
        &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    TDomainDiscretization *ddisc = new(type) TDomainDiscretization(discretization);
    if (!data) {
        return ddisc;
    }
    PDomainDiscretization wdisc(ddisc);
    return (*ddisc)(data).getPtr();
}


PyObject *TDomainDiscretization::__call__(PyObject *args, PyObject *kw) const
{
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:DomainDiscretization",
        DomainDiscretization_call_keywords, &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    return (*this)(data).getPyObject();
}