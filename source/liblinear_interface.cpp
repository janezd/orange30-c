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


/*
Copy from linear.cpp of LIBLINEAR
Changes:
	-#include "linear.h" -> #include "linear.ppp"
	-#ifndef block around swap, min and max definition
*/

#include "common.hpp"

#include <iostream>
#include <sstream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "liblinear_interface.px"


#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
// Defined in linear.cpp. If a new solver is added this should be updated.

static const char *solver_type_table[]=
{
	"L2R_LR", "L2R_L2LOSS_SVC_DUAL", "L2R_L2LOSS_SVC", "L2R_L1LOSS_SVC_DUAL", "MCSVM_CS",
	"L1R_L2LOSS_SVC", "L1R_LR", "L2R_LR_DUAL", NULL
};

/*
 *The folowing load save functions are used for orange pickling
 */

/*
 * Save the model to an std::ostream. This is a modified `save_model` function
 * from `linear.cpp` in LIBLINEAR package.
 */
int linear_save_model_alt(ostream &stream, struct model *model_)
{
	int i;
	int nr_feature=model_->nr_feature;
	int n;
	const parameter& param = model_->param;

	if(model_->bias>=0)
		n=nr_feature+1;
	else
		n=nr_feature;

	int nr_classifier;
	if(model_->nr_class==2 && model_->param.solver_type != MCSVM_CS)
		nr_classifier=1;
	else
		nr_classifier=model_->nr_class;

	stream.precision(17);

	stream << "solver_type " << solver_type_table[param.solver_type] << endl;
	stream << "nr_class " << model_->nr_class << endl;
	stream << "label";
	for(i=0; i<model_->nr_class; i++)
		stream << " " << model_->label[i];
	stream << endl;

	stream << "nr_feature " << nr_feature << endl;

	stream << "bias " << model_->bias << endl;

	stream << "w" << endl;
	for(i=0; i<n; i++)
	{
		int j;
		for(j=0; j<nr_classifier; j++)
			stream << model_->w[i*nr_classifier+j] << " ";
		stream << endl;
	}

	if (stream.good())
		return 0;
	else
		return -1;
}

/*
 * Save linear model into a std::string.
 */
int linear_save_model_alt(string &buffer, struct model *model_)
{
	std::ostringstream strstream;
	int ret = linear_save_model_alt(strstream, model_);
	buffer = strstream.rdbuf()->str();
	return ret;
}

/*
 * Load a linear model from std::istream. This is a modified `load_model`
 * function from `linear.cpp` in LIBLINEAR package.
 */
struct model *linear_load_model_alt(istream &stream)
{
	int i;
	int nr_feature;
	int n;
	int nr_class;
	double bias;
	model *model_ = Malloc(model,1);
	parameter& param = model_->param;

	model_->label = NULL;

	char cmd[81];
	stream.width(80);
	while(stream.good())
	{
		stream >> cmd;
		if(strcmp(cmd, "solver_type")==0)
		{
			stream >> cmd;
			int i;
			for(i=0;solver_type_table[i];i++)
			{
				if(strcmp(solver_type_table[i],cmd)==0)
				{
					param.solver_type=i;
					break;
				}
			}
			if(solver_type_table[i] == NULL)
			{
				fprintf(stderr,"unknown solver type.\n");
				free(model_->label);
				free(model_);
				return NULL;
			}
		}
		else if(strcmp(cmd,"nr_class")==0)
		{
			stream >> nr_class;
			model_->nr_class=nr_class;
		}
		else if(strcmp(cmd,"nr_feature")==0)
		{
			stream >> nr_feature;
			model_->nr_feature=nr_feature;
		}
		else if(strcmp(cmd,"bias")==0)
		{
			stream >> bias;
			model_->bias=bias;
		}
		else if(strcmp(cmd,"w")==0)
		{
			break;
		}
		else if(strcmp(cmd,"label")==0)
		{
			int nr_class = model_->nr_class;
			model_->label = Malloc(int, nr_class);
			for(int i=0;i<nr_class;i++)
				stream >> model_->label[i];
		}
		else
		{
			fprintf(stderr,"unknown text in model file: [%s]\n",cmd);
			free(model_->label);
			free(model_);
			return NULL;
		}
	}

	nr_feature=model_->nr_feature;
	if(model_->bias>=0)
		n=nr_feature+1;
	else
		n=nr_feature;

	int nr_classifier;
	if(nr_class==2 && param.solver_type != MCSVM_CS)
		nr_classifier = 1;
	else
		nr_classifier = nr_class;

	model_->w=Malloc(double, n*nr_classifier);
	for(i=0; i<n; i++)
	{
		int j;
		for(j=0; j<nr_classifier; j++)
			stream >> model_->w[i*nr_classifier+j];
	}
	if (stream.fail())
		return NULL;
	else
		return model_;
}

/*
 * Load a linear model from a std:string.
 */
struct model *linear_load_model_alt(string &buffer)
{
	std::istringstream str_stream(buffer);
	return linear_load_model_alt(str_stream);
}


struct NodeSort{
	bool operator () (const feature_node &lhs, const feature_node &rhs){
		return lhs.index < rhs.index;
	}
};

int countFeatures(TExample const *const ex, bool includeMeta, bool includeRegular)
{
	int count = 1;
	if (includeRegular) {
        TExample::const_iterator const ee = ex->end_features();
		for (TExample::const_iterator i=ex->begin(); i != ee; i++) {
			if (!isnan(*i)) {
				count++;
            }
        }
    }
	if (includeMeta) {
        ITERATE_METAS(const_cast<TExample &>(*ex), mr)
            if (mr.isPrimitive && !isnan(mr.value)) {
				count++;
            }
        }
    }
    return count;
}


feature_node *feature_nodeFromExample(TExample const *const ex, map<int, int> &indexMap, bool includeMeta=false, bool includeRegular=true){
	//cout << "example " << endl;
	int numOfNodes = countFeatures(ex, includeMeta, includeRegular);
	/*if (includeRegular)
		numOfNodes += ex.domain->attributes->size();
	if (includeMeta)
		numOfNodes += ex.meta.size();*/
	feature_node *nodes = new feature_node[numOfNodes];
	feature_node *ptr = nodes;
	int index = 1;
	int featureIndex = 1;
	if (includeRegular){
        TExample::const_iterator const ee = ex->end_features();
		for (TExample::const_iterator i=ex->begin(); i != ee; i++){
			if (!isnan(*i)) {
    			ptr->value = *i;
				ptr->index = index;
				if (indexMap.find(index)==indexMap.end()){
					ptr->index = featureIndex;
					indexMap[index] = featureIndex++;
				} else
					ptr->index = indexMap[index];
				//featureIndices.insert(index);
				//cout << ptr->value << " ";
				ptr++;
			}
			index++;
		}
	}
	if (includeMeta){
		feature_node *first = ptr;
        ITERATE_METAS(const_cast<TExample &>(*ex), mr) 
			if (mr.isPrimitive && !isnan(mr.value)) {
				ptr->value = mr.value;
				//ptr->index = index - i->first;
				if (indexMap.find(mr.id)==indexMap.end()){
					ptr->index = featureIndex;
					indexMap[mr.id] = featureIndex++;
				} else
					ptr->index = indexMap[mr.id];
				//featureIndices.insert(ptr->index);
				ptr++;
			}
		}
		//cout << endl << "	sorting" << endl;
		sort(first, ptr, NodeSort());
	}
	ptr->index = -1;
	return nodes;
}

problem *problemFromExamples(PExampleTable const &examples, map<int, int> &indexMap, bool includeMeta=false, bool includeRegular=true){
	problem *prob = new problem;
	prob->l = examples->size();
	prob->x = new feature_node* [prob->l];
	prob->y = new int [prob->l];
	prob->bias = -1.0;
	feature_node **ptrX = prob->x;
	int *ptrY = prob->y;
	PEITERATE(iter, examples) {
		*ptrX = feature_nodeFromExample(*iter, indexMap, includeMeta, includeRegular);
		*ptrY = (int) iter.getClass();
		ptrX++;
		ptrY++;
	}
	prob->n = indexMap.size();
	//cout << "prob->n " << prob->n <<endl;
	return prob;
}

void destroy_problem(problem *prob){
	for (int i=0; i<prob->l; i++)
		delete[] prob->x[i];
	delete[] prob->x;
	delete[] prob->y;
}


TLinearLearner::TLinearLearner(){
	solver_type = L2_LR;
	eps = 0.01f;
	C=1;
}

PClassifier TLinearLearner::operator()(PExampleTable const &examples)
{
	//cout << "initializing param" << endl;
	parameter *param = new parameter;
	param->solver_type = solver_type;
	param->eps = eps;
	param->C = C;
	param->nr_weight = 0;
	param->weight_label = NULL;
	param->weight = NULL;
	//cout << "initializing problem" << endl;
	map<int, int> *indexMap =new map<int, int>;
	problem *prob = problemFromExamples(examples, *indexMap);
	//cout << "cheking parameters" << endl;
	const char * error_msg = check_parameter(prob, param);
	if (error_msg){
		delete param;
		destroy_problem(prob);
		raiseError(PyExc_SystemError, "LIBLINEAR error: %s" , error_msg);
	}
     /* The solvers in liblinear use rand() function.
	 * To make the results reporoducible we set the seed from the data table's
	 * crc
	 */
	srand(examples->checkSum(false));
	model *model = train(prob, param);
	destroy_problem(prob);

	return PClassifier(new TLinearClassifier(examples->domain->classVar, examples, model, indexMap));
}


TLinearClassifier::TLinearClassifier(PDomain const &dom)
    : TClassifier(dom)
{}

TLinearClassifier::TLinearClassifier(
        PVariable const &var, PExampleTable const &_examples,
        struct model *_model, map<int, int> *_indexMap)
            : TClassifier(var)
{
	linmodel = _model;
	examples = _examples;
	domain = examples->domain;
	indexMap = _indexMap;
	int nr_classifier = (linmodel->nr_class==2)? 1 : linmodel->nr_class;
	weights = PFloatListList(new TFloatListList(nr_classifier));
	for (int i=0; i<nr_classifier; i++){
		weights->at(i) = PFloatList(new TFloatList(linmodel->nr_feature));
		for (int j=0; j<linmodel->nr_feature; j++)
			weights->at(i)->at(j) = linmodel->w[j*nr_classifier+i];
	}
}

TLinearClassifier::~TLinearClassifier(){
	if (linmodel)
		free_and_destroy_model(&linmodel);
	if (indexMap)
		delete indexMap;
}

PDistribution TLinearClassifier::classDistribution(TExample const *const example)
{
    PExample new_example = example->convertedTo(domain);
	int numClass = get_nr_class(linmodel);
	map<int, int> indexMap;
	feature_node *x = feature_nodeFromExample(new_example.borrowPtr(), indexMap, false);

	int *labels = new int [numClass];
	get_labels(linmodel, labels);

	double *prob_est = new double [numClass];
	predict_probability(linmodel, x, prob_est);

	PDistribution dist = TDistribution::create(classVar);
	for (int i=0; i<numClass; i++)
		dist->set(labels[i], prob_est[i]);

	delete[] x;
	delete[] labels;
	delete[] prob_est;
	return dist;
}

TValue TLinearClassifier::operator () (TExample const *const example)
{
    PExample new_example = example->convertedTo(domain);
	int numClass = get_nr_class(linmodel);
	map<int, int> indexMap;
	feature_node *x = feature_nodeFromExample(new_example.borrowPtr(), indexMap, false);

	int predict_label = predict(linmodel, x);
	delete[] x;
	return TValue(predict_label);
}



PyObject *TLinearClassifier::__getnewargs__()
{
	string buff;
	if (linear_save_model_alt(buff, getModel()) != 0) {
		raiseError(PyExc_PicklingError, "Error while saving the model");
    }
    return Py_BuildValue("NNs", 
        classVar.getPyObject(), examples.getPyObject(), buff.c_str());
}

TOrange *TLinearClassifier::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    PExampleTable examples;
    char *model_string;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&O&s:LinearClassifier",
        LinearClassifier_keywords,
        &PVariable::argconverter, &var,
        &PExampleTable::argconverter, &examples,
        &model_string)) {
            return NULL;
    }
    string modstring(model_string);
    model *mod = linear_load_model_alt(modstring);
    if (!mod) {
        PyErr_Format(PyExc_UnpicklingError, "invalid Linear model");
        return NULL;
    }
    return new(type) TLinearClassifier(var, examples, mod);
}
