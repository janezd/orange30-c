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


#ifndef __PREPROCESSORS_HPP
#define __PREPROCESSORS_HPP

#include "preprocessors.ppp"

class TPreprocessor : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Preprocessor);
    enum Result { Default, InPlace, Reference, Copy } PYCLASSCONSTANTS;
    virtual PExampleTable operator()(PExampleTable const &, Result=Default) = 0;
    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data[, result]) -> processed_data");

protected:
    inline PExampleTable constructReturn(PExampleTable const &, int const inplace);
    static void checkChangeReference(int const inplace);
};


class TPreprocessor_removeDuplicates : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_removeDuplicates);
    NEW_WITH_CALL(Preprocessor);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_dropMissing : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_dropMissing);
    NEW_WITH_CALL(Preprocessor);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_dropMissingClasses : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_dropMissingClasses);
    NEW_WITH_CALL(Preprocessor);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_shuffle : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_shuffle);
    NEW_WITH_CALL(Preprocessor);

    PVarList attributes; //P tells which attributes to shuffle
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_shuffle();
    TPreprocessor_shuffle(PVarList const &);

    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_addNoise : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addNoise);
    NEW_WITH_CALL(Preprocessor);

    double proportion; //P proportion of changed discrete values
    double deviation; //P deviation for continuous values
    bool includeClass; //P tells whether to also add noise to class
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addNoise(double const proportion=0, double const deviation=0,
                           bool const includeClass=false);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_addMissing : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addMissing);
    NEW_WITH_CALL(Preprocessor);

    double proportion; //P proportion of removed values
    bool includeClass; //P tells whether to also remove class values
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addMissing(double const proportion=0, 
                             bool const includeClass=false);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_addMissingClasses : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addMissingClasses);
    NEW_WITH_CALL(Preprocessor);

    double proportion; //P proportion of removed class values
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addMissingClasses(double const =0);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);

private:
    void addMissing(PExampleTable const &);
};


class TPreprocessor_addClassNoise : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addClassNoise);
    NEW_WITH_CALL(Preprocessor);

    double proportion; //P proportion of changed class values
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addClassNoise(double const = 0);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};


class TPreprocessor_addGaussianClassNoise : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addGaussianClassNoise);
    NEW_WITH_CALL(Preprocessor);

    double deviation; //P class deviation
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addGaussianClassNoise(double const =0);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};



class TPreprocessor_addClassWeight : public TPreprocessor {
public:
    __REGISTER_CLASS(Preprocessor_addClassWeight);
    NEW_WITH_CALL(Preprocessor);

    PFloatList classWeights; //P weights of examples of particular classes
    bool equalize; //P reweight examples to equalize class proportions
    PRandomGenerator randomGenerator; //P random number generator

    TPreprocessor_addClassWeight();
    TPreprocessor_addClassWeight(
        PFloatList const &weights, const bool preequalize = false);
    virtual PExampleTable operator()(PExampleTable const &, Result=Default);
};



PExampleTable TPreprocessor::constructReturn(
    PExampleTable const &data, int const inplace)
{
    switch (inplace) {
        case InPlace:
            return data;
        case Copy:
            return TExampleTable::constructCopy(data);
        case Reference:
        default:
            return TExampleTable::constructReference(data);
    }
}

#endif
