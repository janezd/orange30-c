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


#ifndef __DISCRETEVARIABLE_HPP
#define __DISCRETEVARIABLE_HPP

#include "discretevariable.ppp"

/*! Descriptor for discrete variables. */

class TDiscreteVariable : public TVariable {
public:
    __REGISTER_CLASS(DiscreteVariable)

    /*! A list of attribute values. When the number of values is above 50,
        the TDiscreteVariable constructs a map #valuesTree for faster indexing
        of vaues. To ensure that the list and the map contain the same
        information, #values should never be modified directly. */       
    PStringList values; //P attribute's values

    /*! The index of the base value, which is used by, for instance,
        binarization and imputation. */
    int baseValue;      //P the index of the base value

    TDiscreteVariable(const string &aname = "");
    TDiscreteVariable(const string &aname, PStringList const &values);
    TDiscreteVariable(const TDiscreteVariable &);

    virtual bool isEquivalentTo(TVariable const &) const;

    void addValue(string const &);
    bool hasValue(string const &);

    virtual bool firstValue(TValue &val) const;
    virtual bool nextValue(TValue &val) const;
    virtual TValue randomValue(int const rand=-1) const;

    virtual inline int noOfValues() const;

    virtual TValue py2val(PyObject *obj) const;
    virtual string val2str(TValue const val) const;
    virtual PyObject *val2py(TValue const val) const;
    virtual TValue str2val(string const &valname) const;
    virtual bool str2val_try(string const &valname, TValue &valu) const;
    virtual TValue str2val_add(string const &valname);

    bool checkValuesOrder(TStringList const &refValues);
    static void presortValues(set<string> const &unsorted, vector<string> &sorted);

    static inline int toInt(TValue const val,
        char *msg_nondiscrete=NULL, char *msg_unknown=NULL);

private:
    /*! Maps value names to indices for faster indexing. */
    mutable map<string, int> valuesTree;
    void createValuesTree() const;

    /// @cond Python
public:
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *keywords) PYDOC("(name[, values]); (name); construct a continuous variable with the given name");
    PyObject *py_addValue(PyObject *args) PYARGS(METH_O, "(value); add a new value to the list of possible values of the feature");
    PyObject *__richcmp__(PyObject *other, int op);
    /// @endcond
};


/*! A static method for converting a double to an int. Exceptions are raised if
    the value is NaN or if it differs from a whole number for more than 1e-6. */
int TDiscreteVariable::toInt(TValue const val, char *msg_nondiscrete, char *msg_unknown)
{
    if (isnan(val)) {
        raiseError(PyExc_ValueError,
            msg_unknown ? msg_unknown : "value is undefined");
    }
    int const ival = int(val);
    if (fabs(ival - val) > 1e-6) {
        raiseError(PyExc_ValueError,
            msg_nondiscrete ? msg_nondiscrete : "value %.3f is not discrete", val);
    }
    return ival;
}


/*! Returns the number of values */
int  TDiscreteVariable::noOfValues() const
{ 
    return values->size(); 
}

#endif
