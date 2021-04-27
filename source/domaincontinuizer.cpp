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
#include "domaincontinuizer.px"


TDiscrete2Continuous::TDiscrete2Continuous(
    int const aval, bool const inv, bool const zeroB)
: value(aval),
  invert(inv),
  zeroBased(zeroB)
{}


void TDiscrete2Continuous::transform(TValue &val) const
{ 
    if (!isnan(val)) {
        int const ival = TDiscreteVariable::toInt(val);
        if ((ival == value) != invert) {
            val = 1;
        }
        else {
            val = zeroBased ? 0.0 : -1.0;
        }
    }
}


TOrdinal2Continuous::TOrdinal2Continuous(double const f)
: factor(f)
{}


void TOrdinal2Continuous::transform(TValue &val) const
{
    if (!isnan(val)) {
        int const ival = TDiscreteVariable::toInt(val);
        val = ival * factor;
    }
}


TNormalizeContinuous::TNormalizeContinuous(double const av, double const sp)
: average(av),
span(fabs(sp) > 1e-6 ? sp : 1.0)
{}


void TNormalizeContinuous::transform(TValue &val) const
{ 
    if (!isnan(val)) {
        val = (val-average)/span;
    }
}


TDomainContinuizer::TDomainContinuizer()
: zeroBased(true),
  continuousTreatment(Leave),
  multinomialTreatment(FrequentIsBase),
  classTreatment(ReportError)
{}


PVariable TDomainContinuizer::discrete2continuous(
    PDiscreteVariable const &var, int const val, bool const inv) const
{
    PVariable contvar(new TContinuousVariable(
        var->getName() + "=" + var->values->at(val)));
    PClassifierFromVar cfv(new TClassifierFromVar(contvar, var));
    cfv->transformer = PTransformValue(
        new TDiscrete2Continuous(val, inv, zeroBased));
    contvar->getValueFrom = cfv;
    return contvar;
}


PVariable TDomainContinuizer::ordinal2continuous(
    PDiscreteVariable const &var, double const factor) const
{
    PVariable contvar(new TContinuousVariable("C_"+var->getName()));
    PClassifierFromVar cfv(new TClassifierFromVar(contvar, var));
    POrdinal2Continuous transf(new TOrdinal2Continuous(1.0/var->values->size()));
    cfv->transformer = transf;
    transf->factor = factor;
    contvar->getValueFrom = cfv;
    return contvar;
}


void TDomainContinuizer::discrete2continuous(
    PDiscreteVariable const &var, TVarList &vars, int const mostFrequent) const
{ 
    if (var->values->size() < 2) {
        return;
    }
    int baseValue;
    switch (multinomialTreatment) {
        case Ignore:
            if (var->values->size() == 2) {
                vars.push_back(discrete2continuous(var, 1));
            }
            break;
        case IgnoreAllDiscrete:
            break;
        case ReportError:
            if (var->values->size() > 2) {
                raiseError(PyExc_ValueError, 
                    "variable '%s' is multinomial", var->cname());
            }
            vars.push_back(discrete2continuous(var, 1));
            break;
        case AsOrdinal:
            vars.push_back(ordinal2continuous(var, 1));
            break;
        case AsNormalizedOrdinal:
            vars.push_back(ordinal2continuous(var, 1.0/(var->values->size()-1.0)));
            break;
        default:
            if (var->baseValue >= 0) {
                baseValue = var->baseValue;
            }
            else if (multinomialTreatment == FrequentIsBase) {
                baseValue = mostFrequent;
            }
            else {
                baseValue = 0;
            }
            if (var->values->size() == 2) {
                vars.push_back(discrete2continuous(var, 1-baseValue));
            }
            else {
                for(int val = 0, mval = var->values->size(); val<mval; val++) {
                    if ((multinomialTreatment==NValues) || (val!=baseValue)) {
                        vars.push_back(discrete2continuous(var, val));
                    }
                }
            }
    }
}


PVariable TDomainContinuizer::continuous2normalized(
    PContinuousVariable const &var, double const avg, double const span) const
{ 
    PVariable contvar(new TContinuousVariable("N_"+var->getName()));
    PClassifierFromVar cfv(new TClassifierFromVar(contvar, var));
    cfv->transformer = PNormalizeContinuous(new TNormalizeContinuous(avg, span));
    contvar->getValueFrom = cfv;
    return contvar;
}


PVariable TDomainContinuizer::discreteClass2continous(
    PDiscreteVariable const &classVar, int const targetClass) const
{
    const int classBase = targetClass >= 0 ? targetClass : classVar->baseValue;
    if (classBase >= 0) {
        if (classBase >= classVar->values->size()) {
            raiseError(PyExc_IndexError,
                "base class value %i out of range", classBase);
        }
        PVariable newClassVar(new TContinuousVariable(
            classVar->getName() + "=" + classVar->values->at(classBase)));
        PClassifierFromVar cfv(new TClassifierFromVar(newClassVar, classVar));
        cfv->transformer = PDiscrete2Continuous(
            new TDiscrete2Continuous(classBase, false, zeroBased));
        newClassVar->getValueFrom = cfv;
        return newClassVar;
    }
    if ((classTreatment == Ignore) || (classVar->values->size() < 2)) {
        return classVar;
    }
    if (classVar->values->size() == 2) {
        return discrete2continuous(classVar, 1);
    }
    if (classTreatment == AsOrdinal) {
        return ordinal2continuous(classVar, 1.0);
    }
    if (classTreatment == AsNormalizedOrdinal) {
        return ordinal2continuous(classVar, 1.0/(classVar->values->size()-1));
    }
    raiseError(PyExc_ValueError, "class '%s' is multinomial", classVar->cname());
    return PVariable();
}


PDomain TDomainContinuizer::operator()(
    PDomain const &dom, int const targetClass) const
{ 
    if ((continuousTreatment != Leave) && dom->hasContinuousAttributes(false)) {
        raiseError(PyExc_ValueError,
            "cannot compute normalization of continuous attributes without data");
    }
    if ((multinomialTreatment == FrequentIsBase) && dom->hasDiscreteAttributes(false)) {
        raiseError(PyExc_ValueError, 
            "cannot determine the most frequent values of discrete attributes without data");
    }
    PVariable newClassVar;
    if (dom->classVar) {
        if (((targetClass>=0) || (classTreatment != Ignore)) &&
            (dom->classVar->varType == TVariable::Discrete) && 
            (dom->classVar->noOfValues() >= 1)) {
            newClassVar = discreteClass2continous(
                PDiscreteVariable(dom->classVar), targetClass);
        }
        else {
            newClassVar = dom->classVar;
        }
    }
    PVarList newvars(new TVarList);
    PITERATE(TVarList, vi, dom->attributes) {
        if ((*vi)->varType == TVariable::Discrete) {
            discrete2continuous(PDiscreteVariable(*vi), *newvars, -1);
        }
        else {
            newvars->push_back(*vi);
        }
    }
    return PDomain(new TDomain(newClassVar, newvars));
}


PDomain TDomainContinuizer::operator()(
    PExampleTable const &egen, int const targetClass) const
{ 
    bool convertClass = ((targetClass>=0) || (classTreatment != Ignore)) &&
        egen->domain->classVar;
    if (!continuousTreatment && (multinomialTreatment != FrequentIsBase)) {
        return (*this)(egen->domain, targetClass);
    }
    const TDomain &domain = *egen->domain;
    vector<double> avgs, spans;
    vector<int> mostFrequent;
    bool hasMostFrequent = (multinomialTreatment == FrequentIsBase) &&
        domain.hasDiscreteAttributes(convertClass);
    if (hasMostFrequent) {
        TDomainDistributions ddist(egen, false, true);
        ITERATE(TDomainDistributions, ddi, ddist) {
            if (*ddi) {
                // won't call modus here, I want the lowest values if there are more values with equal frequencies
                int val = 0, highVal = 0;
                double highestF = 0.0;
                PDiscDistribution ddva(*ddi);
                for(TDiscDistribution::const_iterator di(ddva->begin()), de(ddva->end());
                    di!=de; di++, val++) {
                        if (*di > highestF) {
                            highestF = *di;
                            highVal = val;
                        }
                }
                mostFrequent.push_back(highVal);
            }
            else {
                mostFrequent.push_back(-1);
            }
        }
    }
    if (continuousTreatment && domain.hasContinuousAttributes(convertClass)) {
        TDomainBasicAttrStat dombas(egen);
        ITERATE(TDomainBasicAttrStat, di, dombas) {
            if (*di) {
                if (continuousTreatment == NormalizeBySpan) {
                    const double &min = (*di)->min;
                    const double &max = (*di)->max;
                    if (zeroBased) {
                        avgs.push_back(min);
                        spans.push_back(max-min);
                    }
                    else {
                        avgs.push_back((max+min) / 2.0);
                        spans.push_back((max-min) / 2.0);
                    }
                }
                else {
                    avgs.push_back((*di)->avg);
                    spans.push_back((*di)->dev);
                }
            }
            else {
                avgs.push_back(-1);
                spans.push_back(-1);
            }
        }
    }
    PVariable newClassVar;
    if (convertClass && (domain.classVar->varType == TVariable::Discrete)) {
        newClassVar = discreteClass2continous(
            PDiscreteVariable(domain.classVar), targetClass);
    }
    else {
        newClassVar = domain.classVar;
    }
    PVarList newvars(new TVarList);
    TVarList::const_iterator vi(domain.attributes->begin()),
        ve(domain.attributes->end());
    for(int i = 0; vi!=ve; vi++, i++) {
        if ((*vi)->varType == TVariable::Discrete) {
            discrete2continuous(PDiscreteVariable(*vi), *newvars,
                hasMostFrequent ? mostFrequent[i] : 0);
        }
        else if (continuousTreatment) {
            newvars->push_back(continuous2normalized(
                PContinuousVariable(*vi), avgs[i], spans[i]));
        }
        else {
            newvars->push_back(*vi);
        }
    }
    PDomain newDomain(new TDomain(newClassVar, newvars));
    newDomain->metas = egen->domain->metas;
    return newDomain;
}


char *DomainContinuizer_call_keywords2[] = {"domain", "targetClass", NULL};

PyObject *TDomainContinuizer::__call__(PyObject *args, PyObject *kw) const
{
    PDomain domain;
    int targetClass = -1;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&|i",
        DomainContinuizer_call_keywords2,
        &PDomain::argconverter, &domain, &targetClass)) {
            return (*this)(domain, targetClass).getPyObject();
    }
    PyErr_Clear();

    PExampleTable data;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&|i",
        DomainContinuizer_call_keywords,
        &PExampleTable::argconverter, &data, &targetClass)) {
            return (*this)(data, targetClass).getPyObject();
    }
    return NULL; // error message set by the last PyArg_ParseTupleAndKeywords
}