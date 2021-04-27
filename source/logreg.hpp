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

#ifndef __LOGREG_HPP
#define __LOGREG_HPP

#include "logreg.ppp"
#include "imputation.hpp"

class TLogRegClassifier : public TClassifier {
public:
	__REGISTER_CLASS(LogRegClassifier);

    PDomain continuizedDomain; //P if absent, there is no continuous attributes in original domain
    PImputer imputer; //P if present, it imputes unknown values

	PAttributedFloatList beta; //P estimated beta coefficients for logistic regression
	PAttributedFloatList beta_se; //P estimated standard errors for beta coefficients
	PAttributedFloatList wald_Z; //P Wald Z statstic for beta coefficients
	PAttributedFloatList P; //P estimated significances for beta coefficients
	double likelihood; //P Likelihood: The likelihood function is the function which specifies the probability of the sample observed on the basis of a known model, as a function of the model's parameters. 
	int fit_status; //P Tells how the model fitting ended - either regularly (LogRegFitter.OK), or it was interrupted due to one of beta coefficients escaping towards infinity (LogRegFitter.Infinity) or since the values didn't converge (LogRegFitter.Divergence).

	TLogRegClassifier(PDomain const &);
	PDistribution classDistribution(TExample const *const);
};


#endif
