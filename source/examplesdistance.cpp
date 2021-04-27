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
#include "domaindistributions.hpp"
#include "domainbasicattrstat.hpp"
#include "examplesdistance.px"


TExamplesDistanceConstructor::TExamplesDistanceConstructor(bool const ic)
: ignoreClass(ic)
{}


PyObject *TExamplesDistanceConstructor::__call__(PyObject *args, PyObject *kw) const
{
    PExampleTable data;
    int weightID = 0;
    PDomainDistributions ddist;
    PDomainBasicAttrStat basstat;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "|O&O&O&:ExamplesDistanceConstructor",
        ExamplesDistanceConstructor_call_keywords,
        &PExampleTable::argconverter_n, &data,
        &PDomainDistributions::argconverter_n, &ddist,
        &PDomainBasicAttrStat::argconverter_n, &basstat)) {
            return NULL;
    }
    return (*this)(data, ddist, basstat).getPyObject();
}


PyObject *TExamplesDistance::__call__(PyObject *args) const
{
    PExample ex1, ex2;
    if (!PyArg_ParseTuple(args, "O&O&:ExamplesDistance",
        &PExample::argconverter, &ex1, &PExample::argconverter, &ex2)) {
        return NULL;
    }
    return PyFloat_FromDouble((*this)(ex1.borrowPtr(), ex2.borrowPtr()));
}


TExamplesDistanceConstructor_Hamming::TExamplesDistanceConstructor_Hamming(
    bool const ic, bool const ui)
: ignoreClass(ic), 
  ignoreUnknowns(ui)
{}

PExamplesDistance TExamplesDistanceConstructor_Hamming::operator()(
    PExampleTable const &,
    PDomainDistributions const &, PDomainBasicAttrStat const &) const
{ 
    return PExamplesDistance(
        new TExamplesDistance_Hamming(ignoreClass, ignoreUnknowns)); 
}


TExamplesDistance_Hamming::TExamplesDistance_Hamming(bool const ic, bool const iu)
: ignoreClass(ic),
  ignoreUnknowns(iu)
{}


double TExamplesDistance_Hamming::operator()(TExample const *const e1,
                                             TExample const *const e2) const 
{ 
    PDomain const &d1 = e1->domain, &d2 = e2->domain;
    if (   (d1 != d2)
        && (ignoreClass ? d1->attributes != d2->attributes 
                        : d1->variables != d2->variables)) {
        raiseError(PyExc_ValueError, "cannot compare examples from different domains");
    }
    double dist = 0.0;
    int Na = d1->attributes->size() + (!ignoreClass && d1->classVar ? 1 : 0);
    for(TExample::const_iterator i1 = e1->begin(), i2 = e2->begin(); Na--; i1++, i2++) {
        if (   (!ignoreUnknowns && (isnan(*i1)!=isnan(*i2)))
            || (!values_compatible(*i1, *i2))) {
            dist += 1.0;
        }
    }
    return dist;
}


TExamplesDistanceConstructor_Normalized::TExamplesDistanceConstructor_Normalized(
    bool const ic, bool const norm, bool const iu)
: TExamplesDistanceConstructor(ic),
  normalize(norm),
  ignoreUnknowns(iu)
{}


TExamplesDistance_Normalized::TExamplesDistance_Normalized(
    bool const ignoreClass, bool const no, bool const iu,
    PExampleTable const &egen, 
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &bstat)
: normalizers(new TAttributedFloatList()),
  bases(new TAttributedFloatList()),
  averages(new TAttributedFloatList()),
  variances(new TAttributedFloatList()),
  domainVersion(egen ? egen->domain->version : -1),
  normalize(no),
  ignoreUnknowns(iu)
{ 
    // if we have egen and ddist, we go for ddist unless we also have bstat)
    if (egen && (bstat || !ddist)) { 
        PDomainBasicAttrStat bstat2 = bstat ? bstat
            : PDomainBasicAttrStat(new TDomainBasicAttrStat(egen));
        PDomain const &domain = egen->domain;
        attributes = CLONE(PVarList, ignoreClass ? domain->attributes : domain->variables);
        TDomainBasicAttrStat::const_iterator si(bstat2->begin()), ei(bstat2->end());
        TVarList::const_iterator vi (attributes->begin()), evi(attributes->end());
        for(; (vi!=evi) && (si!=ei); si++, vi++) {
            PDiscreteVariable dvar = PDiscreteVariable::cast(*vi);
            if (!dvar && *si && ((*si)->n > 1e-6)) {
                double const min = (*si)->min;
                double const diff = (*si)->max - min;
                bases->push_back(min);
                normalizers->push_back(diff > 1e-6 ? 1/diff : 0.0);
                averages->push_back((*si)->avg);
                variances->push_back((*si)->dev * (*si)->dev);
            }
            else {
                bases->push_back(0.0);
                averages->push_back(0.0);
                variances->push_back(0.0);
                if (!dvar || !dvar->values->size()) {
                    normalizers->push_back(0.0);
                }
                else {
                    normalizers->push_back(dvar->ordered ? 1.0/(*vi)->noOfValues() : -1.0);
                }
            }
        }
    }
    else if (ddist) {
        attributes = PVarList(new TVarList);
        const_PITERATE(TDomainDistributions, ci, ddist) {
            if (*ci) {
                PVariable const &var = (*ci)->variable;
                attributes->push_back(var);
                PContDistribution dcont(*ci);
                PDiscreteVariable dvar(var);
                if (dcont && (dcont->begin() != dcont->end())) {
                    double const min = (*dcont->distribution.begin()).first;
                    double const dif = (*dcont->distribution.rbegin()).first - min;
                    normalizers->push_back(dif > 0.0 ? 1.0/dif : 0.0);
                    bases->push_back(min);
                    averages->push_back(dcont->average());
                    variances->push_back(dcont->var());
                }
                else {
                    bases->push_back(0.0);
                    averages->push_back(0.0);
                    variances->push_back(0.0);
                    if (!dvar || !dvar->values->size()) {
                        normalizers->push_back(0.0);
                    }
                    else {
                        normalizers->push_back(dvar->ordered ?
                            1.0/dvar->values->size() : -1.0);
                    }
                }
            }
        }
    }
    else if (bstat) {
        attributes = PVarList(new TVarList);
        TDomainBasicAttrStat::const_iterator si(bstat->begin()), ei(bstat->end());
        if (ignoreClass && bstat->hasClassVar && (si != ei)) {
            ei--;
        }
        for(; si!=ei; si++) {
            if (!*si) {
                raiseError(PyExc_ValueError,
                    "cannot compute normalizers from BasicAttrStat if domain include discrete attributes");
            }
            attributes->push_back((*si)->variable);
            if (((*si)->n > 1e-6) && ((*si)->max != (*si)->min)) {
                normalizers->push_back(1.0/((*si)->max-(*si)->min));
                bases->push_back((*si)->min);
                averages->push_back((*si)->avg);
                variances->push_back((*si)->dev * (*si)->dev);
            }
            else {
                normalizers->push_back(0.0);
                bases->push_back(0.0);
                averages->push_back(0.0);
                variances->push_back(0.0);
            }
        }
    }
    normalizers->attributes = bases->attributes =
        averages->attributes = variances->attributes =
        attributes;
}

/* Return a vector of normalized differences between the two examples.
   Quick checks do not guarantee that domains are really same to the training domain.
   To be really safe, we should know the domain and convert both examples. Too slow...
*/

void TExamplesDistance_Normalized::getDifs(TExample const *const e1,
                                           TExample const *const e2,
                                           vector<double> &difs) const
{ 
    checkProperty(attributes);

    PDomain const &d1 = e1->domain, &d2 = e2->domain;
    if (!(    (*d1->attributes == *attributes)
        && (*d2->attributes == *attributes)
        ||  (*d1->variables == *attributes)
        && (*d2->variables == *attributes))) {
            raiseError(PyExc_ValueError,
                "examples belong to mismatching domain(s)");
    }
    checkProperty(normalizers);
    difs.resize(normalizers->size());
    vector<double>::iterator di(difs.begin());
    TExample::const_iterator i1(e1->begin()), i2(e2->begin());
    for(TFloatList::const_iterator si(normalizers->begin()), se(normalizers->end());
        si!=se; si++, i1++, i2++, di++) {
            if (isnan(*i1) || isnan(*i2)) {
                *di = ((*si!=0) && !ignoreUnknowns) ? 0.5 : 0.0;
            }
            else  {
                if (*si>0) {
                    *di = fabs(*i1 - *i2) * (normalize ? *si : 1.0);
                }
                else if (*si<0) {
                    *di = (*i1 == *i2) ? 0.0 : 1.0;
                }
            }
    }
}


void TExamplesDistance_Normalized::getNormalized(TExample const *const e1,
                                                 vector<double> &normalized) const
{
    PDomain const &d1 = e1->domain;
    if ((*d1->attributes == *attributes) || (*d1->variables == *attributes)) {
          raiseError(PyExc_ValueError, "example belongs to wrong domain");
    }
    checkProperty(normalizers);
    checkProperty(bases);
    normalized.resize(normalizers->size());
    TExample::const_iterator ei(e1->begin());
    TVarList::const_iterator vi(attributes->begin());
    vector<double>::iterator ni(normalized.begin());
    TAttributedFloatList::const_iterator basi(bases->begin()),
        normi(normalizers->begin()), norme(normalizers->end());
    for(; normi!=norme; ei++, normi++, basi++, vi++, ni++) {
          if (isnan(*ei) || ((*vi)->varType != TVariable::Continuous)) {
              *ni = numeric_limits<double>::signaling_NaN();
          }
          else {
              if ((*normi > 0) && normalize) {
                  *ni = (*ei - *basi) * (*normi);
              }
              else {
                  *ni = *ei;
              }
          }
      }
}


PExamplesDistance TExamplesDistanceConstructor_Maximal::operator()(
    PExampleTable const &egen, 
    PDomainDistributions const &ddist, PDomainBasicAttrStat const &bstat) const
{
    return PExamplesDistance(new TExamplesDistance_Maximal(
        ignoreClass, normalize, ignoreUnknowns,
        egen, ddist, bstat)); 
}


TExamplesDistance_Maximal::TExamplesDistance_Maximal(
    bool const ignoreClass,
    bool const normalize,
    bool const ignoreUnknowns,
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &dstat)
: TExamplesDistance_Normalized(ignoreClass, normalize, ignoreUnknowns,
                               egen, ddist, dstat)
{}


double TExamplesDistance_Maximal::operator ()(TExample const *const e1,
                                              TExample const *const e2) const 
{ 
    vector<double> difs;
    getDifs(e1, e2, difs);
    return difs.size() ? *max_element(difs.begin(), difs.end()) : 0.0;
}


PExamplesDistance TExamplesDistanceConstructor_Manhattan::operator()(
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &bstat) const
{ 
    return PExamplesDistance(new TExamplesDistance_Manhattan(
        ignoreClass, normalize, ignoreUnknowns,
        egen, ddist, bstat)); 
}


TExamplesDistance_Manhattan::TExamplesDistance_Manhattan(
    bool const ignoreClass,
    bool const normalize,
    bool const ignoreUnknowns,
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &dstat)
: TExamplesDistance_Normalized(ignoreClass, normalize, ignoreUnknowns,
                               egen, ddist, dstat)
{}


double TExamplesDistance_Manhattan::operator ()(TExample const *const e1,
                                                TExample const *const e2) const 
{ 
    vector<double> difs;
    getDifs(e1, e2, difs);
    double dist = 0.0;
    const_ITERATE(vector<double>, di, difs) {
        dist += *di;
    }
    return dist;
}


PExamplesDistance TExamplesDistanceConstructor_Euclidean::operator()(
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &bstat) const
{
    return PExamplesDistance(new TExamplesDistance_Euclidean(
        ignoreClass, normalize, ignoreUnknowns,
        egen, ddist, bstat));
}


TExamplesDistance_Euclidean::TExamplesDistance_Euclidean(
    bool const ignoreClass, 
    bool const normalize,
    bool const ignoreUnknowns,
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &dstat)
: TExamplesDistance_Normalized(ignoreClass, normalize, ignoreUnknowns,
                               egen, ddist, dstat)
{
    if (!attributes) { // came here from pickling; everything will be set later
        return;
    }
    bool hasDisc = false;
    PITERATE(TVarList, ai, attributes) {
        if ((*ai)->varType == TVariable::Discrete) {
            hasDisc = true;
            break;
        }
    }
    if (hasDisc) {
        // We must have either ddist or egen, otherwise
        // TExamplesDistance_Normalized would have raised an exception already
        if (ddist) {
            distributions = PDomainDistributions(new TDomainDistributions());
            TDomainDistributions::iterator ddi(distributions->begin());
            PITERATE(TDomainDistributions, di, ddist) {
                distributions->push_back(
                    (*di)->variable->varType == TVariable::Discrete 
                    ? CLONE(PDistribution, *di) : PDistribution());
            }
        }
        else {
            distributions = PDomainDistributions(
                new TDomainDistributions(egen, false, true));
        }
        bothSpecialDist = PAttributedFloatList(
            new TAttributedFloatList(distributions->size(), 0));
        TDomainDistributions::const_iterator di(distributions->begin());
        TAttributedFloatList::iterator bsi(bothSpecialDist->begin()),
            bse(bothSpecialDist->end());
        for(; bsi != bse; di++, bsi++) {
            if (*di) {
                double sum2 = 0;
                PDiscDistribution distr(*di);
                const_PITERATE(TDiscDistribution, pi, distr) {
                    sum2 += (*pi) * (*pi);
                }
                if (distr->abs > 1e-6) {
                    sum2 /= distr->abs * distr->abs;
                    *bsi = 1-sum2;
                }
            }
        }
    }
    else {
        // fill out the vectors with NULLs
        distributions = PDomainDistributions(new TDomainDistributions());
        for(int i = attributes->size(); i--; ) {
            distributions->push_back(PDistribution());
        }
        bothSpecialDist = PAttributedFloatList(
            new TAttributedFloatList(distributions->size(), 0));
    }
}

double TExamplesDistance_Euclidean::operator ()(TExample const *const e1,
                                                TExample const *const e2) const 
{ 
    vector<double> difs;
    getDifs(e1, e2, difs);
    double dist = 0.0;
    TExample::const_iterator e1i(e1->begin()), e2i(e2->begin());
    TVarList::const_iterator ai(attributes->begin()), ae(attributes->end());
    for(TAttrIdx aidx = 0; ai != ae; e1i++, e2i++, ai++, aidx++) {
        if ((*ai)->varType == TVariable::Continuous) {
            if (!isnan(*e1i) && !isnan(*e2i)) {
                dist += sqr(difs[aidx]);
                continue;
            }
            double const var = (*variances)[aidx];
            if (isnan(*e1i) && isnan(*e2i)) {
                dist += 2*var;
                continue;
            }
            double const avg = (*averages)[aidx];
            double const val = isnan(*e1i) ? *e2i : *e1i;
            dist += sqr(val - avg) + 
                normalize ? var*sqr((*normalizers)[aidx]) : var;
        }
        else { // Discrete
            if (!isnan(*e1i) && !isnan(*e2i)) {
                if (*e1i != *e2i) {
                    dist += 1;
                }
            }
            else if (isnan(*e1i) && isnan(*e1i)) {
                dist += (*bothSpecialDist)[aidx];
            }
            else {
                dist += 1 - (*distributions)[aidx]->p(isnan(*e1i) ? *e2i : *e1i);
            }
        }
    }
    return sqrt(dist);
}


TExamplesDistanceConstructor_Lp::TExamplesDistanceConstructor_Lp()
    : p(1.0)
{}


PExamplesDistance TExamplesDistanceConstructor_Lp::operator()(
    PExampleTable const &egen, 
    PDomainDistributions const &ddist, PDomainBasicAttrStat const &bstat) const
{
    return PExamplesDistance(new TExamplesDistance_Lp(
        ignoreClass, normalize, ignoreUnknowns,
        egen, ddist, bstat, p)); 
}


TExamplesDistance_Lp::TExamplesDistance_Lp(double const ap)
    : p(ap)
{}


TExamplesDistance_Lp::TExamplesDistance_Lp(
    bool const ignoreClass,
    bool const normalize,
    bool const ignoreUnknowns,
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &dstat,
    double const ap)
: TExamplesDistance_Normalized(ignoreClass, normalize, ignoreUnknowns,
                               egen, ddist, dstat),
  p(ap)
{}



double TExamplesDistance_Lp::operator ()(TExample const *const e1,
                                         TExample const *const e2) const 
{ 
    if (p <= 0) {
        raiseError(PyExc_ArithmeticError, "invalid power (p=%f)", p);
    }
    vector<double> difs;
    getDifs(e1, e2, difs);
    double dist = 0.0;
    const_ITERATE(vector<double>, di, difs) {
        dist += pow(fabs(*di), p);
    }
    return pow(dist, 1.0 / p);
}



PExamplesDistance TExamplesDistanceConstructor_Relief::operator()(
    PExampleTable const &egen,
    PDomainDistributions const &ddist,
    PDomainBasicAttrStat const &dstat) const
{ 
    return PExamplesDistance(new TExamplesDistance_Relief(
        egen, ddist, dstat)); 
}


TExamplesDistance_Relief::TExamplesDistance_Relief(
    PExampleTable const &gen,
    PDomainDistributions const &addist,
    PDomainBasicAttrStat const &abstat)
{ 
    PDomain const &domain = gen->domain;
    // for continuous attributes BasicAttrStat suffices; for discrete it doesn't
    const bool hasDiscrete = domain->hasDiscreteAttributes()
        || domain->classVar && (domain->classVar->varType == TVariable::Discrete);
    PDomainDistributions ddist = addist;
    PDomainBasicAttrStat bstat = abstat;
    if (!ddist) {
        if (hasDiscrete) {
            if (!gen) {
                raiseError(PyExc_ValueError,
                    "ReliefF needs either examples or domain distributions expected to work with discrete attributes");
            }
            ddist = PDomainDistributions(new TDomainDistributions(gen));
        }
        else {
            if (!bstat) {
                bstat = PDomainBasicAttrStat(new TDomainBasicAttrStat(gen));
            }
        }
    }
    variables = CLONE(PVarList, gen->domain->variables);
    averages = PAttributedFloatList(new TAttributedFloatList(variables, 0));
    normalizations = PAttributedFloatList(new TAttributedFloatList(variables, 0));
    bothSpecial = PAttributedFloatList(new TAttributedFloatList(variables, 0.5));
    distributions = ddist ? CLONE(PDomainDistributions, ddist) : PDomainDistributions();
    if (distributions) {
        distributions->normalize();
    }
    TVarList::const_iterator ai(variables->begin()), ae(variables->end());
    TAttributedFloatList::iterator avgi = averages->begin();
    TAttributedFloatList::iterator nori = normalizations->begin();
    TAttributedFloatList::iterator bsi = bothSpecial->begin();
    for(TAttrIdx idx = 0; ai != ae; ai++, idx++, avgi++, nori++, bsi++) {
        if ((*ai)->varType == TVariable::Continuous) {
            if (bstat) {
                PBasicAttrStat const &bas = bstat->at(idx);
                *avgi = bas->avg;
                *nori = bas->max - bas->min;
            }
            else {
                PContDistribution contd(ddist->at(idx));
                if (!contd) {
                    raiseError(PyExc_TypeError,
                        "continuous distribution expected for variable '%s'",
                        (*ai)->cname());
                }
                if (contd->size()) {
                    *avgi = contd->average();
                    *nori = (*contd->distribution.rbegin()).first -
                        (*contd->distribution.begin()).first;
                }
            }
        }
        else {
            PDiscDistribution discd(ddist->at(idx));
            if (!discd) {
                raiseError(PyExc_TypeError,
                    "discrete distribution expected for variable '%s'",
                    (*ai)->cname());
            }
            *bsi = 1.0;
            const_PITERATE(TDiscDistribution, di, discd) {
                *bsi -= *di * *di;
            }
        }
    }
}


double TExamplesDistance_Relief::operator()(TExample const *const e1,
                                            TExample const *const e2) const
{ 
    checkProperty(averages);
    checkProperty(normalizations);
    checkProperty(bothSpecial);
    const bool hasDistributions = bool(distributions);
    double dist = 0.0;
    TVarList::const_iterator ai(variables->begin());
    TExample::const_iterator e1i(e1->begin());
    TExample::const_iterator e2i(e2->begin());
    TExample::const_iterator const e1e(e1->end());
    for(TAttrIdx aidx = 0; e1i != e1e; ai++, e1i++, e2i++, aidx++) {
        double dd = 0.0;
        if ((*ai)->varType == TVariable::Discrete) {
            if (!isnan(*e1i) && !isnan(*e2i)) {
                if (*e1i != *e2i) {
                    dd = 1.0;
                }
                else if (isnan(*e1i) && isnan(*e2i)) {
                    dd = (*bothSpecial)[aidx];
                }
                else {
                    if (!hasDistributions) {
                        raiseError(PyExc_ValueError,
                            "cannot handle unknown values ('%s') since "
                            "distributions are not given", (*ai)->cname());
                    }
                    dd = 1-(*distributions)[aidx]->at(isnan(*e1i) ? *e2i : *e1i);
                }
            }
        }
        else {
            double const norm = (*normalizations)[aidx];
            if (norm > 1e-6) {
                if (!isnan(*e1i) && !isnan(*e2i)) {
                    dd = fabs(*e1i - *e2i) / norm;
                }
                else if (isnan(*e1i) && isnan(*e2i)) {
                    dd = 0.5;
                }
                else {
                    dd = fabs((*averages)[aidx] - (isnan(*e1i) ? *e2i : *e1i)) / norm;
                }
            }
        }
        dist += dd > 1.0 ? 1.0 : dd;
    }
    return dist;
}


double TExamplesDistance_Relief::operator()(TAttrIdx const attrNo,
                                            TValue const v1, 
                                            TValue const v2) const
{
    double dd = -1;
    PVariable var = variables->at(attrNo);
    if (var->varType == TVariable::Discrete) {
        if (!isnan(v1) && !isnan(v2)) {
            dd = (v1 != v2) ? 1.0 : 0.0;
        }
        else if (isnan(v1) && isnan(v2)) {
            dd = bothSpecial->at(attrNo);
        }
        else {
            if (!distributions) {
                raiseError(PyExc_ValueError,
                    "cannot handle unknown values ('%s') since distributions "
                    "are not given",
                    var->cname());
            }
            dd = 1 - distributions->at(attrNo)->at(isnan(v1) ? v2 : v1);
        }
    }
    else if (normalizations->at(attrNo) > 0) {
        if (!isnan(v1) && !isnan(v2)) {
            dd = fabs(v1 - v2) / normalizations->at(attrNo);
        }
        else if (isnan(v1) && isnan(v2)) {
            dd = 0.5;
        }
        else {
            dd = fabs(averages->at(attrNo) - isnan(v1) ? v2 : v1)
                / normalizations->at(attrNo);
        }
    }
    return dd > 1.0 ? 1.0 : dd;
}
