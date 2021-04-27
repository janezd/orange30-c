/*
Copyright (c) 2007-2008 The LIBLINEAR Project.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither name of copyright holders nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef LIBLINEAR_INTERFACE_HPP
#define LIBLINEAR_INTERFACE_HPP

#include "common.hpp"
#include "liblinear_interface.ppp"

#include "liblinear/linear.h"


// Alternative model save/load routines (using iostream, needed for in memory serialization)
int linear_save_model_alt(string &, model *);
model *linear_load_model_alt(string &);


class TLinearLearner : public TLearner {
public:
	__REGISTER_CLASS(LinearLearner);
    NEW_WITH_CALL(Learner);
	
	enum Lossfunction1 {L2_LR, L2Loss_SVM_Dual, L2Loss_SVM, L1Loss_SVM_Dual } PYCLASSCONSTANTS_UP;
	
	int solver_type;	//P(&Lossfunction1) Solver type (loss function1)
	double eps;			//P Stopping criteria
	double C;			//P Regularization parameter

	TLinearLearner();
	PClassifier operator()(PExampleTable const &);
};


class TLinearClassifier : public TClassifier {
public:
	__REGISTER_CLASS(LinearClassifier);

	TLinearClassifier(PDomain const &);

	TLinearClassifier(
        PVariable const &var, PExampleTable const &examples,
        model *_model, map<int, int> *indexMap=NULL);

	~TLinearClassifier();

	PDistribution classDistribution(TExample const *const);
	TValue operator()(TExample const *const);

	PFloatListList weights;	//P Computed feature weights
	PExampleTable examples;	//P Examples used to train the classifier

	inline model *getModel() { return linmodel; }
private:
	model *linmodel;
	map<int, int> *indexMap;

public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(class_var, data, model_string)");
    PyObject *__getnewargs__() PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
};


#endif /* LINEAR_HPP */