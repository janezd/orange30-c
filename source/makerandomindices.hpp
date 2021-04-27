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


#ifndef __MAKERANDOMINDICES_HPP
#define __MAKERANDOMINDICES_HPP

#include "makerandomindices.ppp"

/*! MakeRandomIndices is a base class for several classes that
    construct a list of random indices corresponding to examples,
    mostly for use in data sampling.

    To control the randomness and ensure repeatability of experiments,
    each instance of these classes has its own random generator that
    is initialized to a zero seed. Constructing new instances of
    MakeRandomIndices will thus always return the same indices.

    The classes' call operators can be given either a table of
    examples or the number of indices to generate. In the former case,
    indices can be stratified (e.g. class distribution for examples
    with any given index will be approximatelly the same) if the table
    has a (discrete) class.
*/
class TMakeRandomIndices : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(MakeRandomIndices);

    /*! A constant telling whether stratification is enabled
        (StratifiedIfPossible), disabled (NotStratified) or
        requested (Stratified). In the latter case, exception is
        raised if stratification is impossible, e.g. when constructing
        indices for regression data. */
    enum Stratification {StratifiedIfPossible=-1, NotStratified, Stratified} PYCLASSCONSTANTS;

    /*! Sets the stratification type (see #Stratification) */
    int stratified;  //P(&Stratification) requests stratified distributions
    /*! A seed for random generator; ignored if #randomGenerator is set */
    int randseed; //P a seed for random generator
    /*! Random generator used for construction of indices */
    PRandomGenerator randomGenerator; //P a random generator

    TMakeRandomIndices(int const stratified=StratifiedIfPossible, const int seed=-1);
    TMakeRandomIndices(int const stratified, PRandomGenerator const &);

    /*! Abstract operator that returns a list of indices with the
        given length */
    virtual PIntList operator()(int const n) const =0;
    /*! Abstract operator that return a list of indices for given
        examples */
    virtual PIntList operator()(PExampleTable const &) const =0;


    PyObject *__call__(PyObject *args, PyObject *kewords) PYDOC("(data | n); return a list of fold indices");
};


/*! Construct random indices for splitting into two subsamples. The
    list contains 0's and 1's with the given distribution. */
class TMakeRandomIndices2 : public TMakeRandomIndices {
public:
  __REGISTER_CLASS(MakeRandomIndices2)
  NEW_WITH_CALL(MakeRandomIndices)

    /*! Proportion or the number of 0's; values below or equal to 1
        are treated as the proportion and values above 1 are treated
        as the number of 0's. */
  double p0; //P a proportion or a number of 0's

  TMakeRandomIndices2(
      double const p0=1.0,
      int const stratified=StratifiedIfPossible,
      int const randseed=-1);

  TMakeRandomIndices2(
      double const p0,
      int const stratified,
      PRandomGenerator const &);

  virtual PIntList operator()(int const n) const;
  virtual PIntList operator()(PExampleTable const &) const;
};



/*  Prepares a vector of indices with given distribution. Similar to TMakeRandomIndices2, this object's constructor
    is given the size of vector and the required distribution of indices. */
class TMakeRandomIndicesN : public TMakeRandomIndices {
public:
  __REGISTER_CLASS(MakeRandomIndicesN)
  NEW_WITH_CALL(MakeRandomIndices)

  PFloatList p; //P probabilities of indices (last is 1-sum(p))

  TMakeRandomIndicesN(
      int const stratified=StratifiedIfPossible,
      int const randseed=-1);

  TMakeRandomIndicesN(
      PFloatList const &p,
      int const stratified=StratifiedIfPossible,
      int const randseed=-1);

  TMakeRandomIndicesN(
      int const stratified,
      PRandomGenerator const &);

  TMakeRandomIndicesN(
      PFloatList const &p, 
      int const stratified,
      PRandomGenerator const &);

  virtual PIntList operator()(int const n) const;
  virtual PIntList operator()(PExampleTable const &) const;
};


class TMakeRandomIndicesCV : public TMakeRandomIndices {
public:
  __REGISTER_CLASS(MakeRandomIndicesCV)
  NEW_WITH_CALL(MakeRandomIndices)

  int folds; //P number of folds

  TMakeRandomIndicesCV(
      int const folds=10,
      int const stratified=StratifiedIfPossible,
      int const randseed=-1);

  TMakeRandomIndicesCV(
      int const folds, 
      int const stratified,
      PRandomGenerator const &);

  virtual PIntList operator()(int const n) const;
  virtual PIntList operator()(PExampleTable const &) const;
};


#endif
