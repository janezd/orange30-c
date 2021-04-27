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

#include "converts.hpp"
#include "logreg_funcs.hpp"
#include "logregfitter.px"

// output values computed in logistic fitter
class LRInfo {
public:
    LRInfo()
    : beta(NULL), se_beta(NULL), fit(NULL), cov_beta(NULL), stdres(NULL), dependent(NULL) {};
    ~LRInfo();

    int nn, k;
    double chisq;      // chi-squared
    double devnce;     // deviance
    int    ndf;        // degrees of freedom
    double *beta;      // fitted beta coefficients
    double *se_beta;   // beta std.devs
    double *fit;       // fitted probabilities for groups
    double **cov_beta; // approx covariance matrix
    double *stdres;    // residuals
    int    *dependent; // dependent/redundant variables
    int	  error;
};

// input values for logistic fitter
class LRInput {
public:
    LRInput() : data(NULL), success(NULL), trials(NULL) {}
    ~LRInput();

    int nn;
    int k;
    double **data;    //nn*k
    double *success;     //nn
    double *trials;
};


LRInput::~LRInput()
{
    int i;
    if (data) {
        for (i=0; i <= nn; ++i) {
            delete[] data[i];
        }
        delete[] data;
    }
    delete[] success;
    delete[] trials;
}


LRInfo::~LRInfo() {
    if (cov_beta) {
        for (int i = 0; i <= k; ++i) {
            delete cov_beta[i];
        }
        delete[] cov_beta;
    }
    delete[] fit;
    delete[] beta;
    delete[] se_beta;
    delete[] stdres;
    delete[] dependent;
}



PyObject *TLogRegFitter::__call__(PyObject *args, PyObject *kw) PYDOC("(examples[, weightID]) -/-> (status, beta, beta_se, likelihood) | (status, attribute)")
{
    PExampleTable data;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O&", LogRegFitter_call_keywords,
        &PExampleTable::argconverter_n, &data)) {
            return NULL;
    }
    PAttributedFloatList beta, beta_se;
    double likelihood;
    int error;
    PVariable attribute;
    beta = (*this)(data, beta_se, likelihood, error, attribute);
    PyObject *errObj = get_constantObject(error, LogRegFitter_ErrorCode_constList);
    if (error > TLogRegFitter::Divergence) {
       return Py_BuildValue("NN", errObj, attribute.getPyObject());
    }
    return Py_BuildValue("NNNd",
        errObj, beta.getPyObject(), beta_se.getPyObject(), likelihood);
}

TLogRegFitter_Cholesky::TLogRegFitter_Cholesky()
{}


// set error values thrown by logistic fitter
const char *TLogRegFitter_Cholesky::errors[] = {
    "LogRegFitter: ngroups < 2, ndf < 0 -- not enough examples with so many attributes",
    "LogRegFitter: n[i]<0",
    "LogRegFitter: r[i]<0",
    "LogRegFitter: r[i]>n[i]: Class has more that 2 values, please use only dichotomous class!",
    "LogRegFitter: constant variable",
    "LogRegFitter: singularity",
    "LogRegFitter: infinity in beta",
    "LogRegFitter: no convergence"
};


double *ones(int n) {
    double *ret = new double[n];
    for (int i=0; i<n; i++) {
        ret[i] = 1;
    }
    return ret;
}


PAttributedFloatList TLogRegFitter_Cholesky::operator ()(PExampleTable const &gen,
    PAttributedFloatList &beta_se, double &likelihood,
    int &error, PVariable &error_att)
{
    if (!gen->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    PVariable discattr = gen->domain->hasDiscreteAttributes();
    if (discattr) {
        raiseError(PyExc_TypeError,
            "variable '%s' is not continuous", discattr->cname());
    }

    LRInput input;
    input.nn = gen->size();
    input.k = gen->domain->attributes->size();
    constructMatrices(gen, input.data, input.success, input.trials);

    LRInfo O;
    O.nn = input.nn;
    O.k = input.k;
    O.beta = new double[input.k+1];
    O.se_beta = new double[input.k+1];
    O.fit = new double[input.nn+1]; 
    O.stdres = new double[input.nn+1]; 
    O.cov_beta = new double*[input.k+1]; 
    O.dependent = new int[input.k+1];
    int i;
    for(i = 0; i <= input.k; ++i) {
        O.cov_beta[i] = new double[input.k+1];
        O.dependent[i] = 0; // no dependence
    }

    logistic(O.error, input.nn,input.data,input.k,input.success,input.trials,
        O.chisq, O.devnce, O.ndf, O.beta, O.se_beta,
        O.fit, O.cov_beta, O.stdres, O.dependent
        );

    switch (O.error) {
        case 5: error = Constant; break;
        case 6: error = Singularity; break;
        case 7: error = Infinity; break;
        case 8: error = Divergence; break;
        default: error = OK;
    }
    // get variable that caused the error
    if ((O.error == 6) || (O.error == 5) || (O.error == 7)) {
        int i = 1;
        PITERATE(TVarList, vli, gen->domain->attributes) {
            if (O.dependent[i] == 1) {
                error_att = *vli;
                break;
            }
            i++;
        }
    }
    if (O.error>0 && O.error<5) {
        raiseError(PyExc_RuntimeError, errors[O.error-1]);
    }

    // create a new domain where class attributes is positioned at the beginning of 
    // the list. This is needed since beta coefficients start with beta0, representing
    // the intercept which is best colligated to class attribute. 
    PVarList enum_attributes(new TVarList()); 
    enum_attributes->push_back(gen->domain->classVar);
    PITERATE(TVarList, vl, gen->domain->attributes) {
        enum_attributes->push_back(*vl);
    }
    PAttributedFloatList beta(new TAttributedFloatList(enum_attributes));
    beta_se = PAttributedFloatList(new TAttributedFloatList(enum_attributes));
    for (i=0; i<input.k+1; i++) {
        beta->push_back(O.beta[i]);
        beta_se->push_back(O.se_beta[i]);
    }
    likelihood = - O.devnce;
    return beta;
}


void TLogRegFitter::constructMatrices(PExampleTable const &gen,
    double **&X, double *&Y, double *&trials)
{
    int const numExamples = gen->size();
    int const numAttr = gen->domain->attributes->size();
    X = new double*[numExamples+1];
    Y = new double[gen->size()+1];
    trials = new double[gen->size()+1];
    fill(X, X+numExamples, (double *)NULL);
    try {
        int n = 1;
        double **Xi = X, *Yi = Y, *ti = trials;
        *Xi++ = new double[numAttr+1];
        *Yi++ = 0;
        *ti++ = 0;
        if (gen->hasWeights()) {
            PEITERATE(ex, gen) {
                *Xi = new double[numAttr+1];
                copy(ex->begin(), ex->end_features(), (*Xi++)+1);
                *ti++ = ex.getWeight();
                *Yi++ = ex.getWeight() * ex.getClass();
            }
        }
        else {
            fill(ti, ti+gen->size(), 1.0);
            PEITERATE(ex, gen) {
                *Xi = new double[numAttr+1];
                copy(ex->begin(), ex->end_features(), (*Xi++)+1);
                *Yi++ = ex.getClass();
            }
        }
    }
    catch (...) {
        for(int i = 0; i<=numExamples; i++) {
            delete[] X[i];
        }
        delete[] X;
        delete[] Y;
        delete[] trials;
    }
}
