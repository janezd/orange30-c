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
#include "imputation.px"

TTransformValue_NanValue::TTransformValue_NanValue(TValue const &value)
    : valueForNan(value)
{}


TOrange *TTransformValue_NanValue::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    TValue val;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&",
        TransformValue_NanValue_keywords, &TPyValue::argconverter, &val)) {
            return NULL;
    }
    return new TTransformValue_NanValue(val);
}


PyObject *TTransformValue_NanValue::__getnewargs__() const
{
    return PyFloat_FromDouble(valueForNan);
}


PExampleTable TImputer::operator()(PExampleTable const &data)
{
    PExampleTable newdata(domain ? TExampleTable::constructConverted(data, domain)
        : TExampleTable::constructCopy(data));
    PEITERATE(ei, newdata) {
        impute(**ei);
    }
    return newdata;
}


TImputer_constant::TImputer_constant(PDomain const &domain)
: values(TExample::constructFree(domain))
{}


TImputer_constant::TImputer_constant(PExample const &aValues)
: values(TExample::constructCopy(values))
{}


TImputer_constant::TImputer_constant(TExample const &aValues)
: values(TExample::constructCopy(values))
{}


TOrange *TImputer_constant::__new__
    (PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (!_PyArg_NoKeywords("Imputer_constant", kw)) {
        return NULL;
    }
    if (!args || (PyTuple_Size(args) != 1)) {
        PyErr_Format(PyExc_TypeError,
            "Imputer_constant takes exactly one argument (%s given)",
            args ? PyTuple_Size(args) : 0);
        return NULL;
    }
    PyObject *arg = PyTuple_GetItem(args, 0);
    if (OrDomain_Check(arg)) {
        return new TImputer_constant(PDomain(arg));
    }
    if (OrExample_Check(arg)) {
        return new TImputer_constant(PExample(arg));
    }
    PyErr_Format(PyExc_TypeError,
        "Imputer_constant expects domain or example, not '%s'",
        arg->ob_type->tp_name);
    return NULL;
}

void TImputer_model::impute(TExample &example)
{
    checkProperty(models);
    if (models->size() != example.domain->variables->size()) {
        raiseError(PyExc_ValueError,
            "the number of models does not match the number of variables");
    }
    TExample::iterator ei(example.begin()), eie(example.end());
    TClassifierList::iterator mi(models->begin()), me(models->end());
    TVarList::const_iterator di(example.domain->variables->begin());
    for(; (ei != eie); ei++, mi++, di++) {
        if (isnan(*ei) && *mi) {
            if ((*mi)->classVar && ((*mi)->classVar != *di)) {
                raiseError(PyExc_ValueError, 
                    "wrong domain (wrong model for '%s')", (*di)->cname());
            }
            *ei = (**mi)(&example);
        }
    }
}



TImputer_random::TImputer_random(bool const ic, bool const dete)
: imputeClass(ic),
  deterministic(dete)
{}


TImputer_random::TImputer_random(
    bool const ic, bool const dete, PDistributionList const &dist)
: imputeClass(ic),
  deterministic(dete),
  distributions(dist)
{}


void TImputer_random::impute(TExample &example)
{
    bool initialized = !deterministic; // if deterministic, randgen is initialized with crc32 for each exapmle
    TVarList::const_iterator vi(example.domain->variables->begin());
    TVarList::const_iterator ve(example.domain->variables->end());
    TExample::iterator ei(example.begin()), ee(example.end());
    if (!imputeClass && example.domain->classVar) {
        --ve;
    }
    if (!distributions) {
        for(; vi != ve; vi++, ei++) {
            if (isnan(*ei)) {
                if (!initialized) {
                    randgen.initseed = example.checkSum();
                    randgen.reset();
                    initialized = true;
                }
                *ei = (*vi)->randomValue(randgen.randlong());
            }
        }
    }
    else {
        TDistributionList::iterator di(distributions->begin());
        for(; vi!=ve; vi++, ei++, di++) {
            if (isnan(*ei)) {
                if (!initialized) {
                    randgen.initseed = example.checkSum();
                    randgen.reset();
                    initialized = true;
                }
                *ei = (*di)->randomValue(randgen.randlong());
            }
        }
    }
}





TImputerConstructor::TImputerConstructor()
: imputeClass(true)
{}


PyObject *TImputerConstructor::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&",
        ImputerConstructor_call_keywords, &PExampleTable::argconverter, &data)) {
            return NULL;
    }
    return (*this)(data).getPyObject();
}


PImputer TImputerConstructor_constant::operator()(PExampleTable const &egen)
{
    return PImputer(new TImputer_constant(defaults));
}


PImputer TImputerConstructor_average::operator()(PExampleTable const &data)
{
    PImputer_constant imputer(new TImputer_constant(data->domain));
    TExample::iterator vi(imputer->values->begin()), ve(imputer->values->end());
    if (!imputeClass && data->domain->classVar) {
        --ve;
    }
    TDomainDistributions ddist(data);
    TDomainDistributions::const_iterator di(ddist.begin());
    TVarList::const_iterator doi(data->domain->variables->begin());
    int const nExamples = data->size();
    for(; vi!=ve; vi++, di++, doi++) {
        *vi = (*di)->supportsContinuous ?
            (*di)->percentile(50) : (*di)->highestProbValue(nExamples);
    }
    return imputer;
}


PImputer TImputerConstructor_minimal::operator()(PExampleTable const &data)
{
  PImputer_constant imputer(new TImputer_constant(data->domain));
  TExample::iterator vi(imputer->values->begin()), ve(imputer->values->end());
  if (!imputeClass && data->domain->classVar) {
      --ve;
  }
  TDomainBasicAttrStat basstat(data);
  TDomainBasicAttrStat::const_iterator bi(basstat.begin());
  for(; vi!=ve; vi++, bi++) {
      *vi = *bi && (*bi)->n ? (*bi)->min : 0;
  }
  return imputer;
}


PImputer TImputerConstructor_maximal::operator()(PExampleTable const &data)
{
  PImputer_constant imputer(new TImputer_constant(data->domain));
  TExample::iterator vi(imputer->values->begin()), ve(imputer->values->end());
  if (!imputeClass && data->domain->classVar) {
      --ve;
  }
  TDomainBasicAttrStat basstat(data);
  TDomainBasicAttrStat::const_iterator bi(basstat.begin());
  for(; vi!=ve; vi++, bi++) {
      *vi = *bi && (*bi)->n ? (*bi)->max : 0;
  }
  return imputer;
}


TImputerConstructor_asValue::TImputerConstructor_asValue()
    : ignoreContinuous(false)
{}

//!!!!!!!!!!!!
//TODO: Are static (generally, non-heap) instances allowed!?
TTransformValue_IsDefined staticTransform_IsDefined;


PVariable TImputerConstructor_asValue::createImputedVar(PVariable const &var)
{
    PTransformValue transformer;
    PDiscreteVariable newvar;

    if (var->varType == TVariable::Discrete) {
        newvar = PDiscreteVariable(new TDiscreteVariable(var->getName()));
        newvar->values = PStringList(
            new TStringList(*PDiscreteVariable(var)->values));
        newvar->values->push_back("NA");
        transformer = PTransformValue(
            new TTransformValue_NanValue(newvar->values->size()-1));
    }
    else {
        newvar = PDiscreteVariable(
            new TDiscreteVariable(var->getName() + "_defined"));
        newvar->values->push_back("def");
        newvar->values->push_back("undef");
        transformer = PTransformValue::fromBorrowedPtr(&staticTransform_IsDefined);
    }
    PClassifierFromVar cfv(new TClassifierFromVar(newvar, var));
    newvar->getValueFrom = cfv;
    cfv->transformUnknowns = true;
    cfv->transformer = transformer;
    return newvar;
}


PImputer TImputerConstructor_asValue::operator ()(PExampleTable const &data)
{
    PDomain &domain = data->domain;
    if (imputeClass &&
        domain->classVar && domain->classVar->varType == TVariable::Continuous) {
            raiseError(PyExc_TypeError,
                "This method can impute only discrete classes");
    }

    bool hasContinuous = false;
    TVarList newVariables;
    PITERATE(TVarList, vi, domain->attributes) {
        PVariable newvar = createImputedVar(*vi);
        if (newvar) {
            newVariables.push_back(newvar);
            if ((*vi)->varType == TVariable::Continuous) {
                newVariables.push_back(*vi);
                hasContinuous = true;
            }
        }
        else {
            newVariables.push_back(*vi);
        }
    }

    PVariable classVar;
    if (domain->classVar) {
        if (imputeClass) {
            classVar = createImputedVar(domain->classVar);
        }
        if (!classVar) {
            classVar = domain->classVar;
        }
    }

    PDomain imputedDomain(new TDomain(classVar, newVariables));
    TImputer_constant *imputer = new TImputer_constant(imputedDomain);
    PImputer wimputer(imputer);

    if (hasContinuous) {
        TDomainBasicAttrStat basstat(data);
        TExample::iterator aei(imputer->values->begin());
        ITERATE(TDomainBasicAttrStat, bi, basstat) {
            aei++;
            if (*bi) {
                *(aei++) = TValue((*bi)->avg);
            }
        }        
    }
    return wimputer;
}

TImputerConstructor_model::TImputerConstructor_model()
: useClass(false)
{}


PImputer TImputerConstructor_model::operator()(PExampleTable const &data)
{
    TImputer_model *imputer = new TImputer_model;
    PImputer wimputer(imputer);
    imputer->models = PClassifierList(new TClassifierList);
    TVarList vl = *data->domain->variables;
    if (!useClass && data->domain->classVar) {
        vl.erase(vl.end()-1);
    }
    TVarList::iterator vli(vl.begin());
    TVarList::const_iterator vle(vli + data->domain->attributes->size());
    for(; vli != vle; vli++) {
        PLearner &learner = (*vli)->varType == TVariable::Discrete ?
            learnerDiscrete : learnerContinuous;
        if (learner) {
            swap(*vli, vl.back());
            PDomain newdomain(new TDomain(vl));
            PExampleTable newgen =
                TExampleTable::constructConvertedReference(data, newdomain);
            imputer->models->push_back((*learner)(newgen));
            swap(*vli, vl.back());
        }
        else {
            imputer->models->push_back(PClassifier());
        }
    }
    if (data->domain->classVar) {
        PLearner learner;
        if (imputeClass) {
            learner = data->domain->classVar->varType == TVariable::Discrete
                ? learnerDiscrete : learnerContinuous;
        }
        if (learner) {
            imputer->models->push_back((*learner)(data));
        }
        else {
            imputer->models->push_back(PClassifier());
        }
    }
    return wimputer;
}



TImputerConstructor_random::TImputerConstructor_random(const bool dete)
: deterministic(dete)
{}


PImputer TImputerConstructor_random::operator()(PExampleTable const &data)
{
    PDomainBasicAttrStat dbas;
    TDomainBasicAttrStat::const_iterator dbi;
    if (data->domain->hasContinuousAttributes(true)) {
        dbas = PDomainBasicAttrStat(new TDomainBasicAttrStat(data));
        dbi = dbas->begin();
    }
    PDomainDistributions ddist;
    TDomainDistributions::const_iterator ddi;
    if (data->domain->hasDiscreteAttributes(true)) {
        ddist = PDomainDistributions(new TDomainDistributions(data, false, true));
        ddi = ddist->begin();
    }
    PDistributionList distributions(new TDistributionList());
    PITERATE(TVarList, vi, data->domain->variables) {
        if ((*vi)->varType == TVariable::Discrete) {
            distributions->push_back(*ddi);
        }
        else
            distributions->push_back(PGaussianDistribution(
               new TGaussianDistribution((*dbi)->avg, (*dbi)->dev)));
        if (dbas) {
            dbi++;
        }
        if (ddist) {
            ddi++;
        }
    }

    return PImputer(new TImputer_random(imputeClass, deterministic, distributions));
}
