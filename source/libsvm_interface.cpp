/*
Copyright (c) 2000-2009 Chih-Chung Chang and Chih-Jen Lin
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

#ifdef _MSC_VER
  #pragma warning (disable : 4786 4114 4018 4267 4244)
  #pragma warning (disable : 4512) // assigment operator could not be generated
#endif

/* CHANGES TO LIBSVM-2.89
 *-#include "svm.h" to #include"svm.ppp"
 *-commented the swap function definition due to conflict with std::swap
 *-added #ifdef around min max definitions
 *-added examples, classifier and learner pointers to svm_parameter
 *-added svm_parameter member to Kernel
 *-added kernel_custom method to Kernel
 *-added code to compute custom kernel in Kernel::k_function
 *-handle CUSTOM in svm_check_parameter
 *-moved svm_model definition into svm.hpp
 *-added custom to kernel_type_table
/*##########################################
##########################################*/


#include "common.hpp"

#include <iostream>
#include <sstream>

#include "libsvm_interface.px"

// Defined in svm.cpp. If new svm or kernel types are added this should be updated.

static const char *svm_type_table[] =
{
	"c_svc","nu_svc","one_class","epsilon_svr","nu_svr",NULL
};

static const char *kernel_type_table[]=
{
	"linear","polynomial","rbf","sigmoid","precomputed",NULL
};

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

/*
 * Save load functions for use with orange pickling.
 * They are a copy of the standard libSVM save-load functions
 * except that they read/write from/to std::iostream objects.
 */

int svm_save_model_alt(std::ostream& stream, const svm_model *model){
	const svm_parameter& param = model->param;
	stream.precision(17);

	stream << "svm_type " << svm_type_table[param.svm_type] << endl;
	stream << "kernel_type " << kernel_type_table[param.kernel_type] << endl;

	if(param.kernel_type == POLY)
		stream << "degree " << param.degree << endl;

	if(param.kernel_type == POLY || param.kernel_type == RBF || param.kernel_type == SIGMOID)
		stream << "gamma " << param.gamma << endl;

	if(param.kernel_type == POLY || param.kernel_type == SIGMOID)
		stream << "coef0 " << param.coef0 << endl;

	int nr_class = model->nr_class;
	int l = model->l;
	stream << "nr_class " << nr_class << endl;
	stream << "total_sv " << l << endl;
	{
		stream << "rho";
		for(int i=0;i<nr_class*(nr_class-1)/2;i++)
			stream << " " << model->rho[i];
		stream << endl;
	}

	if(model->label)
	{
		stream << "label";
		for(int i=0;i<nr_class;i++)
			stream << " " << model->label[i];
		stream << endl;
	}

	if(model->probA) // regression has probA only
	{
		stream << "probA";
		for(int i=0;i<nr_class*(nr_class-1)/2;i++)
			stream << " " << model->probA[i];
		stream << endl;
	}
	if(model->probB)
	{
		stream << "probB";
		for(int i=0;i<nr_class*(nr_class-1)/2;i++)
			stream << " " << model->probB[i];
		stream << endl;
	}

	if(model->nSV)
	{
		stream << "nr_sv";
		for(int i=0;i<nr_class;i++)
			stream << " " << model->nSV[i];
		stream << endl;
	}

	stream << "SV" << endl;
	const double * const *sv_coef = model->sv_coef;
	const svm_node * const *SV = model->SV;

	for(int i=0;i<l;i++)
	{
		for(int j=0;j<nr_class-1;j++)
			stream << sv_coef[j][i] << " ";

		const svm_node *p = SV[i];

		if(param.kernel_type == PRECOMPUTED)
			stream << "0:" << (int)(p->value) << " ";
		else
			while(p->index != -1)
			{
				stream << (int)(p->index) << ":" << p->value << " ";
				p++;
			}
		stream << endl;
	}

	if (!stream.fail())
		return 0;
	else
		return 1;
}

int svm_save_model_alt(std::string& buffer, const svm_model *model){
	std::ostringstream strstream;
	int ret = svm_save_model_alt(strstream, model);
	buffer = strstream.rdbuf()->str();
	return ret;
}


#include <algorithm>

svm_model *svm_load_model_alt(std::istream& stream)
{
	svm_model *model = Malloc(svm_model,1);
	svm_parameter& param = model->param;
	model->rho = NULL;
	model->probA = NULL;
	model->probB = NULL;
	model->label = NULL;
	model->nSV = NULL;

	char cmd[81];
	stream.width(80);
	while (stream.good())
	{
		stream >> cmd;

		if(strcmp(cmd, "svm_type") == 0)
		{
			stream >> cmd;
			int i;
			for(i=0; svm_type_table[i]; i++)
			{
				if(strcmp(cmd, svm_type_table[i]) == 0)
				{
					param.svm_type=i;
					break;
				}
			}
			if(svm_type_table[i] == NULL)
			{
				fprintf(stderr, "unknown svm type.\n");
				free(model->rho);
				free(model->label);
				free(model->nSV);
				free(model);
				return NULL;
			}
		}
		else if(strcmp(cmd, "kernel_type") == 0)
		{
			stream >> cmd;
			int i;
			for(i=0;kernel_type_table[i];i++)
			{
				if(strcmp(kernel_type_table[i], cmd)==0)
				{
					param.kernel_type=i;
					break;
				}
			}
			if(kernel_type_table[i] == NULL)
			{
				fprintf(stderr,"unknown kernel function.\n");
				free(model->rho);
				free(model->label);
				free(model->nSV);
				free(model);
				return NULL;
			}
		}
		else if(strcmp(cmd,"degree")==0)
			stream >> param.degree;
		else if(strcmp(cmd,"gamma")==0)
			stream >> param.gamma;
		else if(strcmp(cmd,"coef0")==0)
			stream >> param.coef0;
		else if(strcmp(cmd,"nr_class")==0)
			stream >> model->nr_class;
		else if(strcmp(cmd,"total_sv")==0)
			stream >> model->l;
		else if(strcmp(cmd,"rho")==0)
		{
			int n = model->nr_class * (model->nr_class-1)/2;
			model->rho = Malloc(double,n);
			string rho_str;
			for(int i=0;i<n;i++){
				// Read the number into a string and then use strtod
				// for proper handling of NaN's
				stream >> rho_str;
				model->rho[i] = strtod(rho_str.c_str(), NULL);
			}
		}
		else if(strcmp(cmd,"label")==0)
		{
			int n = model->nr_class;
			model->label = Malloc(int,n);
			for(int i=0;i<n;i++)
				stream >> model->label[i];
		}
		else if(strcmp(cmd,"probA")==0)
		{
			int n = model->nr_class * (model->nr_class-1)/2;
			model->probA = Malloc(double,n);
			for(int i=0;i<n;i++)
				stream >> model->probA[i];
		}
		else if(strcmp(cmd,"probB")==0)
		{
			int n = model->nr_class * (model->nr_class-1)/2;
			model->probB = Malloc(double,n);
			for(int i=0;i<n;i++)
				stream >> model->probB[i];
		}
		else if(strcmp(cmd,"nr_sv")==0)
		{
			int n = model->nr_class;
			model->nSV = Malloc(int,n);
			for(int i=0;i<n;i++)
				stream >> model->nSV[i];
		}
		else if(strcmp(cmd,"SV")==0)
		{
			while(1)
			{
				int c = stream.get();
				if(stream.eof() || c=='\n') break;
			}
			break;
		}
		else
		{
			fprintf(stderr,"unknown text in model file: [%s]\n",cmd);
			free(model->rho);
			free(model->label);
			free(model->nSV);
			free(model);
			return NULL;
		}
	}
	if (stream.fail()){
		free(model->rho);
		free(model->label);
		free(model->nSV);
		free(model);
		return NULL;

	}

	// read sv_coef and SV

	int elements = 0;
	long pos = stream.tellg();

	char *p,*endptr,*idx,*val;
	string str_line;
	while (!stream.eof() && !stream.fail())
	{
		getline(stream, str_line);
		elements += std::count(str_line.begin(), str_line.end(), ':');
	}

	elements += model->l;

	stream.clear();
	stream.seekg(pos, ios::beg);

	int m = model->nr_class - 1;
	int l = model->l;
	model->sv_coef = Malloc(double *,m);
	int i;
	for(i=0;i<m;i++)
		model->sv_coef[i] = Malloc(double,l);
	model->SV = Malloc(svm_node*,l);
	svm_node *x_space = NULL;
	if(l>0) x_space = Malloc(svm_node,elements);

	int j=0;
	char *line;
	for(i=0;i<l;i++)
	{
		getline(stream, str_line);
		if (str_line.size() == 0)
			continue;

		line = (char *) Malloc(char, str_line.size() + 1);
		// Copy the line for strtok.
		strcpy(line, str_line.c_str());

		model->SV[i] = &x_space[j];

		p = strtok(line, " \t");
		model->sv_coef[0][i] = strtod(p,&endptr);
		for(int k=1;k<m;k++)
		{
			p = strtok(NULL, " \t");
			model->sv_coef[k][i] = strtod(p,&endptr);
		}

		while(1)
		{
			idx = strtok(NULL, ":");
			val = strtok(NULL, " \t");

			if(val == NULL)
				break;
			x_space[j].index = (int) strtol(idx,&endptr,10);
			x_space[j].value = strtod(val,&endptr);

			++j;
		}
		x_space[j++].index = -1;
		free(line);
	}

	if (stream.fail())
		return NULL;

	model->free_sv = 1;	// XXX
	return model;
}

svm_model *svm_load_model_alt(std::string& stream)
{
	std::istringstream strstream(stream);
	return svm_load_model_alt(strstream);
}

svm_node* example_to_svm(const TExample &ex, svm_node* node, double last=0.0, int type=0){
	if(type==0){
		int index=1;
		for(TExample::const_iterator i=ex.begin(); i!=ex.end_features(); i++){
			if(!isnan(*i)){
				node->value=*i;
				node->index=index++;
				if(*i==numeric_limits<int>::max())
					node--;
				node++;
			}
		}
	}
    if(type == 1){ /*one dummy attr so we can pickle the classifier and keep the SV index in the training table*/
        node->index=1;
        node->value=last;
        node++;
    }
	//cout<<(node-1)->index<<endl<<(node-2)->index<<endl;
	node->index=-1;
	node->value=last;
	node++;
	return node;
}

class SVM_NodeSort{
public:
	bool operator() (const svm_node &lhs, const svm_node &rhs){
		return lhs.index < rhs.index;
	}
};

svm_node* example_to_svm_sparse(const TExample &ex, svm_node* node, double last=0.0, bool useNonMeta=false){
	svm_node *first=node;
	int j=1;
	int index=1;
	if(useNonMeta)
		for(TExample::const_iterator i=ex.begin(); i!=ex.end_features(); i++){
			if(!isnan(*i)){
				node->value=*i;
				node->index=index;
				if(*i==numeric_limits<int>::max())
					node--;
				node++;
			}
			index++;
		}
    ITERATE_METAS(const_cast<TExample &>(ex), mr)
		if(mr.isPrimitive && !isnan(mr.value)){
			node->value=mr.value;
			node->index=index-mr.id;
			if(node->value==numeric_limits<int>::max())
				node--;
			node++;
		}
	}
	sort(first, node, SVM_NodeSort());
	//cout<<first->index<<endl<<(first+1)->index<<endl;
	node->index=-1;
	node->value=last;
	node++;
	return node;
}

/*
 * Precompute Gram matrix row for ex.
 * Used for prediction when using the PRECOMPUTED kernel.
 */
svm_node* example_to_svm_precomputed(
    const TExample &ex, PExampleTable const &examples,
    PKernelFunc const &kernel, svm_node* node)
{
	node->index = 0;
	node->value = 0.0;
	node++;
	int k = 0;
	PEITERATE(iter, examples) {
		node->index = ++k;
		node->value = (*kernel)(**iter, ex);
		node++;
	}
	node->index = -1; // sentry
	node++;
	return node;
}


int getNumOfElements(const TExample &ex, bool meta=false, bool useNonMeta=false){
	if(!meta)
		return std::max(ex.domain->attributes->size()+1, 2);
	else{
		int count=1; //we need one to indicate the end of a sequence
		if(useNonMeta)
			count+=ex.domain->attributes->size();
        ITERATE_METAS(const_cast<TExample &>(ex), mr)
            if (mr.isPrimitive && !isnan(mr.value))
				count++;
        }
		return std::max(count,2);
	}
}

int getNumOfElements(PExampleTable const &examples, bool meta=false, bool useNonMeta=false){
	if(!meta)
		return examples->size() * std::max(examples->domain->attributes->size()+1, 2);
	else{
		int count=0;
		PEITERATE(ex, examples){
			count+=getNumOfElements(**ex, meta, useNonMeta);
		}
		return count;
	}
}


#include "symmatrix.hpp"
svm_node* init_precomputed_problem(svm_problem &problem,
    PExampleTable const &examples, PKernelFunc const &kernelFunc)
{
    TKernelFunc &kernel = *kernelFunc;
	int const n_examples = examples->size();
	int i, j;
	PSymMatrix matrix(new TSymMatrix(n_examples, 0.0));
    TExampleTable::iterator eii(examples->begin());
	for (i = 0; i < n_examples; i++, ++eii) {
        TExampleTable::iterator eji(examples->begin());
		for (j = 0; j <= i; j++, ++eji) {
			matrix->getref(i, j) = kernel(**eii, **eji);
//			cout << i << " " << j << " " << matrix->getitem(i, j) << endl;
		}
    }
	svm_node *x_space = Malloc(svm_node, n_examples * (n_examples + 2));
	svm_node *node = x_space;

	problem.l = n_examples;
	problem.x = Malloc(svm_node*, n_examples);
	problem.y = Malloc(double, n_examples);

    eii = examples->begin();
	for (i = 0; i < n_examples; i++, ++eii){
		problem.x[i] = node;
        problem.y[i] = eii.getClass();
		node->index = 0;
		node->value = i + 1; // instance indices are 1 based
		node++;
		for (j = 0; j < n_examples; j++){
			node->index = j + 1;
			node->value = matrix->getitem(i, j);
			node++;
		}
		node->index = -1; // sentry
		node++;
	}
	return x_space;
}


static void print_string_null(const char* s) {}

TSVMLearner::TSVMLearner(){
	//sparse=false;	//if this learners supports sparse datasets (set to true in TSMVLearnerSparse subclass)
	svm_type = NU_SVC;
	kernel_type = RBF;
	degree = 3;
	gamma = 0;
	coef0 = 0;
	nu = 0.5;
	cache_size = 250;
	C = 1;
	eps = 1e-3f;
	p = 0.1f;
	shrinking = 1;
	probability = 0;
	verbose = false;
	nr_weight = 0;
	weight_label = NULL;
	weight = NULL;
};

PClassifier TSVMLearner::operator ()(PExampleTable const &examples){
	svm_parameter param;
	svm_problem prob;
	svm_model* model;
	svm_node* x_space;

	param.svm_type=svm_type;
	param.kernel_type=kernel_type;
	param.degree=degree;
	param.gamma=gamma;
	param.coef0=coef0;
	param.nu=nu;
	param.C=C;
	param.eps=eps;
	param.p=p;
	param.cache_size=cache_size;
	param.shrinking=shrinking;
	param.probability=probability;
	param.nr_weight = nr_weight;
	if (nr_weight > 0) {
		param.weight_label = Malloc(int, nr_weight);
		param.weight = Malloc(double, nr_weight);
		int i;
		for (i=0; i<nr_weight; i++) {
			param.weight_label[i] = weight_label[i];
			param.weight[i] = weight[i];
		}
	} else {
		param.weight_label = NULL;
		param.weight = NULL;
	}
	//cout<<param.kernel_type<<endl;

	tempExamples=examples;
	//int exlen=examples->domain->attributes->size();
	if(examples->domain->classVar) {
	    if((examples->domain->classVar->varType!=TVariable::Discrete) && !(svm_type==EPSILON_SVR || svm_type==NU_SVR ||svm_type==ONE_CLASS))
    		raiseError(PyExc_TypeError, "This type of SVM expects a discrete class");
    }
	else{
		if(svm_type!=ONE_CLASS)
			raiseError(PyExc_TypeError, "Domain has no class variable");
	}

	if(param.kernel_type==PRECOMPUTED && !kernelFunc)
		raiseError(PyExc_ValueError, "The kernel function is missing");

	int numElements=getNumOfElements(examples);

	if(kernel_type != PRECOMPUTED)
		x_space = init_problem(prob, examples, numElements);
	else // Compute the matrix using the kernelFunc
		x_space = init_precomputed_problem(prob, examples, kernelFunc);

	if(param.gamma==0)
		param.gamma=1.0f/(double(numElements)/double(prob.l)-1);

	const char* error=svm_check_parameter(&prob,&param);
	if(error){
		free(x_space);
		free(prob.y);
		free(prob.x);
		raiseError(PyExc_RuntimeError, "LibSVM parameter error: %s", error);
	}

    //	svm_print_string = (verbose)? &print_string_stdout : &print_string_null;

	// If a probability model was requested LibSVM uses 5 fold
	// cross-validation to estimate the prediction errors. This includes a
	// random shuffle of the data. To make the results reproducible and
	// consistent with 'svm-train' (which always learns just on one dataset
	// in a process run) we reset the random seed. This could have unintended
	// consequences.
	if (param.probability)
	{
		srand(1);
	}
	svm_set_print_string_function((verbose)? NULL : &print_string_null);
	model=svm_train(&prob,&param);

  if ((svm_type==C_SVC || svm_type==NU_SVC) && !model->nSV) {
  	svm_free_and_destroy_model(&model);
    if (x_space)
      free(x_space);
      raiseError(PyExc_RuntimeError, "LibSVM returned no support vectors");
  }

	//cout<<"end training"<<endl;
	svm_destroy_param(&param);
	free(prob.y);
	free(prob.x);
	return PClassifier(createClassifier((param.svm_type==ONE_CLASS)?  \
		PVariable(new TContinuousVariable("one class"))
        : examples->domain->classVar, examples, model, x_space));
}

svm_node* TSVMLearner::example_to_svm(const TExample &ex, svm_node* node, double last, int type){
	return ::example_to_svm(ex, node, last, type);
}


svm_node* TSVMLearner::init_problem(
    svm_problem &problem, PExampleTable const &examples, int n_elements)
{
	problem.l = examples->size();
	problem.y = Malloc(double ,problem.l);
	problem.x = Malloc(svm_node*, problem.l);
	svm_node *x_space = Malloc(svm_node, n_elements);
	svm_node *node = x_space;
    TExampleTable::iterator ei(examples->begin());
	for (int i = 0; i < problem.l; i++, ++ei) {
		problem.x[i] = node;
		node = example_to_svm(**ei, node, i);
		if (examples->domain->classVar)
    		problem.y[i] = ei.getClass();
	}
	return x_space;
}


int TSVMLearner::getNumOfElements(PExampleTable const &examples){
	return ::getNumOfElements(examples);
}

TSVMClassifier* TSVMLearner::createClassifier(PVariable const &var, PExampleTable const &ex, svm_model* model, svm_node* x_space){
	return new TSVMClassifier(var, ex, model, x_space, kernelFunc);
}

TSVMLearner::~TSVMLearner(){
	if(weight_label)
		free(weight_label);

	if(weight)
			free(weight);
}

svm_node* TSVMLearnerSparse::example_to_svm(const TExample &ex, svm_node* node, double last, int type){
	return ::example_to_svm_sparse(ex, node, last, useNonMeta);
}

int TSVMLearnerSparse::getNumOfElements(PExampleTable const &examples){
	return ::getNumOfElements(examples, true, useNonMeta);
}

TSVMClassifier* TSVMLearnerSparse::createClassifier(PVariable const &var, PExampleTable const &ex, svm_model* model, svm_node *x_space){
	return new TSVMClassifierSparse(var, ex, model, x_space, useNonMeta, kernelFunc);
}

/*TSVMClassifier::TSVMClassifier(PDomain const &aDomain, svm_model *aModel)
    : TClassifier(aDomain),
    model(aModel),
    x_space(NULL)
{}
*/

TSVMClassifier::TSVMClassifier(
    const PVariable &var, PExampleTable const &_examples,
    svm_model* _model, svm_node* _x_space,
    PKernelFunc const &_kernelFunc)
: TClassifier(_examples->domain),
  model(_model),
  x_space(_x_space),
  examples(_examples),
  kernelFunc(_kernelFunc)
{
	classVar=var;

	svm_type = svm_get_svm_type(model);
	kernel_type = model->param.kernel_type;

/*	computesProbabilities = model && svm_check_probability_model(model) && \
			(svm_type != NU_SVR && svm_type != EPSILON_SVR); // Disable prob. estimation for regression
            */
	int nr_class = svm_get_nr_class(model);
	int i = 0;
	supportVectors=PExampleTable(TExampleTable::constructEmptyReference(examples));
	if(x_space){
		for(i = 0;i < model->l; i++){
			svm_node *node = model->SV[i];
			int sv_index = 0;
			if(model->param.kernel_type != PRECOMPUTED){
				// The value of the last node (with index == -1) holds the index of the training example.
				while(node->index != -1)
					node++;
				sv_index = int(node->value);
			}
			else
				sv_index = int(node->value) - 1; // The indices for precomputed kernels are 1 based.
			supportVectors->push_back(examples->at(int(node->value)));
		}
	}
	
    if (svm_type==C_SVC || svm_type==NU_SVC){
	    nSV=PIntList(new TIntList(nr_class)); // num of SVs for each class (sum = model->l)
	    for(i=0;i<nr_class;i++)
		    nSV->at(i)=model->nSV[i];
    }

	coef=PFloatListList(new TFloatListList(nr_class-1));
	for(i=0;i<nr_class-1;i++){
		TFloatList *coefs=new TFloatList(model->l);
		for(int j=0;j<model->l;j++)
			coefs->at(j)=model->sv_coef[i][j];
		coef->at(i)=PFloatList(coefs);
	}
	rho=PFloatList(new TFloatList(nr_class*(nr_class-1)/2));
	for(i=0;i<nr_class*(nr_class-1)/2;i++)
		rho->at(i)=model->rho[i];
	if(model->probA){
		probA = PFloatList(new TFloatList(nr_class*(nr_class-1)/2));
		if (model->param.svm_type != NU_SVR && model->param.svm_type != EPSILON_SVR && model->probB) // Regression has only probA
			probB = PFloatList(new TFloatList(nr_class*(nr_class-1)/2));
		for(i=0;i<nr_class*(nr_class-1)/2;i++){
			probA->at(i) = model->probA[i];
			if (model->param.svm_type != NU_SVR && model->param.svm_type != EPSILON_SVR && model->probB)
				probB->at(i) = model->probB[i];
		}
	}
}

TSVMClassifier::~TSVMClassifier(){
	if (model)
		svm_free_and_destroy_model(&model);
	if(x_space)
		free(x_space);
}

PDistribution TSVMClassifier::classDistribution(TExample const *const example)
{
	if(!model)
		raiseError(PyExc_ValueError, "No Model");

	if(svm_type==NU_SVR || svm_type==EPSILON_SVR)
		raiseError(PyExc_TypeError, "Model does not support probabilities for regression");

	int n_elements;
	if (model->param.kernel_type != PRECOMPUTED)
		n_elements = getNumOfElements(*example);
	else
		n_elements = examples->size() + 2;

	int svm_type = svm_get_svm_type(model);
	int nr_class = svm_get_nr_class(model);

	svm_node *x = Malloc(svm_node, n_elements);
	try{
		if (model->param.kernel_type != PRECOMPUTED)
			example_to_svm(*example, x, -1.0);
		else
			example_to_svm_precomputed(*example, examples, kernelFunc, x);
	} catch (...) {
		free(x);
		throw;
	}

	int *labels=(int *) malloc(nr_class*sizeof(int));
	svm_get_labels(model, labels);

	double *prob_estimates = (double *) malloc(nr_class*sizeof(double));;
	svm_predict_probability(model, x, prob_estimates);

	PDistribution dist = TDistribution::create(example->domain->classVar);
	for(int i=0; i<nr_class; i++)
		dist->set(labels[i], prob_estimates[i]);
	free(x);
	free(prob_estimates);
	free(labels);
	return dist;
}

TValue TSVMClassifier::operator()(TExample const *const example)
{
	if(!model)
		raiseError(PyExc_ValueError, "No Model");

	int n_elements;
	if (model->param.kernel_type != PRECOMPUTED)
		n_elements = getNumOfElements(*example); //example.domain->attributes->size();
	else
		n_elements = examples->size() + 2;

	int svm_type = svm_get_svm_type(model);
	int nr_class = svm_get_nr_class(model);

	svm_node *x = Malloc(svm_node, n_elements);
	try {
		if (model->param.kernel_type != PRECOMPUTED)
			example_to_svm(*example, x);
		else
			example_to_svm_precomputed(*example, examples, kernelFunc, x);
	} catch (...) {
		free(x);
		throw;
	}

	double v;

	if(svm_check_probability_model(model)){
		double *prob = (double *) malloc(nr_class*sizeof(double));
		v = svm_predict_probability(model, x, prob);
		free(prob);
	} else
		v = svm_predict(model, x);

	free(x);
	if(svm_type==NU_SVR || svm_type==EPSILON_SVR || svm_type==ONE_CLASS)
		return TValue(v);
	else
		return TValue(int(v));
}


PFloatList TSVMClassifier::getDecisionValues(const TExample &example)
{
	if(!model)
		raiseError(PyExc_ValueError, "No Model");

	int n_elements;
		if (model->param.kernel_type != PRECOMPUTED)
			n_elements = getNumOfElements(example); //example.domain->attributes->size();
		else
			n_elements = examples->size() + 2;

	int svm_type=svm_get_svm_type(model);
	int nr_class=svm_get_nr_class(model);

	svm_node *x = Malloc(svm_node, n_elements);
	try {
		if (model->param.kernel_type != PRECOMPUTED)
			example_to_svm(example, x);
		else
			example_to_svm_precomputed(example, examples, kernelFunc, x);
	} catch (...) {
		free(x);
		throw;
	}

	int nDecValues = nr_class*(nr_class-1)/2;
	double *dec = (double*) malloc(sizeof(double)*nDecValues);
	svm_predict_values(model, x, dec);
	PFloatList res(new TFloatList(nDecValues));
	for(int i=0; i<nDecValues; i++){
		res->at(i) = dec[i];
	}
	free(x);
	free(dec);
	return res;
}


svm_node *TSVMClassifier::example_to_svm(const TExample &ex, svm_node *node, double last, int type){
	return ::example_to_svm(ex, node, last, type);
}

int TSVMClassifier::getNumOfElements(const TExample& example){
	return ::getNumOfElements(example);
}
svm_node *TSVMClassifierSparse::example_to_svm(const TExample &ex, svm_node *node, double last, int type){
	return ::example_to_svm_sparse(ex, node, last, useNonMeta);
}

int TSVMClassifierSparse::getNumOfElements(const TExample& example){
	return ::getNumOfElements(example, true, useNonMeta);
}


PyObject *TSVMLearner::setWeights(PyObject* args, PyObject *kw)
{
	PyObject *pyWeights;
	if (!PyArg_ParseTupleAndKeywords(args, kw, "O:set_weights",
            SVMLearner_setWeights_keywords, &pyWeights)) {
		return NULL;
	}
	Py_ssize_t size = PyList_Size(pyWeights);
	Py_ssize_t i;
	free(weight_label);
	free(weight);
	nr_weight = size;
	weight_label = NULL;
	weight = NULL;
    if (size) {
		weight_label = (int *)malloc(size * sizeof(int));
		weight = (double *)malloc(size * sizeof(double));
	}
	for (i = 0; i < size; i++) {
		int l;
		double w;
		if (!PyArg_ParseTuple(PyList_GetItem(pyWeights, i), "id:setWeights", &l, &w)) {
            free(weight);
            free(weight_label);
            weight_label = NULL;
            weight = NULL;
            return NULL;
        }
		weight[i] = w;
		weight_label[i] = l;
	}
    Py_RETURN_NONE;
}


TOrange *TSVMClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *)
{
    PVariable classVar;
    PExampleTable examples;
    PExampleTable supportVectors;
    char *model_string;
    PKernelFunc kernelFunc;
    if (!PyArg_ParseTuple(args, "O&O&O&sO&:SVMClassifier",
        &PVariable::argconverter, &classVar,
        &PExampleTable::argconverter, &examples,
        &PExampleTable::argconverter, &supportVectors,
        &model_string,
        &PKernelFunc::argconverter_n, &kernelFunc)) {
            return NULL;
    }
    string modstring(model_string);
    svm_model *model = svm_load_model_alt(modstring);
    if (!model) {
        PyErr_Format(PyExc_UnpicklingError, "invalid SVM model");
        return NULL;
    }
    return new(type) TSVMClassifier(classVar, examples, model, NULL, kernelFunc);
}


PyObject *TSVMClassifier::__getnewargs__()
{
    string buf;
    if (svm_save_model_alt(buf, getModel())){
    	raiseError(PyExc_RuntimeError, "Error saving SVM model");
    }
    return Py_BuildValue("NNNsN",
        domain->classVar.getPyObject(),
        examples.getPyObject(),
        supportVectors.getPyObject(),
        buf.c_str(),
        kernelFunc.toPython());
}


PyObject *TSVMClassifier::py_getDecisionValues(PyObject *args, PyObject *kw)
{
	PExample example;
	if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:get_decision_values",
        SVMClassifier_getDecisionValues_keywords,
        &PExample::argconverter, &example)) {
		return NULL;
    }
	return getDecisionValues(*example).getPyObject();
}


PyObject *TSVMClassifier::py_getModel()
{
    string buf;
	svm_model *model = getModel();
	if (!model) {
		raiseError(PyExc_RuntimeError, "No model");
    }
	svm_save_model_alt(buf, model);
    return PyUnicode_FromString(buf.c_str());
}



TOrange *TSVMClassifierSparse::__new__(PyTypeObject *type, PyObject *args, PyObject *)
{
    PVariable classVar;
    PExampleTable examples;
    PExampleTable supportVectors;
    char *model_string;
    PKernelFunc kernelFunc;
    int useNonMeta;
    if (!PyArg_ParseTuple(args, "O&O&O&sO&i:SVMClassifierSparse",
        &PVariable::argconverter, &classVar,
        &PExampleTable::argconverter, &examples,
        &PExampleTable::argconverter, &supportVectors,
        &model_string,
        &PKernelFunc::argconverter_n, &kernelFunc,
        &useNonMeta)) {
            return NULL;
    }
    string modstring(model_string);
    svm_model *model = svm_load_model_alt(modstring);
    if (!model) {
        PyErr_Format(PyExc_UnpicklingError, "invalid SVM model");
        return NULL;
    }
    return new(type) TSVMClassifierSparse(
        classVar, examples, model, NULL, useNonMeta != 0, kernelFunc);
}



PyObject *TSVMClassifierSparse::__getnewargs__()
{
    string buf;
    if (svm_save_model_alt(buf, getModel())){
    	raiseError(PyExc_RuntimeError, "Error saving SVM model");
    }
    return Py_BuildValue("NNNsNi",
        domain->classVar.getPyObject(),
        examples.getPyObject(),
        supportVectors.getPyObject(),
        buf.c_str(),
        kernelFunc.toPython(),
        useNonMeta ? 1 : 0);
}


