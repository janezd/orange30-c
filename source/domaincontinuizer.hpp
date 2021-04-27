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


#ifndef __DOMAINCONTINUIZER_HPP
#define __DOMAINCONTINUIZER_HPP

#include "domaincontinuizer.ppp"


class TDiscrete2Continuous : public TTransformValue {
public:
    __REGISTER_CLASS(Discrete2Continuous);

    int value; //P target value
    bool invert; //P give 1.0 to values not equal to the target
    bool zeroBased; //P if true (default) it gives values 0.0 and 1.0; else -1.0 and 1.0, 0.0 for undefined

    TDiscrete2Continuous(
        int const =-1, bool const invert = false, bool const zeroBased = true);
    virtual void transform(TValue &) const;
};


class TOrdinal2Continuous : public TTransformValue {
public:
    __REGISTER_CLASS(Ordinal2Continuous);

    double factor; //P multiplier

    TOrdinal2Continuous(double const=1.0);
    virtual void transform(TValue &) const;
};


class TNormalizeContinuous : public TTransformValue {
public:
    __REGISTER_CLASS(NormalizeContinuous);

    double average; //P the average value
    double span; //P the value span

    TNormalizeContinuous(double const=0.0, double const=0.0);
    virtual void transform(TValue &) const;
};


class TDomainContinuizer : public TOrange {
public:
    __REGISTER_CLASS(DomainContinuizer);

    enum MultinomialTreatment {LowestIsBase, FrequentIsBase, NValues, Ignore, IgnoreAllDiscrete, ReportError, AsOrdinal, AsNormalizedOrdinal} PYCLASSCONSTANTS;
    enum ContinuousTreatment {Leave, NormalizeBySpan, NormalizeByVariance} PYCLASSCONSTANTS;
    /* Trick pyprops - C++ code uses the same as MultinomialTreatment, these are just aliases:
    enum ClassTreatment {LeaveUnlessTarget=3, ErrorIfCannotHandle=5, AsOrdinal=6} PYCLASSCONSTANTS;
    */

    bool zeroBased; //P if true (default) it gives values 0.0 and 1.0; else -1.0 and 1.0, 0.0 for undefined
    int continuousTreatment; //P(&ContinuousTreatment) treatment of continuous attributes
    int multinomialTreatment; //P(&MultinomialTreatment) treatment of multinomial attributes
    int classTreatment; //P(&ClassTreatment) treatment of the class

    TDomainContinuizer();

    PVariable discrete2continuous(PDiscreteVariable const &var,
        int const val, bool const inv = false) const;
    void discrete2continuous(PDiscreteVariable const &var, TVarList &vars,
        int const mostFrequent) const;
    PVariable discreteClass2continous(PDiscreteVariable const &classVar,
        int const targetClass) const;
    PVariable continuous2normalized(PContinuousVariable const &var,
        double const avg, double const span) const;
    PVariable ordinal2continuous(PDiscreteVariable const &var,
        double const factor) const;

    PDomain operator()(PDomain const &, int const targetClass = -1) const;
    PDomain operator()(PExampleTable const &, int const targetClass = -1) const;

    NEW_NOARGS;
    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("(data[, target_class]); return a domain with continuized variables");
};

#endif

