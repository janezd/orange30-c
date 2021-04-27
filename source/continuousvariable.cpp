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
#include "continuousvariable.px"

/*! Constructs a continuous variable with the given name. */
TContinuousVariable::TContinuousVariable(string const &aname)
: TVariable(aname, TVariable::Continuous, true),
  numberOfDecimals(3),
  scientificFormat(false),
  adjustDecimals(2)
{}

/*! Returns \c true if the given value is a continuous value
    and has the same #varType, #ordered, #sourceVariable and #getValueFrom. */
bool TContinuousVariable::isEquivalentTo(const TVariable &old) const
{
    return dynamic_cast<TContinuousVariable const *>(&old) && TVariable::isEquivalentTo(old);
}


inline int getNumberOfDecimals(const char *vals, bool &hasE)
{
    const char *valsi;
    for(valsi = vals; *valsi && ((*valsi<'0') || (*valsi>'9')) && (*valsi != '.'); valsi++);
    if (!*valsi) {
        return -1;
    }
    if ((*valsi=='e') || (*valsi=='E')) {
        hasE = true;
        return 0;
    }
    for(; *valsi && (*valsi!='.'); valsi++);
    if (!*valsi) {
        return 0;
    }
    int decimals = 0;
    for(valsi++; *valsi && (*valsi>='0') && (*valsi<='9'); valsi++, decimals++);
    hasE = hasE || (*valsi == 'e') || (*valsi == 'E');
    return decimals;
}


/*! Converts the value to Python float or \c None if undefined. */
PyObject *TContinuousVariable::val2py(TValue const val) const
{
    if (isnan(val))
        Py_RETURN_NONE;
    return PyFloat_FromDouble(val);
}


/*! Converts a string to a value. The function converts "." to "," or
    vice-versa if needed. The field #numberOfDecimals is adjusted if
    #adjustDecimals is non-zero. Raises an exception on error. */
TValue TContinuousVariable::str2val(string const &valname) const
{
    if (strIsUndefined(valname)) {
        return UNDEFINED_VALUE;
    }
    const char *vals;
    char *tmp = NULL;

    const char radix = *localeconv()->decimal_point;
    const char notGood = radix=='.' ? ',' : '.';
    int cp = valname.find(notGood);
    if (cp!=string::npos) {
        vals = tmp = strcpy(new char[valname.size()+1], valname.c_str());
        tmp[cp] = radix;
    }
    else {
        vals = valname.c_str();
    }

    double f;
    int ssr = sscanf(vals, "%lf", &f);
    if (!ssr || (ssr==(char)EOF)) {
        delete tmp;
        raiseError(PyExc_ValueError,
            "'%s' is not a legal value for continuous attribute '%s'",
            valname.c_str(), cname());
    }

    int decimals;
    switch (adjustDecimals) {
        case 2:
            numberOfDecimals = getNumberOfDecimals(vals, scientificFormat);
            adjustDecimals = 1;
            break;
        case 1:
            decimals = getNumberOfDecimals(vals, scientificFormat);
            if (decimals > numberOfDecimals) {
                numberOfDecimals = decimals;
            }
    }

    delete tmp;
    return TValue(f);
}

/*! Converts a string to a value by calling #str2val and returns \c false on
    failure. */
bool TContinuousVariable::str2val_try(string const &valname, TValue &valu)
{
    try {
        valu = str2val(valname);
        return true;
    }
    catch (...) {
        return false;
    }
}


/*! Converts a value to a string with the number of decimals and format as
    defined by #numberOfDecimals and #scientificFormat. */
string TContinuousVariable::val2str(TValue const valu) const
{ 
    if (isnan(valu))
        return "?";
    char buf[64];
    if (scientificFormat)
        sprintf(buf, "%g", valu);
    else
        sprintf(buf, "%.*f", numberOfDecimals, valu);
    return buf;
}

/// @cond Python
TOrange *TContinuousVariable::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (args && (PyTuple_Size(args) == 5)) {
        return TVariable::__new__(type, args, kw);
    }
    PyObject *name = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O!:ContinuousVariable",
        ContinuousVariable_keywords, &PyUnicode_Type, &name)) {
        return NULL;
    }
    return new(type) TContinuousVariable(name ? PyUnicode_As_string(name) : "");
}


#include "filter.hpp"

PyObject *TContinuousVariable::__richcmp__(PyObject *other, int op)
{
    if (OrVariable_Check(other)) {
        return TVariable::__richcmp__(other, op);
    }
    TValue min, max = UNDEFINED_VALUE;
    int myop;
    if (PyTuple_Check(other)) {
        if (!PyArg_ParseTuple(other, "dd:ContinuousVariable.__richcmp__", &min, &max)) {
            return NULL;
        }
        if (op == Py_EQ) {
            myop = TValueFilter::Between;
        }
        else if (op == Py_NE) {
            myop = TValueFilter::Outside;
        }
        else {
            myop = TValueFilter::operatorFromPy(op);
        }
    }
    else {
        min = py2val(other);
        myop = TValueFilter::operatorFromPy(op);
        if (myop == TValueFilter::None) {
            return NULL;
        }
    }
    if (((op == Py_GT) || (op == Py_GE)) && (!isnan(max))) {
        min = max;
    }
    PFilter_values filter(new TFilter_values());
    filter->addCondition(PVariable::fromBorrowedPtr(this), myop, min, max);
    return filter.getPyObject();
}

/// @endcond