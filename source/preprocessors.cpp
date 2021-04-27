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
#include "filter.hpp"
#include "makerandomindices.hpp"
#include "preprocessors.px"


void TPreprocessor::checkChangeReference(int const result)
{
    if (result == Reference) {
        raiseError(PyExc_ValueError,
            "chaning the reference would change the referenced data");
    }
}


PyObject *TPreprocessor::__call__(PyObject *args, PyObject *kw)
{
    PExampleTable data;
    Result result = Default;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|i",
        Preprocessor_call_keywords,
        &PExampleTable::argconverter, &data, &result)) {
            return NULL;
    }
    return (*this)(data, Result(result)).getPyObject();
}


PExampleTable TPreprocessor_removeDuplicates::operator()(
    PExampleTable const &data, Result result)
{ 
    if (result == Default) {
        result = Reference;
    }
    PExampleTable table(constructReturn(data, result));
    table->removeDuplicates(true);
    return table;
}


PExampleTable TPreprocessor_dropMissing::operator()(
    PExampleTable const &data, Result result)
{ 
    if (result == Default) {
        result = Reference;
    }
    PFilter filter(new TFilter_hasSpecial(true));
    if (result == InPlace) {
        data->filterInPlace(filter);
        return data;
    }
    else if (result == Reference) {
        PExampleTable table = TExampleTable::constructEmptyReference(data);
        TExampleTable &rtable = *table;
        TExampleIterator ei(data);
        TFilter &rfilter = *filter;
        for(int i = 0; ei; ++ei) {
            if (rfilter(*ei)) {
                rtable.push_back(*ei);
            }
        }
        return table;
    }
    else {
        // better spend some memory for these indicators than over-allocate data
        PIntList toTake(new TIntList(data->size(), 0));
        TIntList::iterator ti(toTake->begin());
        TExampleIterator ei(data);
        TFilter &rfilter = *filter;
        for(int i = 0; ei; ++ei, ti++) {
            if (rfilter(*ei)) {
                *ti = 1;
            }
        }
        return data->select(toTake);
    }
}


PExampleTable TPreprocessor_dropMissingClasses::operator()(
    PExampleTable const &data, Result result)
{ 
    if (result == Default) {
        result = Reference;
    }
    if (result == InPlace) {
        PFilter filter(new TFilter_hasClassValue());
        data->filterInPlace(filter);
        return data;
    }
    else if (result == Reference) {
        PExampleTable table = TExampleTable::constructEmptyReference(data);
        TExampleTable &rtable = *table;
        TExampleIterator ei(data);
        for(int i = 0; ei; ++ei) {
            if (!isnan(ei.getClass())) {
                rtable.push_back(*ei);
            }
        }
        return table;
    }
    else {
        // better spend some time than over-allocate data
        int defined = 0;
        TExampleIterator ei(data);
        while(ei) {
            if (!isnan(ei.getClass())) {
                defined = 0;
            }
        }
        PExampleTable table = TExampleTable::constructEmpty(
            data->domain, defined);
        TExampleTable &rtable = *table;
        ei.seek(0);
        while(ei) {
            if (!isnan(ei.getClass())) {
                rtable.push_back(*ei);
            }
        }
        return table;
    }
}


TPreprocessor_shuffle::TPreprocessor_shuffle()
: attributes(new TVarList())
{}


TPreprocessor_shuffle::TPreprocessor_shuffle(PVarList const &attrs)
: attributes(attrs)
{}


PExampleTable TPreprocessor_shuffle::operator()(
    PExampleTable const &data, Result result)
{
    if (result == Default) {
        result = Reference;
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if (!attributes || !attributes->size()) {
        table->shuffle(randomGenerator);
        return table;
    }
    else {
        vector<int> indices;
        PITERATE(TVarList, vi, attributes) {
            const int idx = data->domain->getVarNum(*vi, false);
            if (idx == ILLEGAL_INT)
                raiseError(PyExc_IndexError,
                    "variable '%s' is not found", (*vi)->cname());
            indices.push_back(idx);
        }
        const int tlen = table->size();
        const_ITERATE(vector<int>, ii, indices) {
            for(int i = tlen; --i; ) {
                swap(table->at(i, *ii),
                     table->at(randomGenerator->randint(i), *ii));
            }
        }
        return table;
    }
}


void addNoise(TAttrIdx const index, double const proportion,
              TMakeRandomIndicesN &mri, PExampleTable const &table)
{ 
    int const nvals = table->domain->variables->at(index)->noOfValues();
    int const N = table->size();
    int const changed = N * proportion;
    int const cdiv = int(0.5 + changed / nvals);
    mri.p = PFloatList(new TFloatList(nvals, cdiv));
    mri.p->push_back(N - nvals*cdiv);
    PIntList rind(mri(N));
    TIntList::const_iterator ri(rind->begin());
    for(int ei = table->size()-1; ei; ei--, ri++) {
        if (*ri < nvals) {
            table->at(ei, index) = *ri;
        }
    }
}


class TGenRand_11 {
public:
    mutable PRandomGenerator rgen;

    inline TGenRand_11()
    {}

    inline TGenRand_11(PRandomGenerator const &agen)
    : rgen(agen)
    {}

    inline double operator()(double const x, double const y) const
    { return rgen->randfloat(x, y); }
};


void addGaussianNoise(TAttrIdx const index, double const deviation,
              TGenRand_11 const &rgen, PExampleTable const &table)
{ 
    for(int ei = table->size(); ei; ) {
        table->at(--ei, index) += gasdev(0.0, deviation, rgen);
    }
}


TPreprocessor_addNoise::TPreprocessor_addNoise(
    double const prop, double const dev, bool const ic)
: proportion(prop),
  deviation(dev),
  includeClass(ic)
{}


PExampleTable TPreprocessor_addNoise::operator()(
    PExampleTable const &data, Result result)
{ 
    if (result == Default) {
        result = Copy;
    }
    checkChangeReference(result);
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if ((proportion < 0.0) || (proportion > 1.0)) {
        raiseError(PyExc_ValueError, "invalid proportion (%.3f)", proportion);
    }
    if (deviation < 0.0) {
        raiseError(PyExc_ValueError, "invalid deviation (%.3f)", deviation);
    }

    TMakeRandomIndicesN makerind;
    makerind.randomGenerator = randomGenerator;
    TGenRand_11 rgen(randomGenerator);    
    int const ide = includeClass ?
        data->domain->attributes->size() : data->domain->variables->size();
    TVarList::const_iterator vi = data->domain->variables->begin();
    for(int idx = 0; idx < ide; idx++, vi++) {
        if ((*vi)->varType == TVariable::Discrete) {
            if (proportion > 0) {
                addNoise(idx, proportion, makerind, table);
            }
        }
        else {
            if (deviation > 0) {
                addGaussianNoise(idx, deviation, rgen, table);
            }
        }
    }
    return table;
} 


TPreprocessor_addMissing::TPreprocessor_addMissing(double const p, bool const c)
: proportion(p),
  includeClass(c)
{}


PExampleTable TPreprocessor_addMissing::operator()(
    PExampleTable const &data, Result result)
{
    if (result == Default) {
        result = Copy;
    }
    checkChangeReference(result);
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if ((proportion < 0.0) || (proportion > 1.0)) {
        raiseError(PyExc_ValueError, "invalid proportion (%.3f)", proportion);
    }

    const int n = table->size();
    TMakeRandomIndices2 makerind(1 - proportion);
    makerind.randomGenerator = randomGenerator;

    TVarList::const_iterator vi = data->domain->variables->begin();
    int const ide = includeClass ?
        data->domain->variables->size() : data->domain->attributes->size();
    for(int idx = 0; idx < ide; idx++, vi++) {
        PIntList rind = makerind(n);
        int eind = 0;
        PITERATE(TIntList, ri, rind) {
            if (*ri) {
                table->at(eind, idx) = UNDEFINED_VALUE;
            }
            eind++;
        }
    }
    return table;
}


TPreprocessor_addMissingClasses::TPreprocessor_addMissingClasses(double const cm)
: proportion(cm)
{}
  
  
PExampleTable TPreprocessor_addMissingClasses::operator()(
    PExampleTable const &data, Result result)
{
    if (result == Default) {
        result = Copy;
    }
    checkChangeReference(result);
    if (!data->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if ((proportion < 0.0) || (proportion > 1.0)) {
        raiseError(PyExc_ValueError, "invalid proportion (%.3f)", proportion);
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if (proportion>0.0) {
        TMakeRandomIndices2 mri2(1 - proportion);
        mri2.randomGenerator = randomGenerator;
        PIntList rind(mri2(data->size()));
        int eind = 0;
        int const classPos = data->domain->attributes->size();
        PITERATE(TIntList, ri, rind) {
            if (*ri) {
                table->at(eind, classPos) = UNDEFINED_VALUE;
            }
            eind++;
        }
    }
    return table;
}


TPreprocessor_addClassNoise::TPreprocessor_addClassNoise(double const cn)
: proportion(cn)
{}


PExampleTable TPreprocessor_addClassNoise::operator()(
    PExampleTable const &data, Result result)
{
    if (result == Default) {
        result = Copy;
    }
    checkChangeReference(result);
    if (!data->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if (data->domain->classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError, "class '%s' is not discrete",
            data->domain->classVar->cname());
    }
    if ((proportion < 0.0) || (proportion > 1.0)) {
        raiseError(PyExc_ValueError, "invalid proportion (%.3f)", proportion);
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if (proportion > 0.0) {
        TMakeRandomIndicesN mri;
        mri.randomGenerator = randomGenerator;
        addNoise(table->domain->attributes->size(), proportion, mri, table);
    }
    return table;
}



TPreprocessor_addGaussianClassNoise::TPreprocessor_addGaussianClassNoise(
    double const dev)
: deviation(dev)
{}


PExampleTable TPreprocessor_addGaussianClassNoise::operator()(
    PExampleTable const &data, Result result)
{
    checkChangeReference(result);
    if (!data->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if (data->domain->classVar->varType != TVariable::Continuous) {
        raiseError(PyExc_ValueError, "class '%s' is not continuous",
            data->domain->classVar->cname());
    }
    if (deviation < 0.0) {
        raiseError(PyExc_ValueError, "invalid deviation (%.3f)", deviation);
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    PExampleTable table = constructReturn(data, result);
    if (deviation>0.0) {
        TGenRand_11 rg(randomGenerator);    
        addGaussianNoise(data->domain->attributes->size(), deviation, rg, table);
    }
    return table;
}


TPreprocessor_addClassWeight::TPreprocessor_addClassWeight()
: classWeights(PFloatList(new TFloatList)),
  equalize(false)
{}


TPreprocessor_addClassWeight::TPreprocessor_addClassWeight(
    PFloatList const &cw, bool const eq)
: equalize(eq),
  classWeights(cw)
{}


PExampleTable TPreprocessor_addClassWeight::operator()(
    PExampleTable const &data, Result result)
{
    if (result == Default) {
        result = Reference;
    }
    if (!data->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    if (data->domain->classVar->varType != TVariable::Discrete) {
        raiseError(PyExc_ValueError, "class '%s' is not discrete",
            data->domain->classVar->cname());
    }
    int const nocl = data->domain->classVar->noOfValues();
    if (classWeights && classWeights->size() && (classWeights->size() != nocl)) {
        raiseError(PyExc_ValueError,
            "size of class_weights must match the number of classes");
    }
    PExampleTable table = constructReturn(data, result);
    vector<double> weights;
    if (equalize) {
        PDistribution dist(getClassDistribution(data));
        TDiscDistribution const *const ddist = 
            dynamic_cast<TDiscDistribution *>(dist.getPtr());
        TDiscDistribution::const_iterator db(ddist->begin()), de(ddist->end());
        TDiscDistribution::const_iterator di;
        if (ddist->size() > nocl) {
            raiseError(PyExc_IndexError,
                "data does not match the class descriptor "
                "(some values are out of range)");
        }
        if (ddist->abs == 0.0) {
            return table;
        }
        int noNullClasses = 0;
        for(di = db; di != de; di++) {
            if (*di>0.0) {
                noNullClasses++;
            }
        }
        const float N = ddist->abs / noNullClasses;
        if (classWeights && classWeights->size()) {
            TFloatList::const_iterator cwi(classWeights->begin());
            for(di = db; di != de; di++, cwi++) {
                weights.push_back(*di <= 0.0 ? 1.0 : *cwi * N / *di);
            }
        }
        else { // no class weights, only equalization
            for(di = db; di != de; di++) {
                weights.push_back(*di <= 0.0 ? 1.0 : N / *di);
            }
        }
    }
    else  {  // no equalization, only weights
        if (!classWeights || !classWeights->size()) {
            raiseError(PyExc_ValueError,
                "class_weights attribute must be set");
        }
        weights.assign(classWeights->begin(), classWeights->end());
    }

    PEITERATE(ei, table) {
        ei.setWeight(ei.getWeight() * weights[int(ei.getClass())]);
    }
    return table;
}

