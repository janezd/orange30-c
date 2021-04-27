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
#include "associationrule.px"


int TAssociationRule::countItems(PExample const &ex)
{ 
    int res = 0;
    const_PITERATE(TExample, ei, ex) {
        if (!isnan(*ei)) {
            res++;
        }
    }
    return res;
}



bool TAssociationRule::applies(const TExample &example, const PExample &side)
{
    PExample ex(example.convertedTo(side->domain));
    if (ex->domain->variables->size()) {
        TExample::const_iterator exi(ex->begin()), sidei(side->begin());
        TExample::const_iterator const exe(ex->end());
        for(; exi != exe; exi++, sidei++) {
            if (!isnan(*sidei) && (isnan(*exi) || (*exi != *sidei))) {
                return false;
            }
        }
        return true;
    }
    else {
        ITERATE_METAS(*side, mr)
            if (mr.isPrimitive ? isnan(mr.value) : !mr.object) {
                continue;
            }
            TMetaValue mv = ex->getMetaIfExists(mr.id);
            if (!mv.id ||
                (mr.isPrimitive != mv.isPrimitive) ||
                (mr.isPrimitive ? (isnan(mv.value) || (mv.value != mr.value))
                   : (!mv.object ||
                   !PyObject_RichCompareBool(mv.object, mr.object, Py_EQ))
                )) {
                return false;
            }
        }
        return true;
    }
}



TOrange *TAssociationRule::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{ 
    if (PyTuple_Size(args) == 6) {
        PExample le, re;
        double nLeft, nRight, nBoth, nExamples;
        if (!PyArg_ParseTuple(args, "O&O&dddd",
            &PExample::argconverter, &le, &PExample::argconverter, &re,
            &nLeft, &nRight, &nBoth, &nExamples)) {
                return NULL;
        }
        return new TAssociationRule(le, re, nLeft, nRight, nBoth, nExamples);
    }

    if (PyTuple_Size(args) == 1) {
        PAssociationRule rule;
        if (!PyArg_ParseTuple(args, "O&", &PAssociationRule::argconverter, &rule)) {
            return NULL;
        }
        return new TAssociationRule(*rule);
    }

    PExample le, re;
    double supp = -1, conf = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&O&|dd", AssociationRule_keywords,
        &PExample::argconverter, &le, &PExample::argconverter, &re, &supp, &conf)) {
            return NULL;
    }
    TAssociationRule *rule = new TAssociationRule(le, re);
    rule->support = supp;
    rule->confidence = conf;
    return rule;
}


PyObject *TAssociationRule::__getnewargs__() const
{
    return Py_BuildValue("NN", left.getPyObject(), right.getPyObject());
}


PyObject *TAssociationRule::py_appliesLeft(PyObject *arg, PyObject *) const
{ 
    PExample ex = TExample::fromDomainAndPyObject(left->domain, arg, false);
    if (ex->domain != left->domain) {
        ex = ex->convertedTo(left->domain);
    }
    return PyBool_FromBool(appliesLeft(*ex));
}


PyObject *TAssociationRule::py_appliesRight(PyObject *arg, PyObject *) const
{ 
    PExample ex = TExample::fromDomainAndPyObject(right->domain, arg, false);
    if (ex->domain != right->domain) {
        ex = ex->convertedTo(right->domain);
    }
    return PyBool_FromBool(appliesRight(*ex));
}


PyObject *TAssociationRule::py_appliesBoth(PyObject *arg, PyObject *) const
{
    PExample ex = TExample::fromDomainAndPyObject(right->domain, arg, false);
    if (ex->domain != right->domain) {
        ex = ex->convertedTo(right->domain);
    }
    return PyBool_FromBool(appliesBoth(*ex));
}


string TAssociationRule::side2string(PExample const &ex)
{ 
    string res;
    if (ex->domain->variables->empty()) {
        ITERATE_METAS(*ex, mr)
            if (res.length()) {
                res += " ";
            }
            res += ex->domain->getMetaVar(mr.id)->getName();
        }
        return res;
    }
    else {
        TVarList::const_iterator vi(ex->domain->variables->begin());
        TExample::const_iterator ei(ex->begin());
        TExample::const_iterator const ee(ex->end());
        for(; ei!=ee; ei++, vi++) {
            if (!isnan(*ei)) {
                if (res.length()) {
                    res += " ";
                }
                res += (*vi)->getName() + "=" + (*vi)->val2str(*ei);
            }
        }
        return res;
    }
}

PyObject *TAssociationRule::__str__() const
{
    return PyUnicode_FromFormat("%s -> %s",
        side2string(left).c_str(), side2string(right).c_str());
}


PyObject *TAssociationRule::__repr__() const
{
    return __str__();
}


PyObject *TAssociationRule::__richcmp__(PyObject *pyother, int op) const
{
    if (!OrAssociationRule_Check(pyother)) {
        raiseError(PyExc_TypeError,
            "cannot compare association rule with instance of '%s'",
            pyother->ob_type->tp_name);
    }
    if ((op != Py_EQ) && (op != Py_NE)) {
        raiseError(PyExc_TypeError, "cannot compare two distributions");
    }
    PAssociationRule other(pyother);
    return PyBool_FromBool(
        ((*left == *other->left)  && (*right == *other->right)) == (op == Py_EQ));
}
