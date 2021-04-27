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

#ifndef __LOGREGFITTER_HPP
#define __LOGREGFITTER_HPP

#include "logregfitter.ppp"


// abstract class for LR fitters. 
// New fitters should be derived from this one
class TLogRegFitter : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(LogRegFitter);

    // Don't change the order (<= Divergence means that model is fitted, > means error)
    enum ErrorCode {OK, Infinity, Divergence, Constant, Singularity} PYCLASSCONSTANTS_UP;

    virtual PAttributedFloatList operator()(PExampleTable const &,
        PAttributedFloatList &, double &, int &, PVariable &)=0;

    virtual void constructMatrices(PExampleTable const &,
        double **&X, double *&Y, double *&trials);

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data); fit betas to data; return (status, beta, beta_se, likelihood) or (status, variable) in case of error");
};


// Logistic regression fitter via minimization of log-likelihood
// orange integration of Aleks Jakulin version of LR
// based on Alan Miller's(1992) F90 logistic regression code
class TLogRegFitter_Cholesky : public TLogRegFitter {
public:
  __REGISTER_CLASS(LogRegFitter_Cholesky);
  NEW_WITH_CALL(LogRegFitter);

/*  int maxit; //maximum no. iterations
  double offset; //offset on the logit scale
  double tol; //  tolerance for matrix singularity
  double eps; //difference in `-2  log' likelihood for declaring convergence.
  double penalty; //penalty (scalar), substract from ML beta' penalty beta. Set if
     //model doesnt converge */

  TLogRegFitter_Cholesky();
  TLogRegFitter_Cholesky(bool showErrors);

  virtual PAttributedFloatList operator()(PExampleTable const &,
      PAttributedFloatList &, double &, int &, PVariable &);

private:
  static const char *errors[];
};

#endif
