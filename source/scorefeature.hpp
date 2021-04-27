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

#ifndef __SCOREFEATURE_HPP
#define __SCOREFEATURE_HPP

#include "scorefeature.ppp"
#include "examplesdistance.hpp"

double getEntropy(double const *, size_t);
inline double getEntropy(TDiscDistribution const &);
double getEntropy(PContingency const &);

double getGini(double const *const, size_t const);
inline double getGini(TDiscDistribution const &);
double getGini(PContingency const &,
               PDiscDistribution const &caseWeights,
               double const classGini=0.0);



class TScoreFeature : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(ScoreFeature);

    static double const Rejected;
    PYCLASSCONSTANT_FLOAT(Rejected, TScoreFeature::Rejected);

    enum UnknownsTreatment {IgnoreUnknowns, ReduceByUnknowns} PYCLASSCONSTANTS;
    bool needsExamples; //P true if the measure cannot be computed from distributions alone
    bool handlesDiscrete; //P tells whether the measure can handle discrete attributes
    bool handlesContinuous; //P tells whether the measure can handle continuous attributes
    bool computesThresholds; //P tells whether the measure can compute threshold functions/maxima for continuous attributes
    int unknownsTreatment; //P(&UnknownsTreatment) treatment of unknown values

    TScoreFeature(
        bool const needsExamples,
        bool const handlesDiscrete,
        bool const handlesContinuous,
        bool const computesThresholds,
        int const unknownsTreatment);

    virtual double operator()(PContingency const &) const;
    virtual double operator()(PDistribution const &) const;
    virtual double operator()(TDiscDistribution const &) const;
    virtual double operator()(TContDistribution const &) const;

    // for most scorers, implementing one of these two is enough
    virtual double operator()(PContingency const &, PDiscDistribution const &) const;
    virtual double operator()(PContingency const &, double const var) const;

    // derived class that implements only one of the following two should implement the second
    virtual double operator()(TAttrIdx const, PExampleTable const &);
    virtual double operator()(PVariable const &, PExampleTable const &);

    template<class TRecorder>
    bool traverseThresholds(
        TRecorder &recorder, 
        PVariable &bvar,
        PContingency const &origContingency, 
        PDistribution const &classDistribution);

    virtual void thresholdFunction(
        vector<pair<double, double> > &res,
        PContingency const &);

    virtual void thresholdFunction(
        vector<pair<double, double> > &res,
        PVariable const &,
        PExampleTable const &);

    virtual double bestThreshold(
        PDistribution &, double &score,
        PContingency const &,
        double const minSubset = -1);

    virtual double bestThreshold(
        PDistribution &, double &score,
        PVariable const &,
        PExampleTable const &,
        double const minSubset = -1);

    virtual PIntList bestBinarization(
        PDistribution &, double &score,
        PContingency const &,
        double const minSubset = -1);

    virtual PIntList bestBinarization(
        PDistribution &, double &score,
        PVariable const &,
        PExampleTable const &, 
        double const minSubset = -1);

    virtual int bestValue(
        PDistribution &, double &score,
        PContingency const &,
        double const minSubset = -1);

    virtual int bestValue(
        PDistribution &, double &score,
        PVariable const &,
        PExampleTable const &, 
        const double minSubset = -1);

    virtual bool checkClassType(int const varType) const;
    virtual void checkClassTypeExc(int const varType) const;

    static bool getPyArgs(
        PyObject *args, PyObject *kw,
        PVariable &, PExampleTable &, 
        PContingency &);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(feature, data) or (feature, domain_contingency) or (contingency, class_distribution)");
    PyObject *threshold_function(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(feature, data) or (feature, domain_contingency) or (contingency, class_distribution)");
    PyObject *best_threshold(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(feature, data) or (feature, domain_contingency) or (contingency, class_distribution)");

protected:
    PContingency prepareBinaryCheat(
        PDistribution const &classDistribution,
        PContingency const &origContingency,
        PVariable &bvar,
        TDiscDistribution *&dis0, TDiscDistribution *&dis1,
        TContDistribution *&con0, TContDistribution *&con1);
};


class TScoreFeature_info : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_info);
    NEW_WITH_CALL(ScoreFeature);

    TScoreFeature_info(int const unkTreat = ReduceByUnknowns);
    virtual double operator()(TDiscDistribution const &) const;
    virtual double operator()
        (PContingency const &, PDiscDistribution const &) const;
};


class TScoreFeature_gainRatio : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_gainRatio);
    NEW_WITH_CALL(ScoreFeature);

    TScoreFeature_gainRatio(const int unkTreat = ReduceByUnknowns);
    virtual double operator()
        (PContingency const &, PDiscDistribution const &) const;
};


class TScoreFeature_gini : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_gini);
    NEW_WITH_CALL(ScoreFeature);

    TScoreFeature_gini(int const unkTreat = ReduceByUnknowns);
    virtual double operator()(TDiscDistribution const &) const;
    virtual double operator()(
        PContingency const &,
        PDiscDistribution const &) const;
};


class TScoreFeature_relevance : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_relevance);
    NEW_WITH_CALL(ScoreFeature);

    TScoreFeature_relevance(int const unkTreat = ReduceByUnknowns);

    virtual double operator()(
        PContingency const &,
        PDiscDistribution const &) const;

    double valueRelevance(
        TDiscDistribution const &dval,
        const TDiscDistribution &classDist) const;
};

/*
class TScoreFeature_cost: public TScoreFeature {
public:
    __R EGISTER_CLASS(ScoreFeature_cost)

    PCostMatrix cost; / / P cost matrix

    TScoreFeature_cost(PCostMatrix costs = PCostMatrix());

    virtual double operator()(PContingency const &, PDiscDistribution const) const;
  	double majorityCost(TDiscDistribution const &dval);
  	void majorityCost(TDiscDistribution const &dval, double &cost, TValue &cclass);
};
*/

class TScoreFeature_MSE : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_MSE)
    NEW_WITH_CALL(ScoreFeature);

    double m; //P m for m-estimate
    double priorVariance; //P prior variance for m-estimate

    TScoreFeature_MSE(int const unkTreat = ReduceByUnknowns);
    virtual double operator()(PContingency const &, double const var) const;
};


class TScoreFeature_relief : public TScoreFeature {
public:
    __REGISTER_CLASS(ScoreFeature_relief);
    NEW_WITH_CALL(ScoreFeature);

    double k; //P number of neighbours
    double m; //P number of reference examples
    bool checkCachedData; //P tells whether to check the checksum of the data before reusing the cached neighbours

    TScoreFeature_relief(int const ak=5, int const am=100);

    virtual double operator()(TAttrIdx const attrNo, PExampleTable const &data);
    virtual double operator()(PVariable const &var, PExampleTable const &);

    double compute_cont(TVariable &var, PExampleTable const &);
    double compute_disc(TVariable &var, PExampleTable const &);

    virtual void thresholdFunction(
        vector<pair<double, double> > &res,
        PVariable const &,
        PExampleTable const &);

    virtual double bestThreshold(
        PDistribution &, double &score,
        PVariable const &,
        PExampleTable const &,
        double const minSubset = -1);

    class TPairGain {
    public:
        float e1, e2, gain;
        inline TPairGain(double const ae1, double const ae2, double const again);
    };

    class TPairGainAdder : public vector<TPairGain>
    {
    public:
        inline void operator()(double const refVal, double const neiVal, double const gain);
    };

    template<class FAdder>
    void thresholdFunction(
        PVariable const &,
        PExampleTable const &,
        FAdder &adder, 
        TValue **attrVals = NULL);

    inline void pairGains(
        TPairGainAdder &gains,
        PVariable const &var,
        PExampleTable const &gen)
    { thresholdFunction(var, gen, gains); }

/*    PSymMatrix gainMatrix(
        PVariable const &var,
        PExampleTable const &gen,
        int **attrVals,
        double **attrDistr);

    PIntList bestBinarization(
        PDistribution &subsets,
        double &score,
        PVariable const &var,
        PExampleTable const &gen,
        double const minSubset = -1);

    int bestValue(
        PDistribution &subsetSizes,
        double &bestScore,
        PVariable const &var,
        PExampleTable const &gen,
        double const minSubset);
*/
    void reset();
    void prepareNeighbours(PExampleTable const &);
    void checkNeighbourhood(PExampleTable const &gen);

protected:
    class TNeighbourExample {
    public:
        int index;
        double weight;
        double weightEE;

        inline TNeighbourExample(int const i, double const w);
        inline TNeighbourExample(int const i, double const w, double const wEE);
    };

    class TReferenceExample {
    public:
        int index;
        vector<TNeighbourExample> neighbours;
        double nNeighbours;
        inline TReferenceExample(int const i = -1);
    };

    vector<double> scores;
    int prevExamples, prevK, prevM;
    unsigned int prevChecksum;

    // the first int the index of the reference example
    // the inner int-float pairs are indices of neighbours and the corresponding weights
    //   (all indices refer to storedExamples)
    vector<TReferenceExample> neighbourhood;
    PExampleTable storedExamples;
    PExamplesDistance distance;
    double ndC, m_ndC;

    class TFunctionAdder : public map<TValue, TValue>
    {
    public:
        inline void addGain(TValue const threshold, double const gain);
        inline void operator()(
            TValue const refVal, TValue const neiVal, TValue const gain);
    };

    double *extractContinuousValues(
        PExampleTable const &gen,
        TVariable &variable,
        TValue &min, TValue &max, TValue &avg, double &N);

    int *extractDiscreteValues(
        PExampleTable const &gen,
        TVariable &variable,
        double *&unk, double &bothUnk);
};
    


inline double getEntropy(TDiscDistribution const &cdist)
{
    return getEntropy(cdist.distribution, cdist.nValues);
}

inline double getGini(TDiscDistribution const &cdist)
{
    return getGini(cdist.distribution, cdist.nValues);
}



TScoreFeature_relief::TNeighbourExample::TNeighbourExample(
    int const i, double const w)
: index(i),
  weight(w)
{}


TScoreFeature_relief::TNeighbourExample::TNeighbourExample(
    int const i, double const w, double const wEE)
: index(i),
  weight(w),
  weightEE(wEE)
{}


TScoreFeature_relief::TReferenceExample::TReferenceExample(int const i)
: index(i),
  nNeighbours(0.0)
{}


TScoreFeature_relief::TPairGain::TPairGain(
    double const ae1, double const ae2, double const again)
: e1(ae1),
  e2(ae2),
  gain(again)
{}


void TScoreFeature_relief::TPairGainAdder::operator()(
    TValue const refVal, TValue const neiVal, double const gain)
{
    if (refVal < neiVal) {
        push_back(TPairGain(refVal, neiVal, gain));
    }
    else {
        push_back(TPairGain(neiVal, refVal, gain));
    }
}


void TScoreFeature_relief::TFunctionAdder::addGain(
    TValue const threshold, double const gain)
{
    iterator lowerBound = lower_bound(threshold);
    if ((lowerBound != end()) && (lowerBound->first == threshold)) {
        lowerBound->second += gain;
    }
    else {
        insert(lowerBound, make_pair(threshold, gain));
    }
}


void TScoreFeature_relief::TFunctionAdder::operator()(
    TValue const refVal, TValue const neiVal, double const gain)
{
    if (refVal < neiVal) {
        addGain(refVal, gain);
        addGain(neiVal, -gain);
    }
    else {
        addGain(neiVal, gain);
        addGain(refVal, -gain);
    }
}


// If attrVals is non-NULL and the values are indeed computed by the thresholdFunction, the caller is 
// responsible for deallocating the table!
template<class FAdder>
void TScoreFeature_relief::thresholdFunction(
    PVariable const &var, PExampleTable const &gen, 
    FAdder &adder, TValue **attrVals)
{
    if (var->varType != TVariable::Continuous) {
        raiseError(PyExc_TypeError,
            "cannot compute thresholds for non-continuous attribute '%s'",
            var->cname());
    }
    checkNeighbourhood(gen);
    TAttrIdx const attrIdx = gen->domain->getVarNum(var, false);
    bool const regression = gen->domain->classVar->varType == TVariable::Continuous;
    if (attrIdx != ILLEGAL_INT) {
        if (attrVals) {
            *attrVals = NULL;
        }
        TExamplesDistance_Relief rdistance =
            dynamic_cast<TExamplesDistance_Relief &>(*distance.borrowPtr());
        TExampleTable const &table = *gen.borrowPtr();
        adder.clear();
        ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
            TValue const refVal = table.at(rei->index, attrIdx);
            if (isnan(refVal)) {
                continue;
            }
            ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                TValue const neiVal = table.at(nei->index, attrIdx);
                if (isnan(neiVal)) {
                    continue;
                }
                double gain;
                double const attrDist = rdistance(attrIdx, refVal, neiVal);
                if (regression) {
                    double const dCdA = nei->weight * attrDist;
                    double const dA = nei->weightEE * attrDist;
                    gain = dCdA / ndC - (dA - dCdA) / m_ndC;
                }
                else {
                    gain = nei->weight * attrDist;
                }
                adder(refVal, neiVal, gain);
            }
        }
    }
    else { // attrIdx == ILLEGAL_INT
        if (!var->getValueFrom) {
            raiseError(PyExc_ValueError,
                "feature '%s' cannot be computed from other features",
                var->cname());
        }
        double avg, min, max, N;
        double  *precals =
            extractContinuousValues(gen, *var.borrowPtr(), min, max, avg, N);
        if (attrVals) {
            *attrVals = precals;
        }
        if ((min != max) && (N > 1e-6)) {
            try {
                double const nor = 1.0 / (min-max);
                adder.clear();
                ITERATE(vector<TReferenceExample>, rei, neighbourhood) {
                    TValue const refVal = precals[rei->index];
                    if (isnan(refVal)) {
                        continue;
                    }
                    ITERATE(vector<TNeighbourExample>, nei, rei->neighbours) {
                        TValue const &neiVal = precals[nei->index];
                        if (isnan(neiVal)) {
                            continue;
                        }
                        double gain;
                        double const attrDist = fabs(refVal - neiVal) * nor;
                        if (regression) {
                            double const dCdA = nei->weight * attrDist;
                            double const dC = nei->weightEE * attrDist;
                            gain = dCdA / ndC - (dC - dCdA) / m_ndC;
                        }
                        else {
                            gain = nei->weight * attrDist;
                        }
                        adder(refVal, neiVal, gain);
                    }
                }
            }
            catch (...) {
                if (!attrVals) {
                    delete precals;
                }
                throw;
            }
        }
        if (!attrVals) {
            delete precals;
        }
    }
}

#endif
