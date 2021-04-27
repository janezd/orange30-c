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


// to include Python.h before STL defines a template set (doesn't work with VC >6.0)
#include "common.hpp" 
#include "scorefeature.px"

double const TScoreFeature::Rejected = numeric_limits<double>::min();

void checkDiscrete(const PContingency &cont, char const *name)
{
    if (cont->varType() != TVariable::Discrete) {
        raiseError(PyExc_TypeError,
            "%s cannot evaluate continuous attributes '%s'",
            name, cont->outerVariable->cname());
    }
    if (cont->innerVariable) {
        if (cont->innerVariable->varType != TVariable::Discrete)
            raiseError(PyExc_TypeError,
                "%s does not work with continuous outcome '%s'",
                name, cont->innerVariable->cname());
    }
    else {
        if (!dynamic_cast<TDiscDistribution *>(
            cont->innerDistribution.borrowPtr())) {
                raiseError(PyExc_TypeError,
                "%s does not work with continuous outcome '%s'", name);
        }
    }
}


void checkDiscreteContinuous(const PContingency &cont, char *name)
{
    if (cont->varType() != TVariable::Discrete) {
        raiseError(PyExc_TypeError,
            "%s cannot evaluate continuous attributes '%s'",
            name, cont->outerVariable->cname());
    }
    if (cont->innerVariable) {
        if (cont->innerVariable->varType != TVariable::Continuous)
            raiseError(PyExc_TypeError,
                "%s does not work with discrete class",
                name, cont->innerVariable->cname());
    }
    else {
        if (!dynamic_cast<TContDistribution *>(
            cont->innerDistribution.borrowPtr())) {
                raiseError(PyExc_TypeError,
                "%s does not work with discrete class", name);
        }
    }
}


void checkHasClass(PExampleTable const &gen)
{
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError,
            "cannot score feature on data without class");
    }
}

/* Prepares the common stuff for binarization through attribute quality assessment:
   - a binary attribute 
   - a contingency matrix for this attribute
   - a DomainContingency that contains this matrix at position newpos (the last)
   - dis0 and dis1 (or con0 and con1, if the class is continuous) that point to distributions
     for the left and the right branch
*/
PContingency TScoreFeature::prepareBinaryCheat(
    PDistribution const &classDistribution,
    PContingency const &origCont,
    PVariable &bvar, 
    TDiscDistribution *&dis0, TDiscDistribution *&dis1,
    TContDistribution *&con0, TContDistribution *&con1)
{
    TDiscreteVariable *ebvar = new TDiscreteVariable("");
    bvar = PVariable(ebvar);
    ebvar->addValue("0");
    ebvar->addValue("1");

    PContingencyClass cont(
        new TContingencyAttrClass(bvar, classDistribution->variable));
    cont->innerDistribution = classDistribution;
    cont->operator[](1);

    PDiscDistribution outerDistribution = 
        PDiscDistribution(cont->outerDistribution);
    outerDistribution->unknowns = origCont->outerDistribution->unknowns;
    outerDistribution->cases = origCont->outerDistribution->cases;
    outerDistribution->abs = origCont->outerDistribution->abs;
    outerDistribution->normalized = origCont->outerDistribution->normalized;

    if (classDistribution->variable->varType == TVariable::Discrete) {
        dis0 = dynamic_cast<TDiscDistribution *>(
            cont->discrete->front().borrowPtr());
        dis1 = dynamic_cast<TDiscDistribution *>(
            cont->discrete->back().borrowPtr());
        con0 = con1 = NULL;
    }
    else {
        con0 = dynamic_cast<TContDistribution *>(
             cont->discrete->front().borrowPtr());
        con1 = dynamic_cast<TContDistribution *>(
             cont->discrete->back().borrowPtr());
        dis0 = dis1 = NULL;
    }
    return cont;
}



TScoreFeature::TScoreFeature(
    bool const ne, bool const hd, bool const hc, bool const ts, int const ut)
: needsExamples(ne),
  handlesDiscrete(hd),
  handlesContinuous(hc),
  computesThresholds(ts),
  unknownsTreatment(ut)
{}


double TScoreFeature::operator()(PContingency const &cont) const
{ 
    if (needsExamples) {
        raiseError(PyExc_SystemError,
            "cannot score features from contingencies"); 
    }
    if (cont->innerVarType() == TVariable::Discrete) {
        if (!handlesDiscrete) {
            raiseError(PyExc_ValueError,
                "feature scorer requires a discrete class");
        }
        if (   (unknownsTreatment == IgnoreUnknowns)
            || !cont->innerDistributionUnknown) {
            return (*this)(cont, PDiscDistribution(cont->innerDistribution));
        }
        else {
            PDiscDistribution classDist = 
                CLONE(PDiscDistribution, cont->innerDistribution);
            *classDist += *cont->innerDistributionUnknown;
            return (*this)(cont, classDist);
        }
    }
    else {
        if (!handlesContinuous) {
            raiseError(PyExc_ValueError,
                "feature scorer requires continuous outcome");
        }
        PContDistribution indist(cont->innerDistribution);
        double var;
        if ((unknownsTreatment != IgnoreUnknowns) &&
            cont->innerDistributionUnknown) {
                PContDistribution indistunk(cont->innerDistributionUnknown);
                var = (indist->abs*indist->var() + indistunk->abs*indistunk->var()) /
                    (indist->abs + indistunk->abs);
        }
        else {
            var = indist->var();
        }
        return (*this)(cont, var);
    }
}


double TScoreFeature::operator()(TAttrIdx const attrNo, PExampleTable const &gen)
{ 
    if (attrNo > int(gen->domain->attributes->size())) {
        raiseError(PyExc_IndexError,
            "attribute index %i out of range", attrNo);
    }
    if (needsExamples) {
        return (*this)(gen->domain->attributes->at(attrNo), gen);
    }
    checkHasClass(gen);
    PContingency contingency(new TContingencyAttrClass(gen, attrNo));
/*****!!!!!!!!!!!!!!!!!!!!!ADD THIS WHERE APPLICABLE!!!!!!!!!
    PDistribution classDistribution = CLONE(TDistribution, contingency.innerDistribution);
    classDistribution->operator+= (contingency.innerDistributionUnknown);
!!!!!!!!!!!!!!!!!!!!!!!****************/
    return (*this)(contingency);
}


double TScoreFeature::operator()(PVariable const &var, PExampleTable const &gen)
{ 
    checkHasClass(gen);
    if (needsExamples) {
        raiseError(PyExc_SystemError,
            "internal error in feature scoring");
    }
    TAttrIdx attrNo = gen->domain->getVarNum(var, false);
    if (attrNo != ILLEGAL_INT) {
        return (*this)(attrNo, gen);
    }
    PContingency contingency(new TContingencyAttrClass(gen, var));
    return (*this)(contingency);
}


double TScoreFeature::operator()(PDistribution const &dist) const
{ 
    TDiscDistribution *discdist =
        dynamic_cast<TDiscDistribution *>(dist.borrowPtr());
    if (discdist) {
        return (*this)(*discdist);
    }
    TContDistribution *contdist =
        dynamic_cast<TContDistribution *>(dist.borrowPtr());
    if (contdist) {
        return (*this)(*contdist);
    }
    raiseError(PyExc_TypeError,
        "invalid distribution type for feature scorer");
    return TScoreFeature::Rejected;
}


double TScoreFeature::operator ()(const TDiscDistribution &) const
{ 
    raiseError(PyExc_TypeError, "cannot score discrete features");
    return TScoreFeature::Rejected;
}


double TScoreFeature::operator ()(
    PContingency const &,
    PDiscDistribution const &) const
{ 
    raiseError(PyExc_TypeError, "cannot score discrete features");
    return TScoreFeature::Rejected;
}


double TScoreFeature::operator ()(const TContDistribution &) const
{ 
    raiseError(PyExc_TypeError, "cannot score continuous features");
    return TScoreFeature::Rejected;
}


double TScoreFeature::operator ()(
    PContingency const &,
    double const) const
{ 
    raiseError(PyExc_TypeError, "cannot score continuous features");
    return TScoreFeature::Rejected;
}


void TScoreFeature::thresholdFunction(
    vector<pair<double, double> > &res,
    PVariable const &var,
    PExampleTable const &gen)
{ 
    if (!computesThresholds || needsExamples) {
        raiseError(PyExc_SystemError, "cannot compute thresholds");
    }
    checkHasClass(gen);
    PContingency contingency(new TContingencyAttrClass(gen, var));
    thresholdFunction(res, contingency);
}


double TScoreFeature::bestThreshold(
    PDistribution &left_right,
    double &score,
    PVariable const &var,
    PExampleTable const &gen,
    double const minSubset)
{ 
    if (needsExamples) {
        vector<pair<double, double> > res;
        thresholdFunction(res, var, gen);
        if (!res.size()) {
            score = TScoreFeature::Rejected;
            return -1;
        }
        score = res.front().second;
        double bestThresh = res.front().first;
        for(vector<pair<double, double> >::const_iterator
            ii(res.begin()), ie(res.end()); ii != ie; ii++) {
                if (ii->second > score) {
                    bestThresh = ii->first;
                    score = ii->second;
            }
        }
        return bestThresh;
    }
    checkHasClass(gen);
    PContingency contingency(new TContingencyAttrClass(gen, var));
    return bestThreshold(left_right, score, contingency, minSubset);
}


template<class TRecorder>
bool TScoreFeature::traverseThresholds(
    TRecorder &recorder, 
    PVariable &bvar,
    PContingency const &origContingency, 
    PDistribution const &classDistribution)
{
    if (needsExamples) {
        raiseError(PyExc_ValueError,
            "cannot compute thresholds from contingencies");
    }
    PVariable var = origContingency->outerVariable;
    if (var->varType != TVariable::Continuous) {
        raiseError(PyExc_ValueError,
            "cannot search for thresholds of a non-continuous variable '%s'",
            var->cname());
    }
    if (origContingency->continuous->size() < 2) {
        return false;
    }
    TDiscDistribution *dis0, *dis1;
    TContDistribution *con0, *con1;
    PContingency cont = 
        prepareBinaryCheat(classDistribution, origContingency, 
                           bvar, dis0, dis1, con0, con1);
    PDiscDistribution outerDistribution(cont->outerDistribution);
    const TDistributionMap &distr = *origContingency->continuous;
    PDistribution const &usedClassDist = unknownsTreatment == IgnoreUnknowns ?
        cont->innerDistribution : classDistribution;
    if (dis0) { // class is discrete
        *dis0 = TDiscDistribution();
        *dis1 = dynamic_cast<TDiscDistribution &>(
            *origContingency->innerDistribution);
        double const &left = dis0->abs, &right = dis1->abs;
        const_ITERATE(TDistributionMap, threshi, distr) {
            *dis0 += dynamic_cast<TDiscDistribution &>(*threshi->second);
            *dis1 -= dynamic_cast<TDiscDistribution &>(*threshi->second);
            if (!recorder.acceptable(threshi->first, left, right)) {
                continue;
            }
            outerDistribution->distribution[0] = left;
            outerDistribution->distribution[1] = right;
            double const score = (*this)(cont, PDiscDistribution(usedClassDist));
            recorder.record(threshi->first, score, left, right);
        }
    }
    else { // class is continuous
        *con0 = TContDistribution();
        *con1 = dynamic_cast<TContDistribution &>(
            *origContingency->innerDistribution);
        double const &left = con0->abs, &right = con1->abs;
        const_ITERATE(TDistributionMap, threshi, distr) {
            *con0 += dynamic_cast<TContDistribution &>(*threshi->second);
            *con1 -= dynamic_cast<TContDistribution &>(*threshi->second);
            if (!recorder.acceptable(threshi->first, left, right)) {
                continue;
            }
            outerDistribution->distribution[0] = left;
            outerDistribution->distribution[1] = right;
            double const score = (*this)(cont, usedClassDist);
            recorder.record(threshi->first, score, left, right);
        }
    }
    return true;
}


class TRecordThresholds {
public:
    vector<pair<double, double> > &res;

    TRecordThresholds(vector<pair<double, double> > &ares)
        : res(ares)
    {}

    inline bool acceptable(double const, double const, double const) const
    { return true; }

    inline void record(double const threshold, double const score,
                       double const left, double const right)
    { 
        if (res.size()) {
            res.back().first = (res.back().first + threshold) / 2.0;
        }
        res.push_back(make_pair(threshold, score)); 
    }
};


void TScoreFeature::thresholdFunction(
    vector<pair<double, double> > &res, PContingency const &origContingency)
{
    PVariable bvar;
    PDistribution classDistribution =
        CLONE(PDistribution, origContingency->innerDistribution);
    if (origContingency->innerDistributionUnknown) {
        *classDistribution += *origContingency->innerDistributionUnknown;
    }

    TRecordThresholds recorder(res);
    if (!traverseThresholds(recorder, bvar, origContingency, classDistribution)) {
        res.clear();
    }
    else if (res.size()) {
        res.erase(res.end()-1);
    }
}


class TRecordMaximalThreshold {
public:
    double const minSubset;

    int wins;
    double bestThreshold, bestScore, bestLeft, bestRight;
    bool fixLast;
    TRandomGenerator &rgen;

    TRecordMaximalThreshold(TRandomGenerator &rg, double const minSub = -1)
        : minSubset(minSub),
        wins(0),
        bestThreshold(-1),
        bestScore(TScoreFeature::Rejected),
        bestLeft(0),
        bestRight(0),
        rgen(rg)
    {}

    inline bool acceptable(double const threshold,
                           double const left, double const right)
    { 
        if (fixLast) {
            bestThreshold = (bestThreshold + threshold) / 2.0;
            fixLast = false;
        }
        return left && (left >= minSubset) && right && (right >= minSubset);
    }

    void record(double const threshold, double const score,
                double const left, double const right)
    {
        if (   (!wins || (score > bestScore)) && ((wins=1)==1)
            || (score == bestScore) && rgen.randbool(++wins)) {
                bestThreshold = threshold;
                fixLast = true;
                bestScore = score;
                bestLeft = left;
                bestRight = right;
        }
    }
};


double TScoreFeature::bestThreshold(PDistribution &subsetSizes,
                                    double &score,
                                    PContingency const &origContingency,
                                    double const minSubset)
{
    PVariable bvar;
    PDistribution classDistribution =
        CLONE(PDistribution, origContingency->innerDistribution);
    if (origContingency->innerDistributionUnknown) {
        *classDistribution += *origContingency->innerDistributionUnknown;
    }
    TRandomGenerator rgen(classDistribution->abs);
    TRecordMaximalThreshold recorder(rgen, minSubset);
    if (   !traverseThresholds(recorder, bvar, origContingency, classDistribution)
        || !recorder.wins) {
        return -1;
    }
    subsetSizes = TDistribution::create(bvar);
    subsetSizes->add(0, recorder.bestLeft);
    subsetSizes->add(1, recorder.bestRight);
    score = recorder.bestScore;
    return recorder.bestThreshold;
}


PIntList TScoreFeature::bestBinarization(
    PDistribution &,
    double &score,
    PContingency const &origContingency,
    double const minSubset)
{
    if (needsExamples) {
        raiseError(PyExc_ValueError,
            "cannot compute optimal binariaztion from contingencies");
    }
    PVariable var = origContingency->outerVariable;
    if (var->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError,
            "cannot binarize a non-continuous variable '%s'", var->cname());
    }
    if (origContingency->continuous->size() < 2) {
        return PIntList();
    }
    raiseError(PyExc_NotImplementedError,
        "'bestBinarization' has not been implemented yet");
    return PIntList();
}


PIntList TScoreFeature::bestBinarization(
    PDistribution &subsets,
    double &score,
    PVariable const &var,
    PExampleTable const &gen,
    double const minSubset)
{ 
    if (!computesThresholds || needsExamples) {
        raiseError(PyExc_ValueError, "cannot search for best binarization");
    }
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError,
            "cannot score feature on data without class");
    }
    PContingencyAttrClass contingency(
        new TContingencyAttrClass(gen, var));
    return bestBinarization(subsets, score, PContingency(contingency), minSubset);
}


int TScoreFeature::bestValue(
    PDistribution &,
    double &score,
    PContingency const &origContingency, 
    double const minSubset)
{
    raiseError(PyExc_TypeError,
        "bestValue is not supported by the selected feature score");
    return TScoreFeature::Rejected;
}


int TScoreFeature::bestValue(
    PDistribution &,
    double &score,
    PVariable const &,
    PExampleTable const &,
    double const minSubset)
{
    raiseError(PyExc_TypeError,
        "bestValue is not supported by the selected feature score");
    return TScoreFeature::Rejected;
}


bool TScoreFeature::checkClassType(int const varType) const
{
    return varType == TVariable::Discrete ? handlesDiscrete : handlesContinuous;
}


void TScoreFeature::checkClassTypeExc(const int varType) const
{
    if (varType == TVariable::Discrete) {
        if (!handlesDiscrete) {
            raiseError(PyExc_ValueError,
                "feature scorer requires continuous outcome");
        }
    }
    else {
        if (!handlesContinuous) {
            raiseError(PyExc_ValueError,
                "feature scorer requires a discrete class");
        }
    }
}



inline double round0(const double &x)
{ 
    return fabs(x) < 1e-6 ? 0.0 : x;
}


double getEntropy(double const *ptr, size_t nValues)
{ 
    double n = 0, sum = 0;
    for(; nValues--; ptr++) {
        if (*ptr > 0) {
            sum += *ptr * log(*ptr);
            n += *ptr;
        }
    }
    return n ? (log(n) - sum/n) / log(2.0) : 0;
}


double getEntropy(PContingency const &cont)
{ 
    checkDiscrete(cont, "getEntropy");
    double sum = 0.0, N = 0.0;
    PDiscDistribution outer(cont->outerDistribution);
    const_ITERATE(TDistributionVector, ci, *cont->discrete) {
        PDiscDistribution dist(*ci);
        N += dist->cases;
        sum += dist->cases * getEntropy(*dist);
    }
    return N ? sum/N : 0.0;
}



TScoreFeature_info::TScoreFeature_info(int const unk)
: TScoreFeature(false, true, false, true, unk)
{}


double TScoreFeature_info::operator()(
    PContingency const &probabilities,
    PDiscDistribution const &classProbabilities) const
{ 
    // checkDiscrete is called by getEntropy
    PDistribution outer = probabilities->outerDistribution;
    if (!outer->cases || (
        (unknownsTreatment == ReduceByUnknowns) &&
        (outer->unknowns == outer->cases))) {
        return 0.0;
    }
    double info = getEntropy(*classProbabilities) - getEntropy(probabilities);
    if (unknownsTreatment == ReduceByUnknowns) {
        info *= 1 - outer->unknowns/outer->cases;
    }
    return round0(info);
}


double TScoreFeature_info::operator()(const TDiscDistribution &dist) const
{ 
  return -getEntropy(dist);
}


TScoreFeature_gainRatio::TScoreFeature_gainRatio(int const unk)
: TScoreFeature(false, true, false, true, unk)
{}


double TScoreFeature_gainRatio::operator()(
    PContingency const &probabilities,
    PDiscDistribution const &classProbabilities) const
{ 
    checkDiscrete(probabilities, "ScoreFeature_gainRatio");
    PDiscDistribution outer(probabilities->outerDistribution);
    if (!outer->cases || (
        (unknownsTreatment == ReduceByUnknowns) &&
        (outer->unknowns == outer->cases))) {
        return 0.0;
    }
    double const attributeEntropy = getEntropy(*outer);
    if (attributeEntropy < 1e-20) {
        return 0.0;
    }
    double gain = getEntropy(*classProbabilities) - getEntropy(probabilities);
    if (gain < 1e-20) {
        return 0.0;
    }
    gain /= attributeEntropy;
    if (unknownsTreatment == ReduceByUnknowns) {
        gain *= 1 - outer->unknowns / outer->cases;
    }
    return round0(gain);
}


double getGini(double const *ptr, size_t nValues)
{ 
    double sum = 0.0, N = 0.0;
    for(; nValues--; ptr++) {
        N += *ptr;
        sum += sqr(*ptr);
    }
    return N ? (1 - sum/N/N)/2 : 0.0;
}


double getGini(PContingency const &cont)
{ 
    checkDiscrete(cont, "getGini");
    double sum = 0.0, N = 0.0;
    PDiscDistribution outer(cont->outerDistribution);
    const_ITERATE(TDistributionVector, ci, *cont->discrete) {
        PDiscDistribution dist(*ci);
        N += dist->cases;
        sum += dist->cases * getGini(*dist);
    }
    return N ? sum/N : 0.0;
}


TScoreFeature_gini::TScoreFeature_gini(int const unk)
: TScoreFeature(false, true, false, true, unk)
{}


double TScoreFeature_gini::operator()(PContingency const &probabilities,
                                      PDiscDistribution const &classProbabilities) const
{ 
    checkDiscrete(probabilities, "ScoreFeature_gini");
    PDistribution const &outer(probabilities->outerDistribution);
    if (!outer->cases || (
        (unknownsTreatment == ReduceByUnknowns) &&
        (outer->unknowns == outer->cases))) {
            return 0.0;
    }
    double gini = getGini(*classProbabilities) - getGini(probabilities);
    if (unknownsTreatment == ReduceByUnknowns) {
        gini *= 1 - outer->unknowns / outer->cases;
    }
    return round0(gini);
}


double TScoreFeature_gini::operator()(TDiscDistribution const &dist) const
{ 
    return round0(-getGini(dist));
}



TScoreFeature_relevance::TScoreFeature_relevance(int const unk)
: TScoreFeature(false,true, false, true, unk)
{}



double TScoreFeature_relevance::valueRelevance(
    const TDiscDistribution &dval, const TDiscDistribution &classDist) const
{ 
    TDiscDistribution::const_iterator ci(classDist.begin()), ce(classDist.end());
    TDiscDistribution::const_iterator hci(ci);
    TDiscDistribution::const_iterator di(dval.begin()), de(dval.end());
    for (; (di!=de) && (ci!=ce) && (*ci < 1e-20); ci++, di++);
    if ((ci==ce) || (di==de)) {
        return 0.0;
    }
    /* 'leftout' is the element for the most probable class encountered so far
    If a more probable class appears, 'leftout' is added and new leftout taken
    If there is more than one most probable class, the one with higher aprior probability
    is taken (as this gives higher relevance). If there is more than one such class,
    it doesn't matter which one we take. */  
    double relev = 0.0;
    double highestProb = *di;
    double leftout = *di / *ci;
    while(++ci!=ce && ++di!=de) {
        if (*ci>=1e-20) {
            double const tras = *di / *ci;
            if (   (*di >  highestProb)
                || (*di == highestProb) && (leftout < tras)) {
                    relev += leftout;
                    leftout = tras;
                    highestProb = *di;
            }
            else {
                relev += tras;
            }
        }
    }
    return relev;
}


double TScoreFeature_relevance::operator()(
    PContingency const &probabilities,
    PDiscDistribution const &classProbabilities) const
{ 
    checkDiscrete(probabilities, "ScoreFeature_relevance");
    PDistribution const &outer(probabilities->outerDistribution);
    if (!outer->cases ||
        ((unknownsTreatment == ReduceByUnknowns) &&
         (outer->unknowns == outer->cases))) {
        return 0.0;
    }
    int C = 0;
    const_PITERATE(TDiscDistribution, di, classProbabilities) {
        if (*di > 1e-20) {
            C++;
        }
    }
    if (C <= 1.0) {
        return 0.0;
    }
    double relevance = 0.0;
    const_ITERATE(TDistributionVector, ci, *probabilities->discrete) {
        TDiscDistribution &dist = dynamic_cast<TDiscDistribution &>(**ci);
        relevance += valueRelevance(dist, *classProbabilities);
    }
    relevance = 1.0 - relevance / (C-1);
    if (unknownsTreatment == TScoreFeature::ReduceByUnknowns) {
        relevance *= 1 - outer->unknowns / outer->cases;
    }
    return round0(relevance);
}


/*

TScoreFeature_cost::TScoreFeature_cost(PCostMatrix costs)
: TScoreFeatureFromProbabilities(true, false),
  cost(costs)
{}


float TScoreFeature_cost::majorityCost(const TDiscDistribution &dval)
{ float cost;
  TValue cclass;
  majorityCost(dval, cost, cclass);
  return cost;
}


void TScoreFeature_cost::majorityCost(const TDiscDistribution &dval, float &ccost, TValue &cclass)
{ 
  checkProperty(cost);

  int dsize = dval.size();
  if (dsize > cost->dimension)
    raiseError("cost matrix is too small");

  TRandomGenerator srgen(dval.sumValues());

  ccost = numeric_limits<float>::max();
  int wins = 0, bestPrediction;

  for(int predicted = 0; predicted < dsize; predicted++) {
    float thisCost = 0;
    for(int correct = 0; correct < dsize; correct++)
      thisCost += dval[correct] * cost->cost(predicted, correct);

    if (   (thisCost<ccost) && ((wins=1)==1)
        || (thisCost==ccost) && srgen.randbool(++wins)) {
      bestPrediction = predicted;
      ccost = thisCost; 
    }
  }
  
  ccost /= dval.abs;
  cclass = TValue(bestPrediction);
}


float TScoreFeature_cost::operator()(PContingency probabilities, const TDiscDistribution &classProbabilities)
{ 
  checkDiscrete(probabilities, "MeasureAttribute_cost");

  const TDistribution &outer = probabilities->outerDistribution.getReference();
  if ((unknownsTreatment == ReduceByUnknowns) && (outer.unknowns == outer.cases))
    return 0.0;
 
  checkProperty(cost);

  float stopCost = majorityCost(classProbabilities);
  
  TDistributionVector::const_iterator mostCommon = (unknownsTreatment == UnknownsToCommon)
    ? probabilities->discrete->begin() + outer.highestProbIntIndex()
    : probabilities->discrete->end();

  float continueCost = 0;
  float N = 0;
  const_ITERATE(TDistributionVector, ci, *probabilities->discrete) {
    const TDiscDistribution &dist = CAST_TO_DISCDISTRIBUTION(*ci);
    if (ci == mostCommon) {
      TDiscDistribution dist2 = dist;
      dist2 += probabilities->innerDistributionUnknown;
      if (dist2.cases && dist2.abs) {
        N += dist2.cases;
        continueCost += dist2.cases * majorityCost(dist2);
      }
    }
    else {
      if (dist.cases && dist.abs) {
        N += dist.cases;
        continueCost += dist.cases * majorityCost(dist.distribution);
      }
    }
  }

  if (unknownsTreatment == UnknownsAsValue) {
    const float &cases = probabilities->innerDistributionUnknown->cases;
    if (cases) {
      N += cases;
      continueCost += cases * majorityCost(CAST_TO_DISCDISTRIBUTION(probabilities->innerDistributionUnknown));
    }
  }

  if (N)
    continueCost /= N;

  float cost = stopCost - continueCost;
  if ((unknownsTreatment == ReduceByUnknowns) && outer.unknowns) // to avoid div by zero if !cases, too
    cost *= (outer.cases / (outer.unknowns + outer.cases));

  return round0(cost);
}

*/

TScoreFeature_MSE::TScoreFeature_MSE(int const unk)
: TScoreFeature(false, false, true, true, unk),
  m(0),
  priorVariance(0)
{}


double TScoreFeature_MSE::operator()(PContingency const &cont, double const var) const
{
    checkDiscreteContinuous(cont, "ScoreFeature_MSE");
    TDistribution const &outer =
        dynamic_cast<TDiscDistribution &>(*cont->outerDistribution);
    if (var < 1e-6) {
        return 0.0;
    }
    double I = 0;
    double downW = 0;
    const_ITERATE(TDistributionVector, ci, *cont->discrete) {
        const TContDistribution &tdist = dynamic_cast<TContDistribution &>(**ci);
        if (tdist.abs > 1e-6) {
            I += tdist.sum2 - tdist.sum*tdist.sum/tdist.abs;
            downW += tdist.abs;
        }
    }

    if (priorVariance > 1e-6 && (m > 1e-6)) {
        I = (I + m * priorVariance) / (downW + m);
    }
    else {
        I /= downW;
    }
    double mse = (var - I)/var;
    if (unknownsTreatment == ReduceByUnknowns) {
        mse *= (outer.cases / (outer.unknowns + outer.cases));
    }
    return round0(mse);
}




TScoreFeature_relief::TScoreFeature_relief(int ak, int am)
: TScoreFeature(true, true, true, true, ReduceByUnknowns), 
  k(ak),
  m(am),
  checkCachedData(true),
  prevExamples(-1),
  prevChecksum(0),
  prevK(-1),
  prevM(-1)
{}




inline bool compare2nd(const pair<int, double> &o1, const pair<int, double> &o2)
{ return o1.second < o2.second; }


void TScoreFeature_relief::prepareNeighbours(PExampleTable const &table)
{
    neighbourhood.clear();
    if (!table->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    bool const regression =
        table->domain->classVar->varType == TVariable::Continuous;
    storedExamples = table;
    int const N = table->size();
    int const classIdx = table->domain->attributes->size();
    int const nClasses = table->domain->classVar->noOfValues();
    vector<vector<int> > examplesByClasses(regression ? 1 : nClasses);
    vector<int> classSizes;
    classSizes.reserve(examplesByClasses.size());
    vector<vector<int > >::iterator ebcb(examplesByClasses.begin());
    vector<vector<int > >::iterator ebce(examplesByClasses.end());
    vector<vector<int > >::iterator ebci;
    double minCl = numeric_limits<double>::max();
    double maxCl = numeric_limits<double>::min();
    if (regression) {
        vector<int> &cls = *ebcb;
        cls.reserve(N);
        PEITERATE(ei, table) {
            TValue clsval = ei.getClass();
            if (!isnan(clsval)) {
                cls.push_back(ei.index());
                if (clsval > maxCl) {
                    maxCl = clsval;
                }
                if (clsval < minCl) {
                    minCl = clsval;
                }
            }
        }
        if (!cls.size()) {
            raiseError(PyExc_ValueError, "no data instances");
        }
        classSizes.push_back(cls.size());
    }
    else {
        PEITERATE(ei, table) {
            TValue clsval = ei.getClass();
            if (!isnan(clsval)) {
                if (int(clsval) >= nClasses) {
                    raiseError(PyExc_IndexError,
                        "class index %i out of range", int(clsval));
                }
                examplesByClasses[int(clsval)].push_back(ei.index());
            }
        }
        for(ebci = ebcb; ebci != ebce; ebci++) {
            classSizes.push_back(ebci->size());
        }
    }

    distance = TExamplesDistanceConstructor_Relief()(table);
    TExamplesDistance_Relief const &rdistance =
        dynamic_cast<const TExamplesDistance_Relief &>(*distance.borrowPtr());

    TRandomGenerator rgen(N);
    int referenceIndex = 0;
    const bool useAll = (m==-1) || (!table->hasWeights() && (m>N));
    double referenceExamples, referenceWeight;
    TExampleTable::iterator ei(table->begin());
    TExampleTable::iterator nei(table->begin());
    for(referenceExamples = 0;
        useAll ? (referenceIndex < N) : (referenceExamples < m);
        referenceIndex++, referenceExamples += referenceWeight) {
            if (!useAll) {
                referenceIndex = rgen.randlong(N);
            }
            ei.seek(referenceIndex);
            referenceWeight = ei.getWeight();
            TValue const referenceClass = ei.getClass();
            neighbourhood.push_back(referenceIndex);
            vector<TNeighbourExample> &refNeighbours =
                neighbourhood.back().neighbours;
            ndC = 0.0;
            ITERATE(vector<vector<int> >, cli, examplesByClasses) {
                int const inCliClass = cli->size();
                if (!inCliClass) {
                    continue;
                }
                vector<pair<int, double> > distances(inCliClass);
                vector<pair<int, double> >::iterator disti(distances.begin());
                ITERATE(vector<int> , clii, *cli) {
                    nei.seek(*clii);
                    double const dist = rdistance(*ei, *nei);
                    *disti++ = make_pair(*clii, dist);
                }
                disti = distances.begin();
                vector<pair<int, double> >::iterator diste(distances.end());
                sort(disti, diste, compare2nd);
                int startNew = refNeighbours.size();
                while((disti != diste) && (disti->second < 1e-6)) {
                    disti++;
                }
                double inWeight, needwei;
                for(needwei = k; (disti != diste) && (needwei > 1e-6); ) {
                    double const thisDist = disti->second;
                    inWeight = 0.0;
                    int const inAdded = refNeighbours.size();
                    do {
                        nei.seek(disti->first);
                        double const neighbourWeight = nei.getWeight();
                        double const weightEE = neighbourWeight * referenceWeight;
                        inWeight += neighbourWeight;
                        TValue const neighbourClass = nei.getClass();
                        if (regression) {
                            double const classDist = rdistance(
                                classIdx, neighbourClass, referenceClass);
                            refNeighbours.push_back(TNeighbourExample(
                                disti->first, weightEE * classDist, weightEE));
                            ndC += weightEE * classDist;
                        }
                        else {
                            double const classDist =
                                (neighbourClass == referenceClass ? -1 :
                                double(inCliClass) / (N - classSizes[int(neighbourClass)]));
                            refNeighbours.push_back(TNeighbourExample(
                                disti->first, weightEE * classDist));
                        }
                    } while ((++disti != diste) && (disti->second == thisDist));
                    needwei -= inWeight;
                }
                if (k-needwei > 1) {
                    double const adj = 1.0 / (k - needwei);
                    vector<TNeighbourExample>::iterator
                        ai(refNeighbours.begin() + startNew),
                        ae(refNeighbours.end());
                    if (regression) {
                        for(; ai != ae; ai++) {
                            ai->weight *= adj;
                            ai->weightEE *= adj;
                        }
                    }
                    else {
                        for(; ai != ae; ai++) {
                            ai->weight *= adj;
                        }
                    }
                }
            }
    }
    if (regression) {
        m_ndC = referenceExamples - ndC;
    }
    else {
        ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
            double const adj = 1.0 / referenceExamples;
            ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                nei->weight *= adj;
            }
        }
    }
}


void TScoreFeature_relief::checkNeighbourhood(PExampleTable const &gen)
{
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    unsigned int newChecksum;
    bool renew = false;
    if ((prevExamples != gen->version) ||
        (k != prevK) ||
        (m != prevM)) {
        newChecksum = gen->checkSum(true);
        renew = true;
    }
    else if (checkCachedData) {
        newChecksum = gen->checkSum(true);
        renew = newChecksum != prevChecksum;
    }
    if (renew)  {
        scores.clear();
        prepareNeighbours(gen);
        prevExamples = gen->version;
        prevChecksum = newChecksum;
        prevK = k;
        prevM = m;
    }
}


double *TScoreFeature_relief::extractContinuousValues(
    PExampleTable const &gen,
    TVariable &variable,
    TValue &min, TValue &max, TValue &avg, double &N)
{
    avg = N = 0.0;
    min = numeric_limits<double>::max();
    max = numeric_limits<double>::min();
    double *pc, *precals;
    precals = pc = new double[gen->size()];
    try {
        PEITERATE(ei, gen) {
            const TValue val = variable.computeValue(*ei);
            *pc++ = val;
            if (!isnan(val)) {
                if (val > max) {
                    max = val;
                }
                if (val < min) {
                    min = val;
                }
                TValue const w = ei.getWeight();
                avg += w * val;
                N += w;
            }
        }
        if (N > 1e-6) {
            avg /= N;
        }
    }
    catch (...) {
        delete precals;
        throw;
    }
    return precals;
}


int *TScoreFeature_relief::extractDiscreteValues(
    PExampleTable const &gen, 
    TVariable &variable,
    double *&unk, double &bothUnk)
{
    const int noVal = variable.noOfValues();
    int *pc, *precals;
    pc = precals = new int[gen->size()];
    unk = new double[noVal];
    try {
        double *ui, *ue = unk + noVal;
        for(ui = unk; ui != ue; *ui++ = 0.0);
        PEITERATE(ei, gen) {
            TValue const val = variable.computeValue(*ei);
            if (!isnan(val) && (val >= 0) && (val < noVal)) {
                unk[*pc++ = int(val)] += ei.getWeight();
            }
            else {
                *pc++ = -1;
            }
        }
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // This code is weird: looks like unk is expected to be normalized!!!
        bothUnk = 1.0;
        for(ui = unk; ui != ue; ui++) {
            bothUnk -= *ui * *ui;
            *ui = 1 - *ui;
        }
    }
    catch (...) {
        delete unk;
        delete precals;
        throw;
    }
    return precals;
}


double TScoreFeature_relief::operator()(
        TAttrIdx const attrNo, PExampleTable const &gen)
{
    if (scores.empty()) {
        const TExamplesDistance_Relief &rdistance =
            dynamic_cast<const TExamplesDistance_Relief &>(*distance.borrowPtr());
        const TExampleTable &table =
            dynamic_cast<const TExampleTable &>(*gen.borrowPtr());
        const int nAttrs = gen->domain->attributes->size();
        scores.assign(nAttrs, 0.0);
        vector<double>::iterator mb(scores.begin()), mi;
        vector<double>::const_iterator const me(scores.end());
        TExample::const_iterator e1i, e1b, e2i;
        TExampleTable::iterator refExi(gen), neiExi(gen);
        int attrNo;
        if (gen->domain->classVar->varType == TVariable::Continuous) {
            vector<double> ndA(nAttrs, 0.0);
            vector<double> ndCdA(nAttrs, 0.0);
            vector<double>::iterator ndAb(ndA.begin()), ndAi;
            vector<double>::const_iterator const ndAe(ndA.end());
            vector<double>::iterator ndCdAb(ndCdA.begin()), ndCdAi;
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                refExi.seek(rei->index);
                e1b = refExi->begin();
                ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                    double const weight = nei->weight;
                    double const weightEE = nei->weightEE;
                    neiExi.seek(nei->index);
                    attrNo = 0;
                    e1i = e1b;
                    e2i = neiExi->begin();
                    ndAi = ndAb;
                    ndCdAi = ndCdAb;
                    while(ndAi != ndAe) {
                        double const attrDist = rdistance(attrNo++, *e1i++, *e2i++);
                        *ndAi++ += weightEE * attrDist;
                        *ndCdAi++ += weight * attrDist;
                    }
                }
            }
            for(ndAi = ndAb, ndCdAi = ndCdAb, mi = mb; mi != me; mi++, ndAi++, ndCdAi++)
                *mi = *ndCdAi / ndC - (*ndAi - *ndCdAi) / m_ndC;
        }
        else {
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                refExi.seek(rei->index);
                e1b = refExi->begin();
                ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                    neiExi.seek(nei->index);
                    double const weight = nei->weight;
                    attrNo = 0;
                    e1i = e1b;
                    e2i = neiExi->begin();
                    mi = mb;
                    while(mi != me) {
                        *mi++ += weight * rdistance(attrNo++, *e1i++, *e2i++);
                    }
                }
            }
        }
    }
    return scores[attrNo];
}



double TScoreFeature_relief::compute_cont(
    TVariable &var, PExampleTable const &gen)
{
    double avg, min, max, N;
    double *precals = extractContinuousValues(gen, var, min, max, avg, N);
    if ((min == max) || (N < 1e-6)) {
        delete precals;
        return 0.0;
    }
    try {
        double const nor = 1.0 / (min-max);
        // continuous attribute, continuous class
        if (gen->domain->classVar->varType == TVariable::Continuous) {
            double ndA = 0.0, ndCdA = 0.0;
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                TValue const refVal = precals[rei->index];
                if (isnan(refVal)) {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        TValue const neiVal = precals[nei->index];
                        double const attrDist =
                            isnan(neiVal) ? 0.5 : fabs(avg - neiVal) * nor;
                        ndA += nei->weightEE * attrDist;
                        ndCdA += nei->weight * attrDist;
                    }
                }
                else {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        TValue const neiVal = precals[nei->index];
                        double const attrDist =
                            fabs(refVal - isnan(neiVal) ? avg : neiVal) * nor;
                        ndA += nei->weightEE * attrDist;
                        ndCdA += nei->weight * attrDist;
                    }
                }
            }
            delete precals;
            return ndCdA / ndC - (ndA - ndCdA) / m_ndC;
        }
        // continuous attribute, discrete class
        else {
            double relf = 0.0;
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                TValue const refVal = precals[rei->index];
                if (isnan(refVal)) {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        TValue const neiVal = precals[nei->index];
                        double const attrDist =
                            isnan(neiVal) ? 0.5 : fabs(avg - neiVal) * nor;
                        relf += nei->weight * attrDist;
                    }
                }
                else {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        TValue const neiVal = precals[nei->index];
                        double const attrDist =
                            fabs(refVal - isnan(neiVal) ? avg : neiVal) * nor;
                        relf += nei->weight * attrDist;
                    }
                }
            }
            delete precals;
            return relf;
        }
    }
    catch (...) {
        delete precals;
        throw;
    }
}


double TScoreFeature_relief::compute_disc(
    TVariable &var, PExampleTable const &gen)
{
    double *unk, bothUnk;
    int *precals = extractDiscreteValues(gen, var, unk, bothUnk);
    try {
        if (gen->domain->classVar->varType == TVariable::Continuous) {
            double ndA = 0.0, ndCdA = 0.0;
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                int const refVal = precals[rei->index];
                if (refVal < 0) {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        int const neiVal = precals[nei->index];
                        double const attrDist = neiVal < 0 ? bothUnk : unk[neiVal];
                        ndA += nei->weightEE * attrDist;
                        ndCdA += nei->weight * attrDist;
                    }
                }
                else {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        int const neiVal = precals[nei->index];
                        double const attrDist = neiVal < 0 ? unk[refVal] :
                            (refVal != neiVal ? 1.0 : 0.0);
                        ndA += nei->weightEE * attrDist;
                        ndCdA += nei->weight * attrDist;
                    }
                }
            }
            delete unk;
            delete precals;
            return ndCdA / ndC - (ndA - ndCdA) / m_ndC;
        }
        else {
            double relf = 0.0;
            ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                int const refVal = precals[rei->index];
                if (refVal < 0) {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        int const neiVal = precals[nei->index];
                        relf += nei->weight * (neiVal < 0 ? bothUnk : unk[neiVal]);
                    }
                }
                else {
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        const int neiVal = precals[nei->index];
                        relf += nei->weight * (neiVal < 0) ?
                            unk[refVal] : (refVal != neiVal ? 1.0 : 0.0);
                    }
                }
            }
            delete unk;
            delete precals;
            return relf;
        }
    }
    catch (...) {
        delete unk;
        delete precals;
        throw;
    }
}
    
double TScoreFeature_relief::operator()(
    PVariable const &var, PExampleTable const &gen)
{
    checkNeighbourhood(gen);
    const int attrIdx = gen->domain->getVarNum(var, false);
    if (attrIdx == gen->domain->attributes->size()) {
        raiseError(PyExc_ValueError, "cannot score the class attribute");
    }
    if (attrIdx != ILLEGAL_INT) {
        return (*this)(attrIdx, gen);
    }
    if (!var->getValueFrom) {
        raiseError(PyExc_ValueError,
            "feature '%s' cannot be computed from other features", var->cname());
    }
    TVariable &variable = *var;
    if (variable.varType == TVariable::Continuous) {
        return compute_disc(variable, gen);
    }
    else {
        return compute_disc(variable, gen);
    }
}



void TScoreFeature_relief::thresholdFunction(
    vector<pair<double, double> > &res,
    PVariable const &var,
    PExampleTable const &gen)
{
    TFunctionAdder divs;
    thresholdFunction(var, gen, divs);
    res.clear();
    double score = 0;
    for(TFunctionAdder::const_iterator di(divs.begin()), de(divs.end()); di != de; di++) {
        res.push_back(make_pair(di->first, score += di->second));
    }
}


double TScoreFeature_relief::bestThreshold(
    PDistribution &subsetSizes, double &bestScore,
    PVariable const &var,
    PExampleTable const &gen,
    double const minSubset)
{
    TFunctionAdder divs;
    int wins = 0;
    double score = 0.0, bestThreshold;
    TRandomGenerator rgen(gen->size());
    if (minSubset > 0) {
        double *attrVals;
        thresholdFunction(var, gen, divs, &attrVals);
        TContDistribution *valueDistribution;
        PDistribution wvd;
        if (attrVals) {
            try {
                double *vali = attrVals, *vale;
                wvd = PDistribution(valueDistribution=new TContDistribution(var));
                if (gen->hasWeights()) {
                    for(TExampleIterator ei(gen); ei; ++ei, vali++) {
                        if (isnan(*vali)) {
                            valueDistribution->add(*vali, ei.getWeight());
                        }
                        else {
                            for(vali = attrVals, vale = attrVals + gen->size();
                                vali != vale; vali++) {
                                    if (isnan(*vali)) {
                                        valueDistribution->add(*vali);
                                    }
                            }
                        }
                    }
                }
            }
            catch (...) {
                delete attrVals;
                throw;
            }
            delete attrVals;
            attrVals = NULL;
        }
        else {
            wvd = TDistribution::fromExamples(gen, var);
            valueDistribution = dynamic_cast<TContDistribution *>(wvd.borrowPtr());
        }
        double left = 0.0, right = valueDistribution->abs;
        double bestLeft, bestRight;
        map<double, double>::iterator distb(valueDistribution->begin()),
            diste(valueDistribution->end()), disti = distb, disti2;
        TFunctionAdder::const_iterator di(divs.begin()), de(divs.end());
        for(; di != de; di++) {
            score += di->second;
            if (!wins || (score > bestScore) || (score == bestScore) && rgen.randbool(++wins)) {
                for(; (disti != diste) && (disti->first <= di->first); disti++) {
                    left += disti->second;
                    right -= disti->second;
                }
                if ((left < minSubset)) {
                    continue;
                }
                if ((right < minSubset) || (disti == diste)) {
                    break;
                }
                if (!wins || (score > bestScore)) {
                    wins = 1;
                }
                bestScore = score;
                bestLeft = left;
                bestRight = right;
                // disti cannot be distb (contemplate the above for)
                disti2 = disti;
                bestThreshold = (disti->first + (--disti2)->first) / 2.0;
            }
        }
        if (!wins) {
            subsetSizes = PDistribution();
            return TScoreFeature::Rejected;
        }
        subsetSizes = PDistribution(new TDiscDistribution(2));
        subsetSizes->add(0, bestLeft);
        subsetSizes->add(1, bestRight);
        return bestThreshold;
    }
    else {
        thresholdFunction(var, gen, divs);
        TFunctionAdder::const_iterator db(divs.begin()), de(divs.end()),
            di = db, di2;
        for(; di != de; di++) {
            score += di->second;
            if (   (!wins || (score > bestScore)) && ((wins=1) == 1)
                || (score == bestScore) && rgen.randbool(++wins)) {
                    di2 = di;
                    bestThreshold = (++di2 == de) ?
                        di->first : (di->first + di2->first) / 2.0;
                    bestScore = score;
            }
        }
        subsetSizes = PDistribution();
        if (!wins) {
            bestScore = TScoreFeature::Rejected;
            return -1;
        }
        return bestThreshold;
    }
}

/*
PSymMatrix TScoreFeature_relief::gainMatrix(PVariable var, PExampleGenerator gen, PDistribution, int **attrVals, float **attrDistr)
{
  TDiscreteVariable *evar = var.AS(TDiscreteVariable);
  if (!evar)
    raiseError("thresholdFunction can only be computed for continuous attributes");

  checkNeighbourhood(gen);

  TSymMatrix *gains = new TSymMatrix(evar->noOfValues());
  PSymMatrix wgains = gains;

  const int attrIdx = gen->domain->getVarNum(var, false);
  const bool regression = gen->domain->classVar->varType == TValue::FLOATVAR;

  if (attrIdx != ILLEGAL_INT) {
    if (attrVals)
      *attrVals = NULL;
    if (attrDistr)
      *attrDistr = NULL;

    const TExamplesDistance_Relief &rdistance = dynamic_cast<const TExamplesDistance_Relief &>(distance.getReference());
    const TExampleTable &table = dynamic_cast<const TExampleTable &>(gen.getReference());

    ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
      const TValue &refVal = table[rei->index][attrIdx];
      if (refVal.isSpecial())
        continue;
      const int &refValI = refVal.intV;

      ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
        const TValue &neiVal = table[nei->index][attrIdx];
        if (neiVal.isSpecial())
          continue;

        const float attrDist = rdistance(attrIdx, refVal, neiVal);
        if (regression) {
          const float dCdA = nei->weight * attrDist;
          const float dA = nei->weightEE * attrDist;
          gains->getref(refValI, neiVal.intV) += dCdA / ndC - (dA - dCdA) / m_ndC;
        }
        else
          gains->getref(refValI, neiVal.intV) += nei->weight * attrDist;
      }
    }
  }

  else {
    if (!var->getValueFrom)
      raiseError("attribute is not among the domain attributes and cannot be computed from them");

    float *unk, bothUnk;
    int *precals = tabulateDiscreteValues(gen, var.getReference(), unk, bothUnk);
    if (attrVals)
      *attrVals = precals;
    if (attrDistr) {
      const int noVal = evar->noOfValues();
      *attrDistr = new float[noVal];
      for(float *ai = *attrDistr, *ui = unk, *ue = unk + noVal; ui != ue; *ai++ = 1 - *ui++);
    }

    try {
      ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
        const int refValI = precals[rei->index];
        ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
          const int neiVal = precals[nei->index];
          const int attrDist = (refValI == ILLEGAL_INT) ? ((neiVal == ILLEGAL_INT) ? bothUnk : unk[neiVal])
                                                        : ((neiVal == ILLEGAL_INT) ? unk[refValI] : (refValI != neiVal ? 1.0 : 0.0));
          if (attrDist == 0.0)
            continue;
          if (regression) {
            const float dCdA = nei->weight * attrDist;
            const float dA = nei->weightEE * attrDist;
            gains->getref(refValI, neiVal) += dCdA / ndC - (dA - dCdA) / m_ndC;
          }
          else
            gains->getref(refValI, neiVal) += nei->weight * attrDist;
        }
      }

      delete unk;
      if (!attrVals)
        delete precals;
    }
    catch (...) {
      if (unk)
        delete unk;
      if (precals)
        delete precals;
      throw;
    }
  }

  return wgains;
}


PIntList TScoreFeature_relief::bestBinarization(PDistribution &subsetSizes, float &bestScore, PVariable var, PExampleGenerator gen, PDistribution apriorClass, const float &minSubset)
{
  TDiscreteVariable *evar = var.AS(TDiscreteVariable);
  if (!evar)
    raiseError("cannot discretly binarize a continuous attribute");

  const int noVal = evar->noOfValues();
  if (noVal > 16)
    raiseError("cannot binarize an attribute with more than 16 values (it would take too long)");

  float *attrDistr = NULL;
  PSymMatrix wgain = gainMatrix(var, gen, apriorClass, NULL, &attrDistr);
  TSymMatrix &gain = wgain.getReference();

  float *gains = new float[noVal * noVal], *gi = gains, *ge;

  int wins = 0, bestSubset;
  float bestLeft, bestRight;

  try {
    float thisScore = 0.0;
    int i, j;
    for(i = 0; i < noVal; i++)
      for(j = 0; j < noVal; j++)
        *gi++ = gain.getitem(i, j);

    float thisLeft = 0.0, thisRight = 0.0;
    float *ai, *ae;
    if (!attrDistr) {
      TDiscDistribution dd(gen, var);
      attrDistr = new float[noVal];
      ai = attrDistr;
      ae = attrDistr + noVal;
      for(vector<float>::const_iterator di(dd.distribution.begin()); ai != ae; thisLeft += (*ai++ = *di++));
    }
    else
      for(ai = attrDistr, ae = attrDistr + noVal; ai != ae; thisLeft += *ai++);

    if (thisLeft < minSubset)
      return NULL;

    bestSubset = 0;
    wins = 0;
    bestLeft = thisLeft;
    bestRight = 0.0;
    bestScore = 0;

    TRandomGenerator rgen(gen->numberOfExamples());

    // if a bit in gray is 0, the corresponding value is on the left
    for(int cnt = (1 << (noVal-1)) - 1, gray = 0; cnt; cnt--) {
      int prevgray = gray;
      gray = cnt ^ (cnt >> 1);
      int graydiff = gray ^ prevgray;
      int diffed;
      for(diffed = 0; !(graydiff & 1); graydiff >>= 1, diffed++);

      if (gray > prevgray) { // something went to the right; subtract all the gains for being different from values on the right
        // prevgray = gray is not needed: they only differ in the bit representing this group
        for(gi = gains + diffed*noVal, ge = gi + noVal; gi != ge; thisScore += prevgray & 1 ? -*gi++ : *gi++, prevgray >>= 1);
        thisLeft -= attrDistr[diffed];
        thisRight += attrDistr[diffed];
      }
      else {
        // prevgray = gray is not needed: they only differ in the bit representing this group
        for(gi = gains + diffed*noVal, ge = gi + noVal; gi != ge; thisScore += prevgray & 1 ? *gi++ : -*gi++, prevgray >>= 1);
        thisLeft += attrDistr[diffed];
        thisRight -= attrDistr[diffed];
      }

      if (   (thisLeft >= minSubset) && (thisRight >= minSubset)
          && (   (!wins || (thisScore > bestScore)) && ((wins=1) == 1)
              || (thisScore == bestScore) && rgen.randbool(++wins))) {
        bestScore = thisScore;
        bestSubset = gray;
        bestLeft = thisLeft;
        bestRight = thisRight;
      }
    }

    delete gains;
    gains = NULL;

    if (!wins || !bestSubset) {
      delete attrDistr;
      return false;
    }
    
    ai = attrDistr;
    TIntList *rightSide = new TIntList();
    for(i = noVal; i--; bestSubset = bestSubset >> 1, ai++)
      rightSide->push_back(*ai > 0 ? bestSubset & 1 : -1);

    delete attrDistr;
    attrDistr = NULL;

    subsetSizes = new TDiscDistribution(2);
    subsetSizes->addint(0, bestLeft);
    subsetSizes->addint(1, bestRight);

    return rightSide;
  }
  catch (...) {
    if (gains)
      delete gains;
    if (attrDistr)
      delete attrDistr;
    throw;
  }
}



int TScoreFeature_relief::bestValue(
    PDistribution &subsetSizes, double &bestScore,
    PVariable const &var,
    PExampleTable const &gen,
    const double &minSubset)
{
  TDiscreteVariable *evar = var.AS(TDiscreteVariable);
  if (!evar)
    raiseError("cannot discretly binarize a continuous attribute");

  const int noVal = evar->noOfValues();

  float *attrDistr = NULL;
  PSymMatrix wgain = gainMatrix(var, gen, apriorClass, NULL, &attrDistr);
  TSymMatrix &gain = wgain.getReference();

  float *gains = new float[noVal * noVal], *gi = gains, *ge;

  int wins = 0;

  try {
    float thisScore = 0.0;
    int i, j;
    for(i = 0; i < noVal; i++)
      for(j = 0; j < noVal; j++)
        *gi++ = gain.getitem(i, j);

    float *ai, *ae;
    float nExamples;
    if (!attrDistr) {
      TDiscDistribution dd(gen, var);
      attrDistr = new float[noVal];
      ai = attrDistr;
      ae = attrDistr + noVal;
      for(vector<float>::const_iterator di(dd.distribution.begin()); ai != ae; *ai++ = *di++);
      nExamples = dd.abs;
    }
    else {
      nExamples = 0;
      for(ai = attrDistr, ae = attrDistr + noVal; ai != ae; nExamples += *ai++);
    }   
   
    float maxSubset = nExamples - minSubset;
    if (maxSubset < minSubset)
      return -1;

    int bestVal = -1;
    wins = 0;
    bestScore = 0;
    TRandomGenerator rgen(gen->numberOfExamples());
    float *gi = gains;
    ai = attrDistr;
    for(int thisValue = 0; thisValue < noVal; thisValue++, ai++) {
      if ((*ai < minSubset) || (*ai > maxSubset)) {
        gi += noVal;
        continue;
      }
      
      float thisScore = -2*gi[thisValue]; // have to subtract this, we'll add it once below
      for(ge = gi + noVal; gi != ge; thisScore += *gi++);
      if (    (!wins || (thisScore > bestScore)) && ((wins=1) == 1)
          || (thisScore == bestScore) && rgen.randbool(++wins)) {
        bestScore = thisScore;
        bestVal = thisValue;
      }
    }

    delete gains;
    gains = NULL;

    if (!wins) {
      delete attrDistr;
      return -1;
    }
    
    subsetSizes = new TDiscDistribution(2);
    subsetSizes->addint(0, nExamples - attrDistr[bestVal]);
    subsetSizes->addint(1, attrDistr[bestVal]);

    delete attrDistr;
    attrDistr = NULL;

    return bestVal;
  }
  catch (...) {
    if (gains)
      delete gains;
    if (attrDistr)
      delete attrDistr;
    throw;
  }
}

*/


char *ScoreFeature_keywords_exampletable[] = {"feature", "data", NULL};
char *ScoreFeature_keywords_domaincontingency[] = {"feature", "domain_contingency", NULL};
char *ScoreFeature_keywords_contingency[] = {"contingency", NULL};

bool TScoreFeature::getPyArgs(
    PyObject *args, PyObject *kw,
    PVariable &var, PExampleTable &data,
    PContingency &cont)
{
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&",
        ScoreFeature_keywords_contingency,
        &PContingency::argconverter, &cont)) {
            return true;
    }
    PyErr_Clear();

    PyObject *pyvar;
    PDomainContingency domcont;
    if (PyArg_ParseTupleAndKeywords(args, kw, "OO&",
        ScoreFeature_keywords_domaincontingency,
        &pyvar, &PDomainContingency::argconverter, &domcont)) {
            TAttrIdx const aidx = domcont->getItemIndex(pyvar);
            cont = domcont->contingencies[aidx];
            return true;
    }
    PyErr_Clear();

    if (PyArg_ParseTupleAndKeywords(args, kw, "OO&",
        ScoreFeature_keywords_exampletable,
        &pyvar, &PExampleTable::argconverter, &data)) {
            var = data->domain->getVar(pyvar, false, true, false);
            return true;
    }

    return false; // error set by the last PyArg_ParseTupleAndKeywords
}


PyObject *TScoreFeature::__call__(PyObject *args, PyObject *kw)
{
    PVariable var;
    PExampleTable data;
    PContingency contingency;
    if (!getPyArgs(args, kw, var, data, contingency)) {
        return NULL;
    }
    double score = data ? (*this)(var, data) : (*this)(contingency);
    return PyFloat_FromDouble(score);
}


PyObject *TScoreFeature::threshold_function(PyObject *args, PyObject *kw)
{
    PVariable var;
    PExampleTable data;
    PContingency contingency;
    if (!getPyArgs(args, kw, var, data, contingency)) {
        return NULL;
    }
    vector<pair<double, double> > res;
    if (data) {
        thresholdFunction(res, var, data);
    }
    else {
        thresholdFunction(res, contingency);
    }
    PyObject *f = PyList_New(res.size());
    Py_ssize_t i = 0;
    for(vector<pair<double, double> >::const_iterator
        ii(res.begin()), ie(res.end()); ii != ie; ii++, i++) {
            PyList_SetItem(f, i, Py_BuildValue("(dd)", ii->first, ii->second));
    }
    return f;
}


PyObject *TScoreFeature::best_threshold(PyObject *args, PyObject *kw)
{
    PVariable var;
    PExampleTable data;
    PContingency contingency;
    if (!getPyArgs(args, kw, var, data, contingency)) {
        return NULL;
    }
    double threshold, score;
    PDistribution dist;
    if (data) {
        threshold = bestThreshold(dist, score, var, data);
    }
    else {
        threshold = bestThreshold(dist, score, contingency);
    }
    return Py_BuildValue("ddO",
        threshold, score, dist ? dist.borrowPyObject() : Py_None);
}