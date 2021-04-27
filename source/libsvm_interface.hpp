/*
Copyright (c) 2000-2007 Chih-Chung Chang and Chih-Jen Lin
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


#ifndef __LIBSVM_INTERFACE_HPP
#define __LIBSVM_INTERFACE_HPP

#include "libsvm_interface.ppp"
#include "libsvm/svm.h"

svm_model *svm_load_model_alt(string& buffer);
int svm_save_model_alt(string& buffer, const svm_model *model);

class TKernelFunc: public TOrange{
public:
	__REGISTER_ABSTRACT_CLASS(KernelFunc);

	virtual double operator()(const TExample &, const TExample &)=0;
};


class TSVMLearner : public TLearner{
public:
	__REGISTER_CLASS(SVMLearner);
    NEW_WITH_CALL(Learner);

    enum SVMType {C_SVC=::C_SVC, Nu_SVC=NU_SVC, OneClass=ONE_CLASS, Epsilon_SVR=EPSILON_SVR, Nu_SVR=NU_SVR } PYCLASSCONSTANTS_UP;
    enum Kernel {Linear=LINEAR, Polynomial=POLY, RBF=::RBF, Sigmoid=SIGMOID, Custom=PRECOMPUTED } PYCLASSCONSTANTS_UP;

	//parameters
	int svm_type; //P(&SVMType)  SVM type (C_SVC=0, NU_SVC, ONE_CLASS, EPSILON_SVR=3, NU_SVR=4)
	int kernel_type; //P(&Kernel)  kernel type (LINEAR=0, POLY, RBF, SIGMOID, CUSTOM=4)
	double degree;	//P polynomial kernel degree
	double gamma;	//P poly/rbf/sigm parameter
	double coef0;	//P poly/sigm parameter
	double cache_size; //P cache size in MB
	double eps;	//P stopping criteria
	double C;	//P for C_SVC and C_SVR
	double nu;	//P for NU_SVC and ONE_CLASS
	double p;	//P for C_SVR
	int shrinking;	//P shrinking
	int probability;	//P probability
	bool verbose;		//P verbose

	int nr_weight;		/* for C_SVC */
	int *weight_label;	/* for C_SVC */
	double* weight;		/* for C_SVC */

	PKernelFunc kernelFunc;	//P custom kernel function

	PExampleTable tempExamples;

	TSVMLearner();
	~TSVMLearner();

	PClassifier operator()(PExampleTable const &);

protected:
	virtual svm_node* example_to_svm(const TExample &ex, svm_node* node, double last=0.0, int type=0);
	virtual int getNumOfElements(PExampleTable const &examples);
	virtual svm_node* init_problem(svm_problem &problem, PExampleTable const &examples, int n_elements);
    virtual TSVMClassifier* createClassifier(PVariable const &var, PExampleTable const &ex, svm_model* model, svm_node* x_space);

public:
    PyObject *setWeights(PyObject* args, PyObject *keywords) PYARGS(METH_BOTH, "(weights) -> None");

};

class TSVMLearnerSparse : public TSVMLearner{
public:
	__REGISTER_CLASS(SVMLearnerSparse);
    NEW_WITH_CALL(Learner);

	bool useNonMeta; //P include non meta attributes in the learning process
protected:
	virtual svm_node* example_to_svm(const TExample &ex, svm_node* node, double last=0.0, int type=0);
	virtual int getNumOfElements(PExampleTable const &examples);
	virtual TSVMClassifier* createClassifier(PVariable const &var, PExampleTable const &ex, svm_model* model, svm_node* x_space);
};


class TSVMClassifier : public TClassifier {
public:
	__REGISTER_CLASS(SVMClassifier)

	TSVMClassifier(PDomain const &domain)
    : TClassifier(domain),
      model(NULL),
      x_space(NULL)
    {};

//	TSVMClassifier(PDomain const &domain, svm_model *model);

    TSVMClassifier(const PVariable & , PExampleTable const &,
                   svm_model*, svm_node*, PKernelFunc const &);
	~TSVMClassifier();

	TValue operator()(TExample const *const);
	PDistribution classDistribution(TExample const *const);

	PFloatList getDecisionValues(const TExample &);

	PIntList nSV; //P nSV
	PFloatList rho;	//P rho
	PFloatListList coef; //P coef
	PFloatList probA; //P probA - pairwise probability information
	PFloatList probB; //P probB - pairwise probability information
	PExampleTable supportVectors; //P support vectors
	PExampleTable examples;	//P examples used to train the classifier
	PKernelFunc kernelFunc;	//P custom kernel function

	int svm_type; //P(&SVMLearner::SVMType)  SVM type (C_SVC=0, NU_SVC, ONE_CLASS, EPSILON_SVR=3, NU_SVR=4)
	int kernel_type; //P(&SVMLearner::Kernel)  kernel type (LINEAR=0, POLY, RBF, SIGMOID, CUSTOM=4)

    svm_model* getModel() {return model;}

protected:
	virtual svm_node* example_to_svm(const TExample &ex, svm_node* node, double last=0.0, int type=0);
	virtual int getNumOfElements(const TExample& example);

private:
	svm_model *model;
	svm_node *x_space;

public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("unpickle a model");
    PyObject *__getnewargs__() PYARGS(METH_NOARGS, "prepare data for unpickling");
    PyObject *py_getDecisionValues(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(instance)");
    PyObject *py_getModel() PYARGS(METH_NOARGS, "return a model as a string");

};

class TSVMClassifierSparse : public TSVMClassifier{
public:
	__REGISTER_CLASS(SVMClassifierSparse)

        bool useNonMeta; //P include non meta attributes

/*	TSVMClassifierSparse(PDomain const &domain)
    : TSVMClassifier(domain)
    {};

	TSVMClassifierSparse(PDomain const &domain, svm_model *mod)
    : TSVMClassifier(domain, mod)
    {};
    */
	TSVMClassifierSparse(PVariable const &var, PExampleTable const &ex,
                         svm_model* model, svm_node* x_space, bool _useNonMeta,
                         PKernelFunc const &kernelFunc)
    : TSVMClassifier(var, ex, model, x_space, kernelFunc),
      useNonMeta(_useNonMeta)
    {}

protected:
	virtual svm_node* example_to_svm(const TExample &ex, svm_node* node, double last=0.0, int type=0);
	virtual int getNumOfElements(const TExample& example);

public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("unpickle a model");
    PyObject *__getnewargs__() PYARGS(METH_NOARGS, "prepare data for unpickling");
};

#endif

