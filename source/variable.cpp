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


// include Python.h before STL defines a template set (doesn't work with VC 6.0)
#include "common.hpp"
#include <algorithm>
#include <queue>
#include <float.h>
#include <locale>

#ifdef DARWIN
#include <strings.h>
#endif

#include "progarguments.hpp"

#include "variable.px"


/*! \enum TVariable::Type Describes the type of the variable.
*/
/*! \var TVariable::Type TVariable::Discrete
      Discrete variable whose
      values are stored as #TValue that is interpreted as an integer
      index and can be used as an ordinary or a meta attribute */
/*! \var TVariable::Type TVariable::Continuous
      Continuous variable
      whose values are stored as #TValue and can be used as an
      ordinary or a meta attribute */
/*! \var TVariable::Type TVariable::String
      A string variable, which can only be used as a meta attribute */


/*! \enum TVariable::MakeStatus Status codes for #make and
   #retrieve.  The code refers to the difference between the
   requested variable (its name, types and values) and the existing
   ones stored in #TVariable::allVariables.
*/
/*! \var TVariable::MakeStatus TVariable::OK
   The new variable contains at least one of the existing values
   and no new values; there is no problem with their order. */
/*! \var TVariable::MakeStatus TVariable::MissingValues
   The new variable contains at least one of the existing values
   and some new values; there is no problem with their order. */
/*! \var TVariable::MakeStatus TVariable::NoRecognizedValues
   The new variable contains none of the existing values. */
/*! \var TVariable::MakeStatus TVariable::Incompatible
   The new variable prescribes an order of values that is
   incompatible with the existing order. */
/*! \var TVariable::MakeStatus TVariable::NotFound 
   A variable with that name and type does not exist yet. */


bool mmvDeallocated = 0;

/* The following class is to be used only for the allVariablesMap variable.
 * It sets a flag after it has been deallocated, so that after program shutdown
 * the deallocated TVariable instances do not try to remove themselves from the
 * map. Besides, this class overrides the non-virtual destructor of STL
 * multimap, so it's not functional if the variable is declared using MMV's
 * supertype!
 */
class MMV :	public multimap<string, TVariable *> {
public:
	~MMV() {
		mmvDeallocated = 1;
	}
};

/*! A map of all variables in existence that is used by #make and
   #retrieve. */
MMV TVariable::allVariables;


/*! Search for an existing variable in #allVariables with the given
    name and type. For discrete variables, the values given as
    arguments are added.  In case multiple variables match, the method
    returns the one with the lowest status. \c NULL is returned if the
    lowest status is equal to or greater than \c failOn, \c Incompatible
    or \c NotFound. 

    \param name Name of the variable
    \param varType The type of the variable (see #Type)
    \param fixedOrderValues Values that the new variable must have in that
            exact order; the existing may not have all or it can have more
    \param values Other values that the new attribute must have; the order
            is arbitrary
    \param failOn The status at which (or above) the existing variable
            no longer matches the requirements
    \param status Returned status. If \c NULL is passed, the
            status is not set.
    \return Existing variable that matches the requirements or \c NULL.
*/
PVariable TVariable::retrieve(string const &name,
                              int const varType,
                              vector<string> const *fixedOrderValues,
                              set<string> const *const values,
                              int const failOn,
                              int * status)
{
    if ((fixedOrderValues && fixedOrderValues->size() ) && (varType != Discrete)) {
        raiseError(PyExc_TypeError,
            "cannot specify the value list for non-discrete attributes");
    }
    if (failOn == TVariable::OK) {
        if (status) {
            *status = TVariable::OK;
        }
        return PVariable();
    }
    vector<pair<TVariable *, int> > candidates;
    vector<string>::const_iterator vvi, vve;
    TStringList::const_iterator svi, sve;

   pair<MMV::iterator,MMV::iterator> rp = allVariables.equal_range(name);
   MMV::iterator it;
   for (it=rp.first; it!=rp.second; ++it) {	
       if ((it->second)->varType == varType) {
            int tempStat = TVariable::OK;

            // non-discrete attributes are always ok,
            // discrete ones need further checking if they have any defined values
            if (varType == TVariable::Discrete) {
                TDiscreteVariable *evar = 
                    dynamic_cast<TDiscreteVariable *>(it->second);
                if (!evar) {   // shouldn't happen, but let's be safe
                    continue; 
                }
                if (evar && evar->values->size()) {
                    if (fixedOrderValues &&
                        !evar->checkValuesOrder(*fixedOrderValues)) {
                        tempStat = TVariable::Incompatible;
                    }
                    if ((tempStat == TVariable::OK)
                        && (   values && values->size()
                            || fixedOrderValues && fixedOrderValues->size())) {
                        for(svi = evar->values->begin(),
                            sve = evar->values->end();
                            (svi != sve)
                            && (!values ||
                                (values->find(*svi) == values->end()))
                            && (!fixedOrderValues || 
                                 (find(fixedOrderValues->begin(),
                                       fixedOrderValues->end(),
                                       *svi) == fixedOrderValues->end()));
                            svi++
                        );
                        if (svi == sve) {
                            tempStat = TVariable::NoRecognizedValues;
                        }
                    }
                    if ((tempStat == TVariable::OK) && fixedOrderValues) {
                        for(vvi = fixedOrderValues->begin(),
                            vve = fixedOrderValues->end();
                            (vvi != vve) && evar->hasValue(*vvi);
                            vvi++);
                        if (vvi != vve) {
                            tempStat = TVariable::MissingValues;
                        }
                    }
                    if ((tempStat == TVariable::OK) && values) {
                        set<string>::const_iterator vsi(values->begin()),
                            vse(values->end());
                        for(; (vsi != vse) && evar->hasValue(*vsi); vsi++);
                        if (vsi != vse) {
                            tempStat = TVariable::MissingValues;
                        }
                    }
                }
            }
            candidates.push_back(make_pair(it->second, tempStat));
            if (tempStat == TVariable::OK)
                break;
        }
    }
    PVariable var;
    int intStatus;
    if (!status) {
        status = &intStatus;
    }
    *status = TVariable::NotFound;
    const int actFailOn = failOn > TVariable::Incompatible ?
        TVariable::Incompatible : failOn;
    for(vector<pair<TVariable *, int> >::const_iterator ci(candidates.begin()),
            ce(candidates.end()); ci != ce; ci++) {
        if (ci->second < *status) {
            *status = ci->second;
            if (*status < actFailOn) {
                var = PVariable::fromBorrowedPtr(ci->first);
            }
        }
    }
    TDiscreteVariable *evar =
        dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
    if (evar) {
        if (fixedOrderValues) {
            const_PITERATE(vector<string>, si, fixedOrderValues)
                evar->addValue(*si);
        }

        if (values) {
            vector<string> sorted;
            TDiscreteVariable::presortValues(*values, sorted);
            const_ITERATE(vector<string>, ssi, sorted)
                evar->addValue(*ssi);
        }
    }
    return var;
}


/*! Call #retrieve to find an existing variable that matches the
    requirements. If there is none, the method creates a new variable. 

    \param name Name of the variable
    \param varType The type of the variable (see #Type)
    \param fixedOrderValues Values that the new variable must have in that
            exact order; the existing may not have all or it can have more
    \param values Other values that the new attribute must have; the order
            is arbitrary
    \param createNewOn The status at which (or above) the existing variable
            no longer matches the requirements
    \param status The status returned by #retrieve. If equal to or above
            \c createNewOn, the variable returned as result is a new
            variable. If \c NULL is passed as \c status, the status is not
            set.
    \return Existing or new variable

    \see make(TAttributeDescription const &, int &, int const)
*/

PVariable TVariable::make(string const &name,
                          int const varType,
                          vector<string> const *fixedOrderValues,
                          set<string> const *const values,
                          int const createNewOn,
                          int *status)
{
    int intStatus;
    if (!status) {
        status = &intStatus;
    }
    PVariable var;
    if (createNewOn == TVariable::OK) {
        *status = TVariable::OK;
    }
    else {
        var = retrieve(name, varType, fixedOrderValues, values, createNewOn,
                          status);
    }
    if (!var) {
        switch (varType) {
            case TVariable::Discrete: {
                var = PVariable(new TDiscreteVariable(name));
                TDiscreteVariable *evar =
                    dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
                if (evar) {
                    if (fixedOrderValues) {
                        const_PITERATE(vector<string>, si, fixedOrderValues)
                            evar->addValue(*si);
                    }
                    if (values) {
                        vector<string> sorted;
                        TDiscreteVariable::presortValues(*values, sorted);
                        const_ITERATE(vector<string>, ssi, sorted)
                            evar->addValue(*ssi);
                    }
                }
                break;
            }
            case TVariable::Continuous: {
                var = PVariable(new TContinuousVariable(name));
                break;
            }
            case TVariable::String: {
                var = PVariable(new TStringVariable(name));
                break;
            }
        }
    }
    return var;
}



/*! Similar to #make(string const &, int const, vector<string> const *,
    set<string> const *const, int const, int *). The variable is described
    with #TVariable::TAttributeDescription and includes the flag whether the variable
    is ordered and additional user-defined flags.

    \see make(string const &, int const, vector<string> const *,
    set<string> const *const, int const, int *)
*/
PVariable TVariable::make(TAttributeDescription const &desc,
                          int &status, int const createNewOn)
{
    set<string> values;
    for(map<string, int>::const_iterator dvi(desc.values.begin()), dve(desc.values.end()); dvi != dve; dvi++) {
        values.insert(values.end(), dvi->first);
    }
    PVariable var = TVariable::make(desc.name, desc.varType, &desc.fixedOrderValues, &values, createNewOn, &status);
    if (!var) {
        raiseError(PyExc_ValueError, "unknown type for attribute '%s'", desc.name.c_str());
    }
    if (desc.ordered) {
        var->ordered = true;
    }
    if (desc.userFlags.size()) {
        PyObject *pyvar = var.borrowPyObject();
        const_ITERATE(TMultiStringParameters, si, desc.userFlags) {
            PyObject *value = PyUnicode_FromString((*si).second.c_str());
            PyObject_SetAttrString(pyvar, (*si).first.c_str(), value);
            Py_DECREF(value);
        }
    }
    return var;
}


/*! Tell whether two variables are equivalent: have the same #varType,
  #ordered, #sourceVariable and #getValueFrom. */
bool TVariable::isEquivalentTo(const TVariable &old) const {
    return    (varType == old.varType)
           && (ordered == old.ordered)
           && (!sourceVariable || !old.sourceVariable || (sourceVariable == old.sourceVariable))
           && (!getValueFrom || !old.getValueFrom || (getValueFrom == old.getValueFrom));
}


/*! Adds a variable to the list of existing variables */
void TVariable::registerVariable()
{
    allVariables.insert(pair<string, TVariable *>(name, this));
}

/*! Removes a variable form the list of existing variables */
void TVariable::unregisterVariable()
{
    pair<MMV::iterator,MMV::iterator> rp = allVariables.equal_range(name);
    for (MMV::iterator it=rp.first; it!=rp.second; ++it) {
        if (it->second == this) {
            allVariables.erase(it);
            break;
        }
    }
}


/* Construct a variable and add it to #allVariables. 
   \param avarType Variable type
   \param ord True if the variable is ordered */
TVariable::TVariable(int const avarType, bool const ord)
: varType(avarType),
  ordered(ord),
  getValueFromLocked(false),
  defaultMetaId(0)
{
    registerVariable();
}


/* Construct a variable and add it to #allVariables.
   \param aname Variable name
   \param avarType Variable type
   \param ord True if the variable is ordered */
TVariable::TVariable(string const &aname, int const avarType, bool const ord)
: name(aname),
  varType(avarType),
  ordered(ord),
  getValueFromLocked(false),
  defaultMetaId(0)
{ 
    registerVariable();
};


/*! Destruct the variable and remove it from #allVariables */
TVariable::~TVariable()
{
    /* When the program shuts down, it may happen that the list is destroyed before
       the variables. We do nothing in this case. */
    if (!mmvDeallocated) {
        unregisterVariable();
    }
}


/*! Return undefined variable as TValue. Returns #undefined_value. */
const TValue TVariable::undefinedValue() const
{ return UNDEFINED_VALUE; }


/*! Return \c true if the given string represents the undefined value
  for the given variable.
  \param valname String representing a value */
bool TVariable::strIsUndefined (string const &valname) const
{
    return (valname=="?") || (valname=="~") || !valname.length();
}


/*! If the given string \c valname represents an unknown value, set
     \c val to \c undefined_value and return \c true; return \c false
     otherwise.
     \param valname A string describing the value
     \param val The value to be set
     \return \c true if the value is set
 */
bool TVariable::str2undefined (string const &valname, TValue &val) const
{ 
    if (strIsUndefined(valname)) {
        val = UNDEFINED_VALUE;
        return true;
    }
    return false;
}


/*! Return the symbolic representation for unknown (<tt>"?"</tt>)
    \param val The value */
string TVariable::undefined2str(TValue const val) const
{ 
    return "?";
}


/*! Return a \c TValue (\c double) represented by the given Python
  object *obj. The abstract type #TVariable already handles missing
  values, values represented by numbers and the strings; the latter
  are passed to #str2val. Numeric values are not checked for range.*/
TValue TVariable::py2val(PyObject *obj) const
{
    if (obj == Py_None)
        return undefined_value;

    if (PyLong_Check(obj))
        return (double)PyLong_AsLong(obj);

    if (PyFloat_Check(obj))
        return PyFloat_AsDouble(obj);

    if (PyUnicode_Check(obj))
        return str2val(PyUnicode_As_string(obj));

    if (OrPyValue_Check(obj)) {
        TPyValue &pyval = (TPyValue &)((OrOrange *)obj)->orange;
        if (pyval.variable && (pyval.variable != this)) {
            raiseError(PyExc_ValueError,
                "expected value of '%s', got value of '%s'",
                name.c_str(), pyval.variable->name.c_str());
        }
        return pyval.value;
    }
    raiseError(PyExc_TypeError,
        "cannot interpret an instance of '%s' as value of variable '%s'",
        obj->ob_type->tp_name, name.c_str());
    return 0;
}


/*! Convert a Python object into a meta value. The default method,
    provided by #TVariable returns this same object (with increased
    reference count. 
    \param obj The object to convert */
PyObject *TVariable::py2pyval(PyObject *obj) const
{
    Py_INCREF(obj);
    return obj;
}


/*! Give a string representation of a primitive value. Works only for
    primitive variables; this default method raises an exception.
    \param val The value to be represented. */
string TVariable::val2str(TValue const val) const
{
    raiseError(PyExc_SystemError,
        "Invalid call: Variables of this type do not have primitive values");
    return string();
}


/*! Return a string representation a non-primitive value. This
    default implementation calls the object's \c __repr__ method.
    \param val The value to be represented. */
string TVariable::pyval2str(PyObject *val) const
{
    if (val == Py_None) {
        return string();
    }
    PyObject  *repred = PyObject_Repr(val);
    GUARD(repred);
    return PyUnicode_As_string(repred);
}


/*! Return a <tt>PyObject *</tt> representing the value. Derived primitive
    variables must overload this method; in #TVariable it only
    raises an exception. */
PyObject *TVariable::val2py(TValue const val) const
{
    raiseError(PyExc_SystemError,
        "Primitive variables should overload val2py method");
    return NULL;
}


/*! Return a <tt>PyObject *</tt> representing the give non-primitive
    value. This default representation returns the same object (with
    increased reference count. */
PyObject *TVariable::pyval2py(PyObject *val) const
{
    Py_INCREF(val);
    return val;
}


/*! Return the primitive value represented by a string. The derived
    primitive classes must overload this method; in #TVariable it only
    raises an exception. For discrete variables the function is expected
    to fail (that is, raise an exception) if the value does not exist.
    \param valname A string representing the value 
    \see str2val_try(string const &, TValue &) 
    \see str2val_add(string const &) */
TValue TVariable::str2val(string const &valname) const
{
    raiseError(PyExc_SystemError,
        "Invalid call: variables of this type do not have primitive values");
    return 0;
}


/*! Try to compute the value represented by a string. Returns \c true
    if successful.
    \param valname A string representing the value
    \param val A reference into which the result is stored if successful
    \see str2val(string const &) const
    \see str2val_add(string const &) */
bool TVariable::str2val_try(string const &valname, TValue &val)
{ 
    try {
        val = str2val(valname);
        return true;
    } catch (exception) {
        return false;
    }
}


/*! Return the value represented by the given string. For discrete
    variables, the value is added if it does not exist yet. 
    \see str2val_try(string const &, TValue &) 
    \see str2val(string const &) const */
TValue TVariable::str2val_add(string const &valname)
{ 
    return str2val(valname);
}


/*! Return a meta value (\c PyObject) represented by the given
    string. For #TVariable, this only returns \c Py_None. The method
    must be overload be derived non-primitive classes. */
PyObject *TVariable::str2pyval(string const &s) const
{
	Py_RETURN_NONE;
}


/*! Calls #str2val_add. Derived classes may change this to provide a
    special representation for reading the value from files. 
    \param valname A string representing the value */
TValue TVariable::filestr2val(string const &valname)
{ 
    return str2val_add(valname);
}


/*! Calls #val2str. Derived classes may change this to provide a
    special representation for writing the value to files. 
    \param val The value to be converted */
string TVariable::val2filestr(TValue const val) const
{ 
    return val2str(val);
}


/*! Calls #pyval2str. Derived classes may change this to provide a
    special representation for writing the value to files. 
    \param val The value to be converted */
string TVariable::pyval2filestr(PyObject *val) const
{
    return pyval2str(val);
}


/*! Calls #str2pyval. Derived classes may change this to provide a
    special representation for reading the value from files. 
    \param valname A string representing the value */
PyObject *TVariable::filestr2pyval(string const &valname)
{
    return str2pyval(valname);
}


/*! Set the given reference to the first value of the variable and
    return \c true. If there is no first value (e.g., a discrete
    variable has no values), return \c false. If the functionality
    is not provided, exception is raised. */
bool TVariable::firstValue(TValue &) const
{
    raiseError(PyExc_TypeError,
        "Variable '%s' cannot provide the first value",
        name.c_str());
    return false;
}


/*! Set the given reference to the next value of the variable and
    return \c true. If there is no next value (e.g., the given reference
    already has the last value of a discrete variable), return \c false.
    If the functionality is not provided, exception is raised. */
bool TVariable::nextValue(TValue &) const
{
    return false;
}


/*! Return a random value for the variable or raise an exception if
  the functionality is not provided. */
TValue TVariable::randomValue(int const) const
{
    raiseError(PyExc_TypeError,
        "Variable '%s' cannot provide random values", name.c_str());
    return 0;
}


/*! Compute the value of the variable from values of other variables,
    given as an example, by calling #getValueFrom. Before doing so,
    the method checks whether #getValueFromLocked is \c true. In this
    case, it returns a missing value. Otherwise, it set is to \c true,
    calls #getValueFrom and sets it back to \c false.
   
    If #getValueFrom is not defined, the function returns a missing value.

    \param ex Values of other variables */
TValue TVariable::computeValue(TExample const *const ex)
{
    if (getValueFrom && !getValueFromLocked) {
        try {
            getValueFromLocked = true;
            const TValue val = (*getValueFrom)(ex);
            getValueFromLocked = false;
            return val;
        }
        catch (...) {
          getValueFromLocked = false;
          throw;
        }
    }
    else {
        return UNDEFINED_VALUE;
    }
}


/*! Return the number of values for enumerable types, and -1 otherwise. */
int TVariable::noOfValues() const
{
    return -1;
}


/// @cond Python

TOrange *TVariable::__new__(PyTypeObject *, PyObject *args, PyObject *kw)
{
    PyObject *var_stat = py_make(NULL, args, kw);
    if (!var_stat) {
        return NULL;
    }
    GUARD(var_stat);
    PyObject *pyvar = PyTuple_GET_ITEM(var_stat, 0);
    Py_INCREF(pyvar);
    return &((OrOrange *)pyvar)->orange;
}

PyObject *TVariable::__repr__() const
{
    string v = string("<")
        + THIS_AS_PyObject->ob_type->tp_name
        + " '" + name + "'>";
    return PyUnicode_FromString(v.c_str());
}


PyObject *TVariable::__getnewargs__() const
{
    TDiscreteVariable const *discvar =
        dynamic_cast<TDiscreteVariable const *>(this);
    return Py_BuildValue("(siOOi)", 
              name.c_str(),
              varType,
              discvar ? discvar->values.borrowPyObject() : Py_None,
              Py_None,
              Incompatible);
}


PyObject *TVariable::__richcmp__(PyObject *other, int op) const
{
    if (OrVariable_Check(other) && ((op == Py_EQ) || (op == Py_NE))) {
        if (this == &((OrOrange *)other)->orange == (op == Py_EQ)) {
            Py_RETURN_TRUE;
        }
        else {
            Py_RETURN_FALSE;
        }
    }
    return PyErr_Format(PyExc_TypeError, "invalid comparison operator");
}


int TVariable::vartypeconverter(PyObject *obj, int *varType)
{
    if (PyLong_Check(obj)) {
        *varType = PyLong_AsLong(obj);
    }
    else if (PyType_Check(obj)) {
        if (obj == (PyObject *)&OrDiscreteVariable_Type) {
            *varType = TVariable::Discrete;
        }
        else if (obj == (PyObject *)&OrContinuousVariable_Type) {
            *varType = TVariable::Continuous;
        }
        else if (obj == (PyObject *)&OrStringVariable_Type) {
            *varType = TVariable::String;
        }
        else {
            PyErr_Format(PyExc_TypeError,
                "'%s' is not a type of variable", obj->ob_type);
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "cannot convert '%s' to variable type", obj->ob_type);
        return 0;
    }
    return 1;
}


PyObject *TVariable::py_getOrMake(PyObject *args, PyObject *kw,
                                  char **keywords,
                                  TVariable::getMakeMeth meth)
{
    char *varName;
    int varType;
    PStringList values_asList;
    PStringList unorderedValues_asList;
    int condition = TVariable::Incompatible;
    if (!PyArg_ParseTupleAndKeywords(args, kw, 
        "sO&|O&O&i:make", keywords,
        &varName,
        &TVariable::vartypeconverter, &varType,
        &PStringList::argconverter_n, &values_asList,
        &PStringList::argconverter_n, &unorderedValues_asList,
        &condition)) {
            return NULL;
    }
    vector<string> values;
    if (values_asList) {
        values.reserve(values_asList->size());
        PITERATE(TStringList, vi, values_asList) {
            values.push_back(*vi);
        }
    }
    set<string> unorderedValues;
    if (unorderedValues_asList) {
        unorderedValues.insert(
            unorderedValues_asList->begin(), unorderedValues_asList->end());
    }
    int status;
    PVariable var = meth(varName, varType,
        &values, &unorderedValues, condition, &status);
    return Py_BuildValue("NO", var.toPython(),
        get_constantObject(status, Variable_MakeStatus_constList));
}


PyObject *TVariable::py_retrieve(PyObject *, PyObject *args, PyObject *kw)
{
    return py_getOrMake(args, kw, Variable_retrieve_keywords, &TVariable::retrieve);
}


PyObject *TVariable::py_make(PyObject *, PyObject *args, PyObject *kw)
{
    return py_getOrMake(args, kw, Variable_make_keywords, &TVariable::make);
}


PyObject *TVariable::__get__name(OrVariable *self)
{
    return PyUnicode_FromString(((TVariable &)self->orange).cname());
}


int TVariable::__set__name(OrVariable *self, PyObject *value)
{
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
            "variable name should be a string, not '%s'",
            value->ob_type->tp_name);
        return -1;
    }
    ((TVariable &)self->orange).setName(PyUnicode_As_string(value));
    return 0;
}


PyObject *TVariable::py_randomvalue() const
{
    return PyObject_FromNewOrange(new TPyValue(
        PVariable::fromBorrowedPtr(const_cast<TVariable *>(this)),
        randomValue()));
}


PyObject *TVariable::py_computeValue(PyObject *args, PyObject *kw)
{ 
        return PyErr_Format(PyExc_NotImplementedError, "Not implemented yet.");
/*        TExample *ex;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!:Variable.computeValue", Variable_computeValue_keywords,
                                         PyExample_Type, &ex))
            Py_RETURN_NONE;

        int idx = ex->domain->getVarNum(self, false);
        if (idx != ILLEGAL_INT) {
            return Value_FromVariableValue(self, ex[idx]);
        }
        if (!self->getValueFrom) {
            return PyErr_Format(PyExc_SystemError, "'getValueFrom' is not defined");
        }
        return Value_FromVariableValue(var, self->computeValue(*ex));*/
}


PyObject *TVariable::undefined() const
{
    return PyObject_FromNewOrange(new TPyValue(
        PVariable::fromBorrowedPtr(const_cast<TVariable *>(this)),
        undefinedValue()));
}

PyObject *TVariable::DC() const
{
    return undefined();
}

PyObject *TVariable::DK() const
{
    return undefined();
}

PyObject *TVariable::specialValue() const
{
    return undefined();
}


/// @endcond

TVariable::TAttributeDescription::TAttributeDescription(
    const string &aName, const int &aType, bool anOrdered)
: name(aName),
  varType(aType),
  ordered(anOrdered)
{}


TVariable::TAttributeDescription::TAttributeDescription(
    TVariable::TAttributeDescription const &old)
: name(old.name),
  varType(old.varType),
  typeDeclaration(old.typeDeclaration),
  ordered(old.ordered),
  fixedOrderValues(old.fixedOrderValues),
  values(old.values),
  userFlags(old.userFlags)
{}


TVariable::TAttributeDescription &TVariable::TAttributeDescription::operator =(
    TVariable::TAttributeDescription const &old)
{
    if (this != &old) {
        name = old.name;
        varType = old.varType;
        typeDeclaration = old.typeDeclaration;
        ordered = old.ordered;
        fixedOrderValues = old.fixedOrderValues;
        values = old.values;
        userFlags = old.userFlags;
    }
    return *this;
}

/*! Add a fixed order value */
void TVariable::TAttributeDescription::addValue(const string &s)
{
  fixedOrderValues.push_back(s);
  values[s] = 1;
}
