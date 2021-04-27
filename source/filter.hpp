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


#ifndef __FILTER_HPP
#define __FILTER_HPP

#include "filter.ppp"


class TFilter : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Filter);

    bool negate; //P if true, filter output should be negated.
    PDomain domain; //P domain to which the examples are converted (if needed)

    TFilter(bool const=false);
    TFilter(PDomain const &, bool const=false);
    virtual bool operator()(PExample const &);
    virtual bool operator()(TExample const * const)=0;
    virtual PExampleTable operator()(PExampleTable const &);
    virtual PFilter deepCopy() const;

    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(instance); select or reject the example");
    static PyObject *__and__(PyObject *self, PyObject *other);
    static PyObject *__or__(PyObject *self, PyObject *other);
};


typedef TOrangeVector<PFilter, TWrappedReferenceHandler<PFilter>, &OrFilterList_Type> TFilterList;
PYVECTOR(FilterList, Filter)


class TFilter_random : public TFilter {
public:
    __REGISTER_CLASS(Filter_random);
    NEW_WITH_CALL(Filter);     

    double prob; //P probability of selecting an example
    PRandomGenerator randomGenerator; //P random generator

    TFilter_random(
        double const =0.5,
        bool const=false,
        PRandomGenerator const & =PRandomGenerator());
    virtual bool operator()(TExample const *const );
};


class TFilter_hasSpecial : public TFilter {
public:
    __REGISTER_CLASS(Filter_hasSpecial);
    NEW_WITH_CALL(Filter);

    TFilter_hasSpecial(bool const=false);
    TFilter_hasSpecial(PDomain const &, bool const=false);
    virtual bool operator()(TExample const *const);
};


class TFilter_isDefined : public TFilter {
public:
    __REGISTER_CLASS(Filter_isDefined);
    NEW_WITH_CALL(Filter);

    PAttributedBoolList check; //P tells which attributes to check; checks all if the list is empty

    TFilter_isDefined(bool const=false);
    TFilter_isDefined(PDomain const &, bool const=false);
    virtual bool operator()(TExample const *const);

    bool isDefined_noConversion(TExample const *const) const;

    static int __set__domain(PyObject *self, PyObject *val);
};


class TFilter_hasMeta: public TFilter {
public:
    __REGISTER_CLASS(Filter_hasMeta);
    NEW_WITH_CALL(Filter);

    TMetaId id; //P meta attribute id

    TFilter_hasMeta(TMetaId const anid = 0, bool const = false);
    virtual bool operator()(TExample const *const);
};


class TFilter_hasClassValue : public TFilter {
public:
    __REGISTER_CLASS(Filter_hasClassValue);
    NEW_WITH_CALL(Filter);

    TFilter_hasClassValue(bool const=false);
    TFilter_hasClassValue(PDomain const &, bool const=false);
    virtual bool operator()(TExample const *const);
};


class TFilter_sameValue : public TFilter {
public:
    __REGISTER_CLASS(Filter_sameValue);
    NEW_WITH_CALL(Filter);

    TAttrIdx position; //P position of the observed attribute
    TValue value; //P value that the selected examples should have

    TFilter_sameValue(
        TValue const=UNDEFINED_VALUE,
        TAttrIdx const=-1,
        bool const=false,
        PDomain const & =PDomain());

    virtual bool operator()(TExample const *const);
};


class TValueFilter : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(ValueFilter);

    enum Operator { None, Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual, Between, Outside, Contains, NotContains, BeginsWith, EndsWith, Listed } PYCLASSCONSTANTS;

private:
    // TODO: this is a memory leak: variable does not participate in cyclic gc!!!
    PVariable variable;
    mutable TAttrIdx lastPosition;
    mutable int lastDomainVersion;

public:
    int acceptMissing; //P tells whether missing values are accepted (1), rejected (0) or ignored (-1)

    TValueFilter(int const accUndef=0);
    inline PVariable const &getVariable() const;
    inline void setVariable(PVariable const &);
    inline TAttrIdx getVarPos(PDomain const &) const;

    virtual int operator()(TExample const *const) const = 0; // Returns 1 for accept, 0 for reject, -1 for ignore
    virtual PValueFilter deepCopy() const;

    static Operator operatorFromPy(int op);

    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepares arguments for unpickling");
    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("(instance); select or reject the example");
    static PyObject *__get__variable(OrValueFilter *self);
    static int __set__variable(OrValueFilter *self, PyObject *value);
};



class TValueFilter_continuous : public TValueFilter {
public:
    __REGISTER_CLASS(ValueFilter_continuous);

    TValue min; //P (+ref) reference value (lower bound for interval operators)
    TValue max; //P upper bound for interval operators
    bool outside; //P obsolete: if true, the filter accepts the values outside the interval, not inside
    int oper; //P(&ValueFilter::Operator) operator

    TValueFilter_continuous(
        PVariable const &,
        TValue const min=0.0, TValue const max=0.0,
        bool const outs = false, int const accs = 0);

    TValueFilter_continuous(
        PVariable const &,
        int const op,
        TValue const min=0.0, TValue const max=0.0, 
        int const accs = 0);

    inline virtual void setVariable(PVariable const &);
    virtual int operator()(TExample const *const) const;
    virtual PValueFilter deepCopy() const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, oper, min, max) or (variable, oper, ref)");
};


class TValueFilter_discrete : public TValueFilter {
public:
    __REGISTER_CLASS(ValueFilter_discrete);
    NEW_WITH_CALL(ValueFilter);

    PValueList values; //P accepted values
    bool negate; //P negate

    TValueFilter_discrete(
        PVariable const &,
        PValueList const & = PValueList(),
        int const accs = 0,
        bool const negate = false);

    inline virtual void setVariable(PVariable const &);
    virtual int operator()(TExample const *const) const;
    virtual PValueFilter deepCopy() const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, values)");
};


class TValueFilter_string : public TValueFilter {
public:
    __REGISTER_CLASS(ValueFilter_string);
    NEW_WITH_CALL(ValueFilter);

    string min; //P (+ref) reference value (lower bound for interval operators)
    string max; //P upper bound for interval operators
    int oper;   //P(&ValueFilter::Operator) operator
    bool caseSensitive; //P if true (default), the operator is case sensitive

    TValueFilter_string(
        PVariable const &,
        int const op,
        string const &min,
        string const &max,
        int const accs = 0,
        bool const csens = true);

    inline virtual void setVariable(PVariable const &);
    virtual int operator()(TExample const *const) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, oper, min, max) or (variable, oper, ref)");
};


class TValueFilter_stringList : public TValueFilter {
public:
    __REGISTER_CLASS(ValueFilter_stringList);
    NEW_WITH_CALL(ValueFilter);

    PStringList values; //P accepted values
    bool negate; //P if false, it matches values not in the list
    bool caseSensitive; //P if true (default), the comparison is case sensitive

    TValueFilter_stringList(
        PVariable const &,
        PStringList const &,
        int const accs = 0,
        bool const negate = false,
        const bool csens = true);

    inline virtual void setVariable(PVariable const &);
    virtual int operator()(TExample const *const) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(variable, values)");
};


typedef TOrangeVector<PValueFilter,
                      TWrappedReferenceHandler<PValueFilter>,
                      &OrValueFilterList_Type> TValueFilterList;

PYVECTOR(ValueFilterList, ValueFilter)


class TFilter_values : public TFilter {
public:
    __REGISTER_CLASS(Filter_values);
    NEW_WITH_CALL(Filter);

    enum Operator { None, Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual, Between, Outside, Contains, NotContains, BeginsWith, EndsWith, Listed } PYCLASSCONSTANTS;

    PValueFilterList conditions; //P a list of filters

    /*  If conjunction == true, example is chosen if no values are rejected
    If conjunction == false, example is chosen if at least one value is accepted
    The above rules apply also when no values could be tested (think how :)

    negate is applied to whole expression, not to individual terms */

    bool conjunction; //P if true, filter computes conjunction, otherwise disjunction

    TFilter_values(
        bool const conjunc=true,
        bool const aneg=false,
        PDomain const & =PDomain());

    TFilter_values(
        PValueFilterList const &,
        bool const conjunct,
        bool const aneg=false,
        PDomain const & =PDomain());

    virtual bool operator()(TExample const *const);
    virtual PFilter deepCopy() const;

    TValueFilterList::iterator findCondition(PVariable const &var);

    void updateCondition(PValueFilter const &);

    void addCondition(PVariable const &, TValue const, bool const negate=false);
    void addCondition(PVariable const &, PValueList const &, bool const negate=false);
    void addCondition(PVariable const &, int const oper, TValue const min, TValue const max=0.0);
    void addCondition(PVariable const &, int const oper, string const &, string const &);
    void addCondition(PVariable const &, PStringList const &, bool const negate=false);
    void removeCondition(PVariable const &);
};


class TFilter_sameExample : public TFilter {
public:
    __REGISTER_CLASS(Filter_sameExample);
    NEW_WITH_CALL(Filter);

    PExample example; //P example with which examples are compared

    TFilter_sameExample(bool const aneg=false);
    TFilter_sameExample(PExample const &, bool const aneg=false);
    virtual bool operator()(TExample const *const);
};


class TFilter_compatibleExample : public TFilter {
public:
    __REGISTER_CLASS(Filter_compatibleExample);
    NEW_WITH_CALL(Filter);

    PExample example; //P example with which examples are compared

    TFilter_compatibleExample(bool const aneg=false);
    TFilter_compatibleExample(PExample const &, bool const aneg=false);
    virtual bool operator()(TExample const *const);
};


class TFilter_conjunction : public TFilter {
public:
    __REGISTER_CLASS(Filter_conjunction);
    NEW_WITH_CALL(Filter);

    PFilterList filters; //P a list of filters;

    TFilter_conjunction();
    TFilter_conjunction(PFilterList const &);
    virtual bool operator()(TExample const *const);
};


class TFilter_disjunction : public TFilter {
public:
    __REGISTER_CLASS(Filter_disjunction);
    NEW_WITH_CALL(Filter);

    PFilterList filters; //P a list of filters;

    TFilter_disjunction();
    TFilter_disjunction(PFilterList const &);
    virtual bool operator()(TExample const *const);
};


PVariable const &TValueFilter::getVariable() const
{
    return variable;
}

void TValueFilter::setVariable(PVariable const &var)
{
    if (!var) {
        raiseError(PyExc_ValueError, "ValueFilter.variable must be defined");
    }
    lastDomainVersion = -1;
    variable = var;
}

void TValueFilter_discrete::setVariable(PVariable const &var)
{
    if (var && (var->varType != TVariable::Discrete)) {
        raiseError(PyExc_TypeError,
            "variable '%s' is not discrete", var->cname());
    }
    TValueFilter::setVariable(var);
}

void TValueFilter_continuous::setVariable(PVariable const &var)
{
    if (var && (var->varType != TVariable::Continuous)) {
        raiseError(PyExc_TypeError,
            "variable '%s' is not continuous", var->cname());
    }
    TValueFilter::setVariable(var);
}

void TValueFilter_string::setVariable(PVariable const &var)
{
    if (var && (var->varType != TVariable::String)) {
        raiseError(PyExc_TypeError,
            "'%s' is not a string variable", var->cname());
    }
    TValueFilter::setVariable(var);
}

void TValueFilter_stringList::setVariable(PVariable const &var)
{
    if (var && (var->varType != TVariable::String)) {
        raiseError(PyExc_TypeError,
            "'%s' is not a string variable", var->cname());
    }
    TValueFilter::setVariable(var);
}

TAttrIdx TValueFilter::getVarPos(PDomain const &domain) const
{
    if (domain->version == lastDomainVersion) {
        return lastPosition;
    }
    // this will throw exception if needed
    lastPosition = domain->getVarNum(variable);
    // set lastDomainVersion only if no exception;
    lastDomainVersion = domain->version;
    return lastPosition;
}

#endif
