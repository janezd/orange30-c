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


#ifndef __CONTINUOUSVARIABLE_HPP
#define __CONTINUOUSVARIABLE_HPP

#include "continuousvariable.ppp"

/*! Descriptor for continuous variables. */
class TContinuousVariable : public TVariable {
public:
    __REGISTER_CLASS(ContinuousVariable);

    ///< Number of decimals used when printing out the value; default is 3
    mutable int numberOfDecimals; //P number of digits after decimal point

    ///< Sets whether to use the scientific format (%g) for printing the value
    mutable bool scientificFormat; //P use scientific format in output
    
    /*! If 0, the number of decimals is fixed; if 1 the number of decimals can
        be increased when #str2val is given a string with a greater number of
        decimals; if 2, no values have been converted yet, so the number of
        decimals is adjusted at the first call of #str2val */
    mutable int adjustDecimals; //P adjust number of decimals according to the values converted (0 - no, 1 - yes, 2 - yes, but haven't seen any yet)

    TContinuousVariable(string const &aname="");

    virtual bool isEquivalentTo(TVariable const &) const;

    virtual PyObject *val2py(TValue const val) const;
    virtual string val2str(TValue const val) const;
    virtual TValue str2val(string const &valname) const;
    virtual bool str2val_try(string const &valname, TValue &valu);

    /// @cond Python
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *keywords) PYDOC("(name); construct a continuous variable with the given name");
    PyObject *__richcmp__(PyObject *other, int op);
    /// @endcond
};

#endif
