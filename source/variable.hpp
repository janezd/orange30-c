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


#include "variable.ppp"

#ifndef __VARIABLE_HPP
#define __VARIABLE_HPP


class TExample;
class TTransformValue;
class TDistribution;
class TClassifier;

#include "classifier.hpp"
#include "stladdon.hpp"


class MMV;

class TVariable : public TOrange {
public:
    __REGISTER_ABSTRACT_CLASS(Variable)
private:
    static MMV allVariables;
    void registerVariable();
    void unregisterVariable();

   /*! A lock indicating that #getValueFrom is called; this prevents
      circular lookups.
 
      /see getValueFrom, sourceVariable  */
    bool getValueFromLocked;

    /*! The name of the variable */
    string name;

public:
    enum Type {Discrete, Continuous, String }; PYCLASSCONSTANTS;
 
    /*! The type; can be \c Discrete, \c Continuous or \c String */
    int  varType; //PR(&Type) contains type

    /*! Tells whether the values of a discrete attribute are ordered */
    bool ordered; //P if true, attribute's values are ordered

    /*! Default meta id for this attribute. Used in TDomain::prepareDomain to give
        the meta attribute the same id as in other domains. This id should
        be treated as a hint: the attribute is allowed to have other id's
        in other domains. */
    int defaultMetaId; //P default (proposed, suggested...) meta id for this attribute
    
    /*! Random generator used by #randomValue */
    mutable PRandomGenerator randomGenerator; //P random generator

    /*! The attribute from which this attribute is computed; used when 
        applicable. This field is not used by TVariable itself for
        computing the value of the variable (see #getValueFrom), but can
        be used as an indicator. */
    PVariable sourceVariable; //P The attribute that this attribute is computed from (when applicable)

    /*! A classifier used for computing the value of the variable from
        other variables. */
    PClassifier getValueFrom; //P Function to compute the value from values of other variables

protected:
    TVariable(int const avarType, bool const ordered = false);
    TVariable(string const &, int const avarType, bool const ordered = false);

public:
    ~TVariable();

    inline string const &getName() const;
    inline char const *cname() const;
    inline void setName(string const &a);

    enum MakeStatus { OK, MissingValues, NoRecognizedValues, Incompatible, NotFound } PYCLASSCONSTANTS;
  
    /*! The signature for a function that returns an existing variable or
        constructs a new one from a description */
    typedef PVariable getMakeMeth(string const &name,
                                  int const varType,
                                  vector<string> const *fixedOrderValues,
                                  set<string> const *const values,
                                  int const condition,
                                  int *status);

    static PVariable retrieve(string const &name,
                              int const varType,
                              vector<string> const *fixedOrderValues=NULL,
                              set<string> const *const values=NULL,
                              int const failOn=Incompatible,
                              int *status=NULL);
                                
  /* Gets an existing variable or makes a new one. A new one is made if there is no
     existing variable by that name or its status (above) equals or exceeds createNewOne.
     The returned status equals to the result of the search for an existing variable,
     except if createNewOn==OK, in which case status is always OK.  */
    static PVariable make(string const &name,
                           int const varType,
                           vector<string> const *fixedOrderValues=NULL,
                           set<string> const *const value=NULL,
                           const int createNewOn=Incompatible,
                           int *status=NULL);
      
    /*! A structure with information for creating a variable */
    class TAttributeDescription {
    public:
        string name; /*!< Variable name */
        int varType; /*!< Variable type (see #TVariable::Type) */
        bool ordered; /*!< \c true for discrete variables with ordered values */
        string typeDeclaration; /*!< A type declaration read from tab-delimited file */
        vector<string> fixedOrderValues; /*!< Values with prescribed order */
        map<string, int> values; /*!< Values and their frequencies */
        multimap<string, string> userFlags; /*!< Additional flags given by the user */

        TAttributeDescription(string const &, int const &, bool const = false);
        TAttributeDescription(TAttributeDescription const &);
        TAttributeDescription &operator =(TVariable::TAttributeDescription const &);
        void addValue(string const &s);
    };

    static PVariable make(TAttributeDescription const &desc, int &status,
        int const createNewOn=TVariable::Incompatible);

    virtual bool isEquivalentTo(const TVariable &) const;
    inline bool isPrimitive() const;
  
    virtual const TValue undefinedValue() const;

    bool strIsUndefined(string const &) const;
    bool str2undefined(string const &, TValue &valu) const;
    string undefined2str(TValue const) const;

    TValue py2val(PyObject *obj) const;
	virtual PyObject *py2pyval(PyObject *) const;

    virtual string val2str(TValue const) const;
    virtual string pyval2str(PyObject *) const;

    virtual PyObject *val2py(TValue const) const;
    virtual PyObject *pyval2py(PyObject *) const;

    virtual TValue str2val(string const &) const;
    virtual bool str2val_try(string const &, TValue &);
    virtual TValue str2val_add(string const &);
    virtual PyObject *str2pyval(string const &) const;

    virtual string val2filestr(TValue const) const;
    virtual string pyval2filestr(PyObject *) const;

    virtual TValue filestr2val(string const &);
    virtual PyObject *filestr2pyval(string const &);

    virtual bool   firstValue(TValue &) const;
    virtual bool   nextValue(TValue &) const;
    virtual TValue randomValue(int const rand=-1) const;

    // Returns the number of different values, -1 if it cannot be done (for example, if variable is continuous)
    virtual int  noOfValues() const;

    virtual TValue computeValue(TExample const *const);

    /// @cond Python
    static int vartypeconverter(PyObject *obj, int *varType);

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *keywords) PYDOC("(name, type[, ordered_values, unordered_values, create_new_on]) -> Variable; return existing or new variable)");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); return arguments for unpickling");
    PyObject *__richcmp__(PyObject *other, int op) const;
    PyObject *__repr__() const;

    static PyObject *py_getOrMake(PyObject *args, PyObject *kw,
                                  char **keywords,
                                  getMakeMeth meth);

    static PyObject *py_retrieve(PyObject *, PyObject *, PyObject *) PYARGS(METH_BOTH | METH_STATIC, "(name, type[, ordered_values, unordered_values, fail_on]) -> (Variable|None, status)");
    static PyObject *py_make(PyObject *, PyObject *, PyObject *) PYARGS(METH_BOTH | METH_STATIC, "(name, type[, ordered_values, unordered_values, create_new_on]) -> (Variable|None, status)");

    static PyObject *__get__name(OrVariable *self);
    static int __set__name(OrVariable *self, PyObject *value);

    PyObject *py_randomvalue() const PYARGS(METH_NOARGS, "() -> Value");
    PyObject *py_computeValue(PyObject *args, PyObject *kw) PYARGS(METH_VARARGS, "(instance) -> Value");

    // these are all the same; the first one is OK, others are obsolete
    PyObject *undefined() const PYARGS(METH_NOARGS, "() -> Value");
    PyObject *DC() const PYARGS(METH_NOARGS, "() -> Value");
    PyObject *DK() const PYARGS(METH_NOARGS, "() -> Value");
    PyObject *specialValue() const PYARGS(METH_NOARGS, "() -> Value");
    /// @endcond
};

typedef TOrangeVector<PVariable, TWrappedReferenceHandler<PVariable>, &OrVarList_Type> TVarList;

PYVECTOR(VarList, Variable)

/*! Returns \c true is the variable stores the value as double, that is,
    the variable is TContinuousVariable or TDiscreteVariable */
bool TVariable::isPrimitive() const 
{
    return (varType == Discrete) || (varType == Continuous);
}


/*! Get the variable's name */
string const &TVariable::getName() const
{
    return name;
}

/*! Get the variable's name as char * */
char const *TVariable::cname() const
{
    return name.c_str();
}

/*! Set the variable's name */
void TVariable::setName(string const &a)
{
    unregisterVariable();
    name = a;
    registerVariable();
}

#endif
