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
#include "calibrate.hpp"
#include "bayes.px"


TBayesLearner::TBayesLearner() 
: loessPoints(50),
  loessWindowProportion(0.5),
  loessDistributionMethod(TBayesLearner::Fixed),
  estimatorConstructor(new TProbabilityEstimatorConstructor_relative()),
  conditionalEstimatorConstructor(new TProbabilityEstimatorConstructor_relative()),
  adjustThreshold(false)  
{}


TBayesLearner::TBayesLearner(const TBayesLearner &old)
: TLearner(old),
  loessPoints(old.loessPoints),
  loessDistributionMethod(old.loessDistributionMethod),
  estimatorConstructor(old.estimatorConstructor),
  conditionalEstimatorConstructor(old.conditionalEstimatorConstructor),
  adjustThreshold(old.adjustThreshold)
{}


PClassifier TBayesLearner::operator()(PExampleTable const &gen)
{ 
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if (gen->domain->classVar->varType == TVariable::Continuous) {
        raiseError(PyExc_ValueError, "Bayes learner does not handle regression problems");
    }
    PDomainContingency stat(new TDomainContingency(gen));
    PProbabilityEstimator priorEstimator =
        (*estimatorConstructor)(stat->classes, PDistribution());
    PITERATE(TDomainContingency, di, stat) {
        if ((*di)->varType() == TVariable::Discrete) {
            ITERATE(TDistributionVector, ddi, *(*di)->discrete) {
                *ddi = (*conditionalEstimatorConstructor)(*ddi, stat->classes)
                    ->probabilities;
            }
        }
        else {
            *di = loessSmoothing(PContingencyAttrClass(*di));
        }
    }
    PDiscDistribution prior(priorEstimator->probabilities);
    PBayesClassifier classifier(new (getReturnType(&OrBayesClassifier_Type))
        TBayesClassifier(gen->domain, prior, stat));
    if (adjustThreshold && (gen->domain->classVar->noOfValues() == 2)) {
        double optCA;
        classifier->threshold = findOptimalCAThreshold(classifier, gen, optCA);
    }
    return classifier;
}


TBayesClassifier::TBayesClassifier(PDomain const &domain) 
: TClassifier(domain),
threshold(0.5)
{}


TBayesClassifier::TBayesClassifier(PDomain const &dom,
                                   PDiscDistribution const &dist,
                                   PDomainContingency const &dcont,
                                   const double thresh)
: TClassifier(dom),
  distribution(dist),
  conditionalDistributions(dcont),
  threshold(thresh)
{}


PDistribution TBayesClassifier::classDistribution(TExample const *const origexam)
{ 
    checkProperty(domain);
    PExample exam = origexam->convertedTo(domain, false);
    PDiscDistribution result = CLONE(PDiscDistribution, distribution);
    result->normalize();
    PDiscDistribution classDistDiv = CLONE(PDiscDistribution, distribution);
    PITERATE(TDiscDistribution, ci, classDistDiv) {
        *ci = *ci < 1e-20 ? 1 : 1 / *ci;
    }
    TDomainContingency::const_iterator dci(conditionalDistributions->begin());
    TDomainContingency::const_iterator dce(conditionalDistributions->end());
    for (TExample::const_iterator ei(exam->begin()); dci!=dce; ei++, dci++) {
        if (!isnan(*ei)) {
            // If we have a contingency, that's great
            PDistribution cp = (*dci)->p(*ei);
            if (cp->cases > 1e-6) {
                *result *= *cp;
                *result *= *classDistDiv;
                result->normalize(); // or should we do this outside?!?!?!
            }
        }
    }
    if (result->abs == numeric_limits<double>::infinity()) {
        for(TDiscDistribution::iterator di(result->begin()), de(result->end());
            di != de; di++) {
                *di = *di==numeric_limits<double>::infinity() ? 1.0 : 0.0;
        }
    }
    return result;
}


TValue TBayesClassifier::operator ()(TExample const *const exam)
{ 
    if (classVar->noOfValues() == 2) {
        return TValue(classDistribution(exam)->at(1) >= threshold ? 1 : 0);
    }
    else {
        return classDistribution(exam)->highestProbValue(exam);
    }
}

void TBayesClassifier::predictionAndDistribution(
    TExample const *const ex, TValue &val, PDistribution &classDist)
{ 
    classDist = classDistribution(ex);
    if (classVar->noOfValues() == 2) {
        val = TValue(classDist->at(1) >= threshold ? 1 : 0);
    }
    else {
        val = classDist->highestProbValue(ex);
    }
}


TOrange *TBayesClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PDomain domain;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
        "O&:BayesClassifier", BayesClassifier_keywords,
        &PDomain::argconverter, &domain)) {
        return NULL;
    }
    return new(type) TBayesClassifier(domain);
}



static inline void copyFromDist(double *dst, TDistribution const &dist)
{
    TDiscDistribution const &ddist = dynamic_cast<TDiscDistribution const &>(dist);
    copy(ddist.begin(), ddist.end(), dst);
}

static inline void addVector(double *dst, double *src, int size)
{
    for(; size--; *dst++ += *src++);
}

static inline void addMulVector(double *dst, double *src, double m, int size)
{
    for(; size--; *dst++ += *src++ * m);
}

static inline void mulScalar(double *dst, const double m, int size)
{
    for(; size--; *dst++ *= m);
}

static inline void mulVector(double *dst, const double *m, int size)
{
    for(; size--; *dst++ *= *m++);
}

static inline void sqr(double *dst, int size)
{
    for(; size--; dst++) {
        *dst *= *dst;
    }
}

static inline void mulSqrScalar(double *dst, const double m, int size)
{
    for(; size--; dst++) {
        *dst = *dst * *dst * m;
    }
}

static inline void mulSqrVector(double *dst, const double *m, int size)
{
    for(; size--; dst++, m++) {
        *dst = *dst * *dst * *m;
    }
}


PContingencyAttrClass TBayesLearner::loessSmoothing(
    PContingencyAttrClass const &frequencies)
{ 
    PVariable innerVar = frequencies->innerVariable;
    if (!innerVar) {
        raiseError(PyExc_ValueError, "inner variable is not set");
    }
    if (innerVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError,
            "variable '%s' is not discrete", innerVar->cname());
    }
    if (frequencies->varType() != TVariable::Continuous) {
        raiseError(PyExc_ValueError,
            "variable '%s' (condition) is not continuous",
            frequencies->outerVariable ? frequencies->outerVariable->cname()
            : "");
    }
    if (!frequencies->continuous->size()) {
        raiseError(PyExc_ValueError,
            "distribution (of attribute values, probably) is empty or "
            "has only a single value");
    }

    const int nClasses = innerVar->noOfValues();
    PContingencyAttrClass cont(CLONE(PContingencyAttrClass, frequencies));
    cont->continuous->clear();
    const TDistributionMap &points = *frequencies->continuous;
    const TDistributionMap::const_iterator lowedge = points.begin();
    const TDistributionMap::const_iterator highedge = points.end();

    vector<double> xpoints;
    distributePoints(points, loessPoints, xpoints, loessDistributionMethod);
    if (!xpoints.size()) {
        raiseError(PyExc_ValueError, "no sampling points (check 'nPoints')");
    }

    if (frequencies->continuous->size() == 1) {
        PDiscDistribution f = CLONE(PDiscDistribution, lowedge->second);
        f->normalize();
        f->variances = PFloatList(new TFloatList(f->size(), 0));
        const_ITERATE(vector<double>, pi, xpoints) {
            (*cont->continuous)[*pi] = f;
        }
        return cont;
    }

    bool needAll;
    double totalNumOfPoints = frequencies->outerDistribution->abs;
    int needpoints = int(ceil(totalNumOfPoints * loessWindowProportion));
    if (needpoints<3) {
        needpoints = 3;
    }

    TSimpleRandomGenerator rgen(frequencies->outerDistribution->cases);
    TDistributionMap::const_iterator from, to;
    vector<double>::const_iterator pi(xpoints.begin()), pe(xpoints.end());
    double refx = *pi;

    if ((needpoints <= 0) || (needpoints >= totalNumOfPoints)) {  //points.size()
        needAll = true;
        from = lowedge;
        to = highedge;
    }
    else {
        needAll = false;

        /* Find the window */
        from = points.lower_bound(refx);
        to = points.upper_bound(refx);
        if (from==to)
            if (to != highedge)
                to++;
            else
                from --;

        /* Extend the interval; we set from to highedge when it would go beyond lowedge, to indicate that only to can be modified now */
        while (needpoints > 0) {
            if ((to == highedge) ||
                ((from != highedge) && (refx - (*from).first < (*to).first - refx))) {
                if (from == lowedge)
                    from = highedge;
                else {
                    from--;
                    needpoints -= (*from).second->cases;
                }
            }
            else {
                to++;
                if (to!=highedge)
                    needpoints -= (*to).second->cases;
                else
                    needpoints = 0;
            }

        }

        if (from == highedge)
            from = lowedge;
        /*    else
        from++;*/
    }

    const size_t memsze = nClasses * sizeof(double);
    double *Sy = (double *)malloc(memsze);
    double *Syy = (double *)malloc(memsze);
    double *Sxx = (double *)malloc(memsze);
    double *Sxy = (double *)malloc(memsze);
    double *ty = (double *)malloc(memsze);
    try {
        int numOfOverflowing = 0;
        // This follows http://www-2.cs.cmu.edu/afs/cs/project/jair/pub/volume4/cohn96a-html/node7.html
        for(;;) {
            TDistributionMap::const_iterator tt = to;
            --tt;

            if (tt == from) {
                PDiscDistribution mSy = CLONE(PDiscDistribution, tt->second);
                mSy->normalize();
                mSy->variances = PFloatList(new TFloatList(mSy->size(), 0.0));
                (*cont->continuous)[refx] = mSy;
            }
            else {
                double h = (refx - (*from).first);
                if ((*tt).first - refx  >  h) {
                    h = ((*tt).first - refx);
                }
                /* Iterate through the window */
                tt = from;
                const double &x = (*tt).first;
                TDistribution const *const y = (*tt).second.borrowPtr();
                double cases = y->abs;

                double w = fabs(refx - x) / h;
                w = 1 - w*w*w;
                w = w*w*w;

                const double num = y->abs; // number of instances with this x - value
                double n = w * num;
                double Sww = sqr(w) * num;

                double Sx = w * x * num;
                double Sxx = x * Sx;
                double Swwx  = x * Sww;
                double Swwxx = sqr(x) * Sww;

                copyFromDist(Sy, *tt->second);
                memcpy(Syy, Sy, memsze);
                memcpy(Sxy, Sy, memsze);
                mulScalar(Sy, w, nClasses);
                mulSqrScalar(Syy, w, nClasses);
                mulScalar(Sxy, w*x, nClasses);
                if (tt!=to) {
                    while (++tt != to) {
                        const double &x = tt->first;
                        const TDistribution *const y = tt->second.borrowPtr();
                        cases += y->abs;
                        w = fabs(refx - x) / h;
                        w = 1 - w*w*w;
                        w = w*w*w;

                        const double num = y->abs;
                        n   += w * num;
                        Sww += sqr(w) * num;
                        Sx  += w * x * num;
                        Swwx += sqr(w) * x * num;
                        Swwxx += sqr(w)*sqr(x) * num;
                        Sxx += w * sqr(x) * num;

                        copyFromDist(ty, *y);
                        mulScalar(ty, w, nClasses);
                        addVector(Sy, ty, nClasses);
                        addMulVector(Sxy, ty, x, nClasses);

                        copyFromDist(ty, *y);
                        mulSqrScalar(ty, w, nClasses);
                        addVector(Syy, ty, nClasses);
                    }
                    double sigma_x2 = n<1e-6 ? 0.0 : (Sxx - Sx * Sx / n)/n;
                    if (sigma_x2 < 1e-10) {
                        (*cont->continuous)[refx] = 
                            PDiscDistribution(new TDiscDistribution(innerVar));
                    }
                    else {
                        const double difx = refx - Sx/n;
                        for(int i = 0; i < nClasses; i++) {
                            double &Syi = Sy[i];
                            const double sigma_y2 = (Syy[i] - sqr(Syi)/n) / n;
                            const double sigma_xy = (Sxy[i] - Sx*Syi/n) / n;
                            Syi = Syi / n * sigma_xy / sigma_x2 * difx;
// probabilities that are higher than 0.9 are normalized with logistic function, which produces two positive 
// effects: prevents overfitting and avoids probabilities that are higher than 1.0. But, on the other hand, this 
// solution is rather ad hoc. Do the same for probabilities that are lower than 0.1.
                            if (Syi > 0.9) {
                                Syi = 1/(1+exp(-10*(Syi-0.9)*log(9.0)-log(9.0)));
                            }
                            if (Syi < 0.1) {
                                Syi = 1/(1+exp(10*(0.1-Syi)*log(9.0)+log(9.0)));
                            }
                            ty[i] = (sigma_y2 - sqr(sigma_xy) /
                                sigma_x2) * (1 + sqr(difx)/sigma_x2);
                        }
                        PDiscDistribution ndist(new TDiscDistribution(Sy, nClasses));
                        ndist->variable = innerVar;
                        ndist->normalize();
                        ndist->cases = cases;
                        ndist->variances = PFloatList(new TFloatList(nClasses));
                        copy(ty, ty+nClasses, ndist->variances->begin());
                        (*cont->continuous)[refx] = ndist;
                    }
                }
            }

            // on to the next point
            pi++;
            if (pi==pe) {
                break; 
            }
            refx = *pi;
            // Adjust the window
            while (to!=highedge) {
                double dif = (refx - (*from).first) - ((*to).first - refx);
                if ((dif>0) || (dif==0) && rgen.randbool()) {
                    if (numOfOverflowing > 0) {
                        from++;
                        numOfOverflowing -= (*from).second->cases;
                    }
                    else {
                        to++;
                        if (to!=highedge) 
                            numOfOverflowing += (*to).second->cases;
                    }
                }
                else {
                    break;
                }
            }
        }
    }
    catch (...) {
        free(Sy);
        free(Syy);
        free(Sxx);
        free(Sxy);
        free(ty);
    }
    free(Sy);
    free(Syy);
    free(Sxx);
    free(Sxy);
    free(ty);
    return cont;
}
