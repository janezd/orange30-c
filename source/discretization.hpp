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


#ifndef __DISCRETIZATION_HPP
#define __DISCRETIZATION_HPP

#include "discretization.ppp"


class TDiscretizer : public TTransformValue {
public:
    __REGISTER_ABSTRACT_CLASS(Discretizer);

    virtual PDiscreteVariable constructVar(PContinuousVariable const &,
                                           TValue const mindiff =1.0)=0;
    virtual void getCutoffs(vector<TValue> &cutoffs) const = 0;

    PyObject *construct_variable(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(variable[, min_diff]); return a discretized variable");
};



class TEqualWidthDiscretizer : public TDiscretizer {
public:
    __REGISTER_CLASS(EqualWidthDiscretizer);

    int   nIntervals; //P(+n) number of intervals
    TValue firstCut; //P the first cut-off point
    TValue step; //P step (width of interval)

    TEqualWidthDiscretizer(int const=-1, TValue const=-1.0, TValue const=-1.0);

    virtual void transform(TValue &) const;
    virtual PDiscreteVariable constructVar(PContinuousVariable const &,
                                           TValue const mindiff=1.0);
    virtual void getCutoffs(vector<TValue> &cutoffs) const;

    NEW_NOARGS;
    static PyObject *__get__points(OrEqualWidthDiscretizer *self);
};


class TThresholdDiscretizer : public TDiscretizer {
public:
    __REGISTER_CLASS(ThresholdDiscretizer);

    TValue threshold; //P threshold

    TThresholdDiscretizer(TValue const threshold = 0.0);
    virtual void transform(TValue &) const;
    virtual PDiscreteVariable constructVar(PContinuousVariable const &,
                                           TValue const mindiff=1.0);
    virtual void getCutoffs(vector<TValue> &cutoffs) const;

    NEW_NOARGS;
    static PyObject *__get__points(OrThresholdDiscretizer *self);
};


class TIntervalDiscretizer : public TDiscretizer  {
public:
    __REGISTER_CLASS(IntervalDiscretizer);
    NEW_NOARGS;

    PFloatList points; //P cut-off points

    TIntervalDiscretizer();
    TIntervalDiscretizer(PFloatList const &apoints);
    virtual void transform(TValue &) const;
    PDiscreteVariable constructVar(PContinuousVariable const &var,
                                   TValue const mindiff=1.0);
    virtual void getCutoffs(vector<TValue> &cutoffs) const;

};



class TDiscretization : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Discretization);

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PExampleTable const &) const=0;

    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(variable, data); return a discretized variable");
};


class TEqualWidthDiscretization : public TDiscretization {
public:
    __REGISTER_CLASS(EqualWidthDiscretization);
    NEW_NOARGS;

    int nIntervals; //P(+n) number of intervals

    TEqualWidthDiscretization(int const anumber=4);

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PExampleTable const &) const;

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PBasicAttrStat const &) const;

};


class TEqualFreqDiscretization : public TDiscretization {
public:
    __REGISTER_CLASS(EqualFreqDiscretization);
    NEW_NOARGS;

    int nIntervals; //P(+n) number of intervals

    TEqualFreqDiscretization(int anumber =4);

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PExampleTable const &) const;

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PContDistribution const &) const;

    void cutoffsByMidpoints(PIntervalDiscretizer const &,
                            PContDistribution const &distr,
                            TValue &mindiff) const;
    void cutoffsByCounting(PIntervalDiscretizer const &,
                           PContDistribution const &,
                           TValue &mindiff) const;
};


class TEntropyDiscretization : public TDiscretization {
public:
    __REGISTER_CLASS(EntropyDiscretization);
    NEW_NOARGS;

    bool forceAttribute; //P minimal number of intervals; default = 0 (no limits)

    TEntropyDiscretization();

    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PExampleTable const &) const;

/*    virtual PDiscreteVariable operator()(
        PContinuousVariable const &, PContDistribution const &) const;
*/
private:
    typedef map<TValue, TDiscDistribution> TS;
    virtual PDiscreteVariable operator()(TS const &,
                                         TDiscDistribution const &,
                                         PContinuousVariable const &) const;

    void divide(TS::const_iterator const &, TS::const_iterator const &,
                TDiscDistribution const &,
                double const entropy,
                int const k,
                vector<pair<double, double> > &,
                TSimpleRandomGenerator &rgen,
                double &mindiff) const;
};


class TDomainDiscretization : public TOrange {
public:
    __REGISTER_CLASS(DomainDiscretization);

    PDiscretization discretization; //P discretization

    TDomainDiscretization(PDiscretization const & =PDiscretization());
    virtual PDomain operator()(PExampleTable const &) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(discretization[, data]); construct a domain discretizer or a discretized domain");
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(data); construct a discretized domain");

protected:
    PDomain equalWidthDomain(PExampleTable const &gen) const;
    PDomain equalFreqDomain(PExampleTable const &gen) const;
//    PDomain entropyDomain(PExampleTable const &gen) const;
    PDomain otherDomain(PExampleTable const &gen) const;
};

#endif
