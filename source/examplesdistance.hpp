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


#ifndef __EXAMPLESDISTANCE_HPP
#define __EXAMPLESDISTANCE_HPP

#include "examplesdistance.ppp"
#include "basicattrstat.hpp"

class TExample;
class TExampleTable;
class TDomainBasicAttrStat;
class TDomainDistributions;
class TExamplesDistance;

class TExamplesDistanceConstructor : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(ExamplesDistanceConstructor)

    bool ignoreClass; //P if true (default), class value is ignored when computing distances

    TExamplesDistanceConstructor(bool const ignoreClass = true);
    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const =0;

    PyObject *__call__(PyObject *args, PyObject *kw) const PYDOC("([data, distributions, basic_stat]) -> double; compute distance between a pair of examples");
};

class TExamplesDistance : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(ExamplesDistance)

    virtual double operator()(TExample const *const, TExample const *const) const =0;

    PyObject *__call__(PyObject *args) const PYDOC("(instance1, instance2) -> double; compute distance between a pair of examples");
};



/* An abstract objects which returns 'normalized' distance between two examples.
   'ranges' stores
      1/attribute_range for continuous attributes
      1/number_of_values for ordinal attributes
      -1.0 for nominal attribute
      0 if attribute is to be ignored (this can happen for various reasons,
          such as continuous attribute with no known values)
   When computing "difs", it returns a vector that contains
      abs(ex1[i]-ex2[i]) * ranges[i] for continuous and ordinal attributes
      0 or 1 for nominal attributes
   Distance between two values can be greater than 1!
*/
class TExamplesDistanceConstructor_Normalized : public TExamplesDistanceConstructor {
public:
    __REGISTER_ABSTRACT_CLASS(ExamplesDistanceConstructor_Normalized)

    bool normalize; //P tells whether to normalize distances between attributes
    bool ignoreUnknowns; //P if true (default: false) unknown values are ignored in computation

    TExamplesDistanceConstructor_Normalized(
        bool const ic=true, bool const norm=true, bool const iu=false);
};

class TExamplesDistance_Normalized : public TExamplesDistance {
public:
  __REGISTER_ABSTRACT_CLASS(ExamplesDistance_Normalized)

  PVarList attributes; //P a list of attributes
  PAttributedFloatList normalizers; //P normalizing factors for attributes
  PAttributedFloatList bases; //P lowest values for attributes
  PAttributedFloatList averages; //P average values for continuous attribute values
  PAttributedFloatList variances; //P variations for continuous attribute values
  int domainVersion; //P version of domain on which the ranges were computed
  bool normalize; //P tells whether to normalize distances between attributes
  bool ignoreUnknowns; //P if true (default: false) unknown values are ignored in computation

  TExamplesDistance_Normalized(
      bool const ic=true, bool const no=true, bool const iu=false,
      PExampleTable const & =PExampleTable(),
      PDomainDistributions const & =PDomainDistributions(),
      PDomainBasicAttrStat const & =PDomainBasicAttrStat());

  void getDifs(TExample const *const, TExample const *const, vector<double> &difs) const;
  void getNormalized(TExample const *const e1, vector<double> &normalized) const;
};



class TExamplesDistanceConstructor_Hamming : public TExamplesDistanceConstructor {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Hamming)
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    bool ignoreClass; //P if true (default), class value is ignored when computing distances
    bool ignoreUnknowns; //P if true (default: false) unknown values are ignored in computation

    TExamplesDistanceConstructor_Hamming(bool const ic=true, bool const ui=false);

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};

class TExamplesDistance_Hamming : public TExamplesDistance {
public:
    __REGISTER_CLASS(ExamplesDistance_Hamming)
    NEW_NOARGS

    bool ignoreClass; //P if true (default), class value is ignored when computing distances
    bool ignoreUnknowns; //P if true (default: false) unknown values are ignored in computation

    TExamplesDistance_Hamming(bool const ic=true, bool const iu=false);
    virtual double operator()(TExample const *const, TExample const *const) const;
};



class TExamplesDistanceConstructor_Maximal : public TExamplesDistanceConstructor_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Maximal)
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};


class TExamplesDistance_Maximal : public TExamplesDistance_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistance_Maximal)
    NEW_NOARGS

    TExamplesDistance_Maximal(
        bool const ic=true, bool const no=true, bool const iu=false,
        PExampleTable const & =PExampleTable(),
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat());

    virtual double operator()(TExample const *const, TExample const *const) const;
};


class TExamplesDistanceConstructor_Manhattan : public TExamplesDistanceConstructor_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Manhattan)
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};


class TExamplesDistance_Manhattan : public TExamplesDistance_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistance_Manhattan)
    NEW_NOARGS

    TExamplesDistance_Manhattan(
        bool const ic=true, bool const no=true, bool const iu=false,
        PExampleTable const & =PExampleTable(),
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat());

    virtual double operator()(TExample const *const, TExample const *const) const;
};



class TExamplesDistanceConstructor_Euclidean : public TExamplesDistanceConstructor_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Euclidean)
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};

class TExamplesDistance_Euclidean : public TExamplesDistance_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistance_Euclidean)
    NEW_NOARGS
    PDomainDistributions distributions; //P distributions (of discrete attributes only)
    PAttributedFloatList bothSpecialDist; //P distances between discrete attributes if both values are unknown

    TExamplesDistance_Euclidean(
        bool const ic=true, bool const no=true, bool const iu=false,
        PExampleTable const & =PExampleTable(),
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat());
    virtual double operator()(TExample const *const, TExample const *const) const;
};


/* Lp distance:
     p root of sum of distances raised to the power of p between corresponding attribute values.
     A generalization of Euclidean (p=2), Manhattan (p=1) and Maximal (p=inf)
*/

class TExamplesDistance_Lp : public TExamplesDistance_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistance_Lp);
    NEW_NOARGS;

    double p; //PR p
    TExamplesDistance_Lp(double p=1.0);
    TExamplesDistance_Lp(
        bool const ic, bool const no=true, bool const iu=false,
        PExampleTable const & =PExampleTable(),
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat(),
        double const p=1.0);

    virtual double operator()(TExample const *const, TExample const *const) const;
};


class TExamplesDistanceConstructor_Lp : public TExamplesDistanceConstructor_Normalized {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Lp);
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    double p; //P p
    TExamplesDistanceConstructor_Lp();

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};


class TExamplesDistanceConstructor_Relief : public TExamplesDistanceConstructor {
public:
    __REGISTER_CLASS(ExamplesDistanceConstructor_Relief)
    NEW_WITH_CALL(ExamplesDistanceConstructor)

    virtual PExamplesDistance operator()(
        PExampleTable const &,
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat()) const;
};

class TExamplesDistance_Relief : public TExamplesDistance {
public:
    __REGISTER_CLASS(ExamplesDistance_Relief)
    NEW_NOARGS
    PVarList variables; //P a list of attributes
    PDomainDistributions distributions; //P distributions of values of attributes
    PAttributedFloatList averages; //P average values of attributes
    PAttributedFloatList normalizations; //P ranges of values of attributes
    PAttributedFloatList bothSpecial; //P distance if both values of both attributes are undefined

    TExamplesDistance_Relief(
        PExampleTable const & =PExampleTable(),
        PDomainDistributions const & =PDomainDistributions(),
        PDomainBasicAttrStat const & =PDomainBasicAttrStat());

    virtual double operator()(TExample const *const, TExample const *const) const;
    virtual double operator()(TAttrIdx const attrNo,
                              TValue const v1, TValue const v2) const;
};

#endif

