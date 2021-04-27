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
#include "discretevariable.px"

const char *sortedDaNe[] = {"da", "ne", 0 };
const char *resortedDaNe[] = {"ne", "da", 0};

const char **specialSortCases[] = { sortedDaNe, 0};
const char **specialCasesResorted[] = { resortedDaNe, 0};

const char *putAtBeginning[] = {"no", "none", "absent", "normal", 0};

/*! Constructs a discrete variable with the given name and an empty list
    of values. */
TDiscreteVariable::TDiscreteVariable(string const &aname)
: TVariable(aname, TVariable::Discrete, false),
  values(new TStringList()),
  baseValue(-1)
{}


/*! Constructs a discrete variable with the given name and values. The list
    of values is not copied; the class stores the pointer to the given list. */
TDiscreteVariable::TDiscreteVariable(string const &aname, PStringList const &val)
: TVariable(aname, TVariable::Discrete, false),
  values(val ? val : PStringList(new TStringList())),
  baseValue(-1)
{}


/*! Copy constructor. The list of values is cloned. */
TDiscreteVariable::TDiscreteVariable(const TDiscreteVariable &var)
: TVariable(var),
  values(var.values->clone()),
  baseValue(var.baseValue)
{}


/*! Returns true if the given variable is an instance of TDiscreteVariable,
    and has the same #varType, #ordered, #sourceVariable, #getValueFrom,
    #baseValue and #values. */
bool TDiscreteVariable::isEquivalentTo(const TVariable &old) const
{
    TDiscreteVariable const *eold = dynamic_cast<TDiscreteVariable const *>(&old);

    return eold
        && TVariable::isEquivalentTo(old)
        && ((baseValue == -1) || (eold->baseValue != -1) || (baseValue == eold->baseValue))
        && (values->size() == eold->values->size())
        && equal(values->begin(), values->end(), eold->values->begin());
}


/*! Sets the value to the first value of the variable or to undefined if the
    variable has an empty list of values. Returns \c true on success and
    \c false. */
bool TDiscreteVariable::firstValue(TValue &val) const
{
    if (values->size()) {
        val = TValue(0);
        return true;
    }
    else {
        val = UNDEFINED_VALUE;
        return false;
    }
}


/*! Sets the value to the next value and returns \c true; if the given value
    was the last one, it returns \c false. */
bool TDiscreteVariable::nextValue(TValue &val) const
{ 
    return ++val < values->size(); 
}


/*! Returns a random value for the variable. If a random value (an argument
    greater than 0) is given as an argument, the value returned value is the
    argument modulo the number of values. Otherwise, a random generator is
    constructed and used to pick a random value. */
TValue TDiscreteVariable::randomValue(int const rand) const
{
    if (!values->size()) {
        raiseError(PyExc_ValueError, "no values");
    }
    if (rand > 0) {
        return rand % values->size();
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    return TValue(randomGenerator->randint(values->size()));
}


/*! Adds a new value with the given name to #values. When the number of values
    exceeds 50, index #valuesTree is created. */
void TDiscreteVariable::addValue(string const &val)
{
    if (values->size() > 50) {
        if (valuesTree.empty()) {
            createValuesTree();
        }
        map<string, int>::iterator lb = valuesTree.lower_bound(val);
        if ((lb == valuesTree.end()) || (lb->first != val)) {
        // watch the order!
            valuesTree.insert(lb, make_pair(val, values->size()));
            values->push_back(val);
        }
    }
    else {
        if (!exists(values->begin(), values->end(), val)) {
            values->push_back(val);
        }
        if ((values->size() == 5) &&
            ((values->front() == "f") || (values->front() == "float"))) {
            TStringList::const_iterator vi(values->begin()), ve(values->end());
            char *eptr;
            char numtest[32];
            while(++vi != ve) { // skip the first (f/float)
                if ((*vi).length() > 31)
                    break;
                strcpy(numtest, (*vi).c_str());
                for(eptr = numtest; *eptr; eptr++) {
                    if (*eptr == ',') {
                        *eptr = '.';
                    }
                }
                double foo = strtod(numtest, &eptr);
                while (*eptr==32) {
                    eptr++;
                }
                if (*eptr)
                    break;
            }
            if (vi==ve) {
                PyErr_WarnFormat(PyExc_UserWarning, 1,
                    "is '%s' a continuous attribute unintentionally defined "
                    "by '%s'?", cname(), values->front().c_str());
            }
        }
    }
}


/*! Returns \c true if the variable has a value with the given name. */
bool TDiscreteVariable::hasValue(string const &s)
{
    if (!valuesTree.empty()) {
        return valuesTree.lower_bound(s) != valuesTree.end();
    }
    PITERATE(TStringList, vli, values) {
        if (*vli == s) {
            return true;
        }
    }
    return false;
}


/*! Returns a TValue corresponding to the given PyObject. The method calls
    the inherited method which does all the work (possibly calling
    #TDiscreteVariable::str2val). Upon return it checks whether the index is
    within range and raises an exception if needed. */
TValue TDiscreteVariable::py2val(PyObject *obj) const
{
    TValue val = TVariable::py2val(obj);
    if ((val < 0) || (val >= values->size())) {
        raiseError(PyExc_ValueError,
            "value %i is out of range for variable '%s'", val, cname());
    }
    return val;
}


/*! Converts a value to Python string. */
PyObject *TDiscreteVariable::val2py(TValue const val) const
{
    if (isnan(val)) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(val2str(val).c_str());
}


/*! Converts a value from string representation to TValue; the value is added
    if it did not exist yet. */
TValue TDiscreteVariable::str2val_add(string const &valname)
{
    if (strIsUndefined(valname)) {
        return UNDEFINED_VALUE;
    }
    const int noValues = values->size();
    if (noValues > 50) {
        if (valuesTree.empty()) {
            createValuesTree();
        }
        map<string, int>::iterator lb = valuesTree.lower_bound(valname);
        if ((lb != valuesTree.end()) && (lb->first == valname)) {
            return TValue(lb->second);
        }
        else {
            valuesTree.insert(lb, make_pair(valname, noValues));
            values->push_back(valname);
            return TValue(noValues);
        }
    }

    else {
        TStringList::iterator vi = find(values->begin(), values->end(), valname);
        if (vi != values->end()) {
            return TValue(vi - values->begin());
        }
        else {
            addValue(valname);
            return TValue(noValues);
        }
    }
}


/*! Converts a value from string representation to TValue; raises an exception
    if the value is not found. */
TValue TDiscreteVariable::str2val(string const &valname) const
{
    if (strIsUndefined(valname)) {
        return UNDEFINED_VALUE;
    }
    if (values->size() > 50) {
        if (valuesTree.empty()) {
            createValuesTree();
        }
        map<string, int>::const_iterator vi = valuesTree.find(valname);
        if (vi == valuesTree.end())
            raiseError(PyExc_ValueError,
            "attribute '%s' does not have value '%s'", cname(), valname.c_str());
        return TValue(vi->second);
    }
    else {
        TStringList::const_iterator vi = find(values->begin(), values->end(), valname);
        if (vi == values->end())
            raiseError(PyExc_ValueError,
            "attribute '%s' does not have value '%s'", cname(), valname.c_str());
        return TValue(vi - values->begin());
    }
}


/*! Converts a value from string representation to TValue; returns \c true on
    success, and \c false if the value is not found. */
bool TDiscreteVariable::str2val_try(string const &valname, TValue &valu) const
{
    try {
        valu = str2val(valname);
        return true;
    }
    catch (...) {
        return false;
    }
}



/*! Converts TValue to a string representation of value or \#RNGE if the value
    index is out of range. */
string TDiscreteVariable::val2str(TValue const val) const
{ 
    if (isnan(val)) {
        return undefined2str(val);
    }
    if (val > values->size()) {
        return "#RNGE";
    }
    return (*values)[int(val)];
}


/*! Constructs a #valuesTree for the current list #values. */
void TDiscreteVariable::createValuesTree() const
{
    int i = 0;
    const_PITERATE(TStringList, vi, values) {
        valuesTree[*vi] = i++;
    }
}


/*! Check whether the list of values matches the given list up to the
    length of the shortest list. */
bool TDiscreteVariable::checkValuesOrder(TStringList const &refValues)
{
    TStringList::const_iterator ni(refValues.begin()), ne(refValues.end());
    TStringList::const_iterator ei(values->begin()), ee(values->end());
    while((ei != ee) && (ni != ne)) {
        if (*ei++ != *ni++) {
            return false;
        }
    }
    return true;
}


/*! Sorts a set of values and handles several special cases. Values "no", "ne",
    "absent" or "normal", are put at the beginning of the list. Other values
    are sorted alphabetically. */
void TDiscreteVariable::presortValues(
    const set<string> &unsorted, vector<string> &sorted)
{
    sorted.clear();
    sorted.insert(sorted.begin(), unsorted.begin(), unsorted.end());
    vector<string>::iterator si, se(sorted.end());
    const char ***ssi, **ssii, ***rssi;
    for(ssi = specialSortCases, rssi = specialCasesResorted; *ssi; ssi++, rssi++) {
        for(si = sorted.begin(), ssii = *ssi; *ssii && (si != se) && !stricmp(*ssii, si->c_str()); *ssii++);
        if (!*ssii && (si==se)) {
            sorted.clear();
            sorted.insert(sorted.begin(), *rssi, *rssi + (ssii - *ssi));
            return;
        }
    }
    se = sorted.end();
    for(ssii = putAtBeginning; *ssii; ssii++) {
        for(si = sorted.begin(); (si != se) && stricmp(*ssii, si->c_str()); si++);
        if (si != se) {
            const string toMove = *si;
            sorted.erase(si);
            sorted.insert(sorted.begin(), toMove);
            break;
        }
    }
}

/// @cond Python
TOrange *TDiscreteVariable::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
Besides the name, constructor accepts a list of possibly symbolic values
for the variable.
*/
{
    if (args && (PyTuple_Size(args) == 5)) {
        return TVariable::__new__(type, args, kw);
    }
    PyObject *pyname = NULL;
    PStringList values;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O!O&:DiscreteVariable",
        DiscreteVariable_keywords, &PyUnicode_Type, &pyname,
        &PStringList::argconverter_n, &values)) {
            return NULL;
    }
    const string name = pyname ? PyUnicode_As_string(pyname) : "";
    return new(type) TDiscreteVariable(name, values);
}


PyObject *TDiscreteVariable::py_addValue(PyObject *arg)
{
    if (!PyUnicode_Check(arg)) {
        return PyErr_Format(PyExc_TypeError,
            "DiscreteVariable.add_value expects a string, not '%s'",
            arg->ob_type);
    }
    addValue(PyUnicode_As_string(arg));
    Py_RETURN_NONE;
}


#include "filter.hpp"

PyObject *TDiscreteVariable::__richcmp__(PyObject *other, int op)
{
    if ((op != Py_EQ) && (op != Py_NE)) {
        return PyErr_Format(PyExc_TypeError,
            "Discrete values can only be compared for (in)equality");
    }
    if (OrVariable_Check(other)) {
        return TVariable::__richcmp__(other, op);
    }

    bool const negate = op == Py_NE;
    if (!PyUnicode_Check(other)) { // prevent iterating over a string
        vector<double> values;
        try {
            PVariable const me = PVariable::fromBorrowedPtr(this);
            if (TValueList::convertVarSeq(me, other, values)) {
                PValueList valueList(new TValueList(values));
                PFilter_values filter(new TFilter_values());
                filter->addCondition(me, valueList, negate);
                return filter.getPyObject();
            }
        }
        catch (...) {
        }
    }
    TValue ref = py2val(other);
    PFilter_values filter(new TFilter_values());
    filter->addCondition(PVariable::fromBorrowedPtr(this), ref, negate);
    return filter.getPyObject();
}
/// @endcond