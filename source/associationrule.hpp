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


#ifndef _ASSOCIATIONRULE_HPP
#define _ASSOCIATIONRULE_HPP

#include "associationrule.ppp"

class TAssociationRule : public TOrange {
public:
    __REGISTER_CLASS(AssociationRule);

    PExample left; //PR left side of the rule
    PExample right; //PR right side of the rule
    double support; //P support for the rule
    double confidence; //P confidence of the rule
    double coverage; //P rule's coverage
    double strength; //P rule's strength
    double lift; //P rule's lift
    double leverage; //P rule's leverage
    double nAppliesLeft; //P number of examples covered by the rule's left side
    double nAppliesRight; //P number of examples covered by the rule's right side
    double nAppliesBoth; //P number of examples covered by the rule
    double nExamples; //P number of learning examples
    int nLeft; //PR number of items on the rule's left side
    int nRight; //PR number of items on the rule's right side

    PExampleTable examples; //PR examples which the rule was built from
    PIntList matchLeft; //PR indices of examples that match the left side of the rule
    PIntList matchBoth; //PR indices to examples that match both sides of the rule

    inline TAssociationRule(
        PExample const &, PExample const &);

    inline TAssociationRule(
        PExample const &al, PExample const &ar,
        double const napLeft, double const napRight,
        double const napBoth, double const nExamples,
        int const anleft=-1, int const anright=-1);

    inline bool operator ==(const TAssociationRule &other) const;

    static bool applies(const TExample &ex, const PExample &side);
    inline bool appliesLeft(const TExample &ex) const;
    inline bool appliesRight(const TExample &ex) const;
    inline bool appliesBoth(const TExample &ex) const;
    static int countItems(PExample const &ex);

    static string side2string(PExample const &ex);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(left, right, support, confidence)");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    PyObject *py_appliesLeft(PyObject *arg, PyObject *kw) const PYARGS(METH_O, "(instance) -> bool");
    PyObject *py_appliesRight(PyObject *arg, PyObject *kw) const PYARGS(METH_O, "(instance) -> bool");
    PyObject *py_appliesBoth(PyObject *arg, PyObject *kw) const PYARGS(METH_O, "(instance) -> bool");
    PyObject *__repr__() const;
    PyObject *__str__() const;
    PyObject *__richcmp__(PyObject *other, int op) const;
};


typedef TOrangeVector<PAssociationRule,
    TWrappedReferenceHandler<PAssociationRule>, &OrAssociationRules_Type>
    TAssociationRules;

PYVECTOR(AssociationRules, AssociationRule)


TAssociationRule::TAssociationRule(PExample const &al, PExample const &ar)
    : left(al),
    right(ar),
    support(0.0),
    confidence(0.0),
    coverage(0.0),
    strength(0.0),
    lift(0.0),
    leverage(0.0),
    nAppliesLeft(0),
    nAppliesRight(0),
    nAppliesBoth(0),
    nExamples(0),
    nLeft(countItems(al)),
    nRight(countItems(ar))
{}


TAssociationRule::TAssociationRule(
    PExample const &al, PExample const &ar,
    double const napLeft, double const napRight, double const napBoth,
    double const nExamples, int const anleft, int const anright)
: left(al),
  right(ar), 
  support(napBoth/nExamples),
  confidence(napBoth/napLeft),
  coverage(napLeft/nExamples),
  strength(napRight/napLeft),
  lift(nExamples * napBoth /napLeft / napRight),
  leverage((napBoth*nExamples - napLeft*napRight)/nExamples/nExamples),
  nAppliesLeft(napLeft),
  nAppliesRight(napRight),
  nAppliesBoth(napBoth),
  nExamples(nExamples),
  nLeft(anleft < 0 ? countItems(al) : anleft),
  nRight(anright < 0 ? countItems(ar) : anright)
{}

bool TAssociationRule::operator ==(TAssociationRule const &other) const
{ return !left->cmp(*other.left) && !right->cmp(*other.right); }


bool TAssociationRule::appliesLeft(const TExample &ex) const
{ return applies(ex, left); }


bool TAssociationRule::appliesRight(const TExample &ex) const
{ return applies(ex, right); }


bool TAssociationRule::appliesBoth(const TExample &ex) const
{ return applies(ex, left) && applies(ex, right); }


#endif
