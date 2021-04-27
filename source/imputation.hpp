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


#ifndef __IMPUTATION_HPP
#define __IMPUTATION_HPP

#include "imputation.ppp"

class TTransformValue_IsDefined : public TTransformValue
{
public:
    __REGISTER_CLASS(TransformValue_IsDefined);
    NEW_NOARGS;
    inline virtual void transform(TValue &val) const;
};


class TTransformValue_NanValue : public TTransformValue
{
public:
    __REGISTER_CLASS(TransformValue_NanValue);
    TValue valueForNan; //P value for imputed instead of NaN

    TTransformValue_NanValue(TValue const &);
    inline virtual void transform(TValue &val) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(value)");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
};


class TImputer : public TOrange
{
public:
    __REGISTER_ABSTRACT_CLASS(Imputer);

    PDomain domain; //P domain to which examples are converted (usually NULL)

    inline virtual PExample operator()(TExample const &);
    virtual PExampleTable operator()(PExampleTable const &);

    virtual void impute(TExample &) = 0;

    inline static void imputeDefaults(
        TExample &example, PExample const &defaults);
};


class TImputer_constant : public TImputer
{
public:
    __REGISTER_CLASS(Imputer_constant);

    PExample values; //P values that are inserted instead of missing ones

    TImputer_constant(PDomain const &);
    TImputer_constant(PExample const &);
    TImputer_constant(TExample const &);

    inline virtual void impute(TExample &);
    inline virtual PExample operator()(TExample const &);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(domain or example); construct the imputer");
};


class TImputer_model : public TImputer
{
public:
    __REGISTER_CLASS(Imputer_model);
    NEW_NOARGS;

    PClassifierList models; //P classifiers
    virtual void impute(TExample &example);
};


class TImputer_random : public TImputer
{
public:
    __REGISTER_CLASS(Imputer_random);
    NEW_NOARGS;

    bool imputeClass;   //P Tells whether to impute the class values, too (default: true)
    bool deterministic; //P tells whether to initialize random by example's CRC (default: false)
    PDistributionList distributions; //P probability functions

    TImputer_random(bool const imputeClass=true, bool const deterministic=false);
    TImputer_random(bool const impCls, bool const dtrmin, PDistributionList const &);
    virtual void impute(TExample &example);

private:
    TRandomGenerator randgen;
};



class TImputerConstructor : public TOrange
{
public:
    __REGISTER_ABSTRACT_CLASS(ImputerConstructor);

    bool imputeClass; //P tells whether to impute the class value (default: true)

    TImputerConstructor();
    virtual PImputer operator()(PExampleTable const &) = 0;

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(data) -> imputer");
};


class TImputerConstructor_constant : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_constant);
    NEW_WITH_CALL(ImputerConstructor);

    PExample defaults; //P default values to be imputed instead missing ones
    virtual PImputer operator()(PExampleTable const &);
};


class TImputerConstructor_average : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_average);
    NEW_WITH_CALL(ImputerConstructor);
    virtual PImputer operator()(PExampleTable const &);
};


class TImputerConstructor_minimal : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_minimal);
    NEW_WITH_CALL(ImputerConstructor);
    virtual PImputer operator()(PExampleTable const &);
};


class TImputerConstructor_maximal : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_maximal);
    NEW_WITH_CALL(ImputerConstructor);
    virtual PImputer operator()(PExampleTable const &);
};


class TImputerConstructor_asValue : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_asValue);
    NEW_WITH_CALL(ImputerConstructor);
    bool ignoreContinuous; //P don't add indicators for continuous variables

    TImputerConstructor_asValue();
    virtual PImputer operator()(PExampleTable const &);
    static PVariable createImputedVar(PVariable const &);
};


class TImputerConstructor_model : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_model);
    NEW_WITH_CALL(ImputerConstructor);
    PLearner learnerDiscrete; //P learner for discrete attributes
    PLearner learnerContinuous; //P learner for continuous attributes

    bool useClass; //P tells whether to use class value in imputation (default: false)

    TImputerConstructor_model();
    virtual PImputer operator()(PExampleTable const &);
};


class TImputerConstructor_random : public TImputerConstructor
{
public:
    __REGISTER_CLASS(ImputerConstructor_random);
    NEW_WITH_CALL(ImputerConstructor);
    bool deterministic; //P tells whether to initialize random by example's CRC (default: false)

    TImputerConstructor_random(const bool deterministic = false);
    virtual PImputer operator()(PExampleTable const &);
};



void TTransformValue_IsDefined::transform(TValue &val) const
{
    val = isnan(val) ? 1 : 0;
}


void TTransformValue_NanValue::transform(TValue &val) const
{
    if (isnan(val)) {
        val = valueForNan;
    }
}


PExample TImputer::operator()(TExample const &example)
{
    PExample ex(
        domain ? example.convertedTo(domain) : TExample::constructCopy(example));
    impute(*ex);
    return ex;
}


void TImputer::imputeDefaults(TExample &example, PExample const &defaults)
{ 
    TExample::const_iterator ei(defaults->begin());
    TExample::iterator oi(example.begin()), oe(example.end());
    for(; oi != oe; oi++, ei++) {
        if (isnan(*oi) && !isnan(*ei)) {
            *oi = *ei;
        }
    }
}


PExample TImputer_constant::operator()(TExample const &example)
{
    if (domain && (domain != values->domain)) {
        raiseError(PyExc_ValueError,
            "imputer's domain does not match the default values");
    }
    return TImputer::operator()(example);
}


void TImputer_constant::impute(TExample &example)
{
    if (example.domain != values->domain) {
        raiseError(PyExc_ValueError,
            "imputer's domain does not match the domain of example");
    }
    imputeDefaults(example, values);
}



#endif
