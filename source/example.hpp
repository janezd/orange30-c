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


#ifndef __EXAMPLE_HPP
#define __EXAMPLE_HPP

#include "example.ppp"

/*! \file */

class TVariable;

/*! Represents an example (data instance, row). Example contains data
    which is written as an array of doubles, each corresponding to a
    value of one variable from the domain. Data is always contiguous.

    An example can be related to a table or free. This is indicated by
    \c referenceType that can have three different values: \c Free
    represents a free example, unrelated to any #TExampleTable or
    another collection of examples. Such examples own the data pointed
    to by #values.

    Reference type \c Direct represents examples whose #values point
    to a row in #TExampleTable. Modifying such example directly
    modifies the data in the table.

    Type \c Indirect represents an example whose data needed to be
    copied from the table since the data in the table is not
    contiguous or the values are not represented as \c
    double. Although such examples still refer to a table, modifying
    them may not immediatelly change the data in the table. The
    example owns it data (#values). 

    Free examples have their own chains of meta attributes and
    weights, while others refer to the data in the referenced table.

    Some examples are copied from the table but are then
    independent. They are marked as \c Free.

    Instances of #TExample are constructed by named
    constructors. #constructFree and #constructCopy construct \c Free
    examples. #constructTableItem constructs a direct reference if the
    #TExampleTable stores the data as an array of doubles with an
    attribute stride of <tt>sizeof(double)</tt>; otherwise it copies
    the data and constructs an indirect reference example.  */

class TExample : public TOrange {
    friend class TExampleIterator;
    friend class TExampleTable;
    friend class TDomain;

    TExample(int const reftype, PDomain const &dom, PExampleTable const &tab,
        TValue *const vals=NULL, int const exampleIdx=-1, char *const refrow=NULL);
    TExample(TExample const &);

protected:
    /*! Example's data. The data is either owned by the example (if \c
       referenceType is \c Free or \c Indirect) or a pointer to the
       corresponding row in the table (\c Direct).

       The field is \c const to prevent uncontrolled changes if
       if the example is an \c Indirect reference. For \c
       Free and \c Direct examples it may be safely cast away.
    */
    TValue const *values;
    
    /*! The pointer beyond the last of example's regular values. */
    TValue const *values_end;

    /*! A reference to a row in the table; used for \c Indirect
      examples. Equivalent to <tt>table->examples[exampleIndex]</tt>, but
      saves lookups. */
    char *referenced_row;

    /*! The handle of the first meta attribute in the chain; used only
      for \c Free examples. */
    int firstMeta;

    /*! The weight of the example; used only for #Free examples */
    double weight;

public:
    /*! Iterator type for TExample */
    typedef TValue *iterator;
    /*! Constant iterator type for TExample */
    typedef TValue const *const_iterator;

    __REGISTER_CLASS(Example);
 
    /// Example's domain
    PDomain domain; //PR Data instance's domain

    /// The table to which the example belongs (for \c Direct and \c Indirect)
    PExampleTable table; //PR Table to which the data instance belongs

    /*! \enum ReferenceType
        Possible values for #referenceType (see the overview of TExample for details)

        \var Free
        Example is not included in any TExampleTable and owns its data

        \var Indirect
        Example is from a table in a different format, so it owns
        its data and its values are written back to the table

        \var Direct
        Example refers to the data in the table */
    enum ReferenceType {Free, Indirect, Direct}; PYCLASSCONSTANTS; 

    /*! Reference type; see the overview of TExample for details */
    int referenceType; //PR(&ReferenceType) describes the example's relation with the data collection

    /*! The index of the example in the table (for \c Direct and \c Indirect) */
    int exampleIndex; //PR index of example in the example table to which it belongs
    
    inline ~TExample();

    static PExample constructFree(PDomain const &dom, TValue *data=NULL);
    static PExample constructFree(PDomain const &dom, vector<double> const &data);
    static PExample constructCopy(PExampleTable const &tab, int const exampleIdx);
    static inline PExample constructCopy(TExample const &example);
    static inline PExample constructCopy(PExample const &example);
    static PExample constructTableItem(PExampleTable const &tab, int const exampleIdx);
    PExample convertedTo(PDomain const &dom, bool const filterMetas=false) const;

    inline iterator begin();
    inline iterator end();
    inline iterator end_features();

    inline const_iterator begin() const;
    inline const_iterator end() const;
    inline const_iterator end_features() const;

    // Only the const operator is provided to prevent modifying copies of examples.
    // setValue should be used for setting values.
    inline TValue const operator [](TAttrIdx const) const;

    inline TValue const getValue(TAttrIdx const) const;
    inline TValue const getValue(string const &) const;
    inline TValue const getValue(PVariable const &) const;

    inline void setValue(int const, TValue const);
    inline void setValue(string const &, TValue const);
    inline void setValue(PVariable const &, TValue const);
    void setValue_indirect(TAttrIdx const i, TValue const v);

    inline TValue const getClass() const;
    inline void setClass(TValue const);

    inline double getWeight() const;
    inline void setWeight(double const weight);

    void addToCRC(unsigned int  &crc, const bool includeMetas) const;
    inline unsigned int checkSum(const bool includeMetas=false) const;

protected:
    inline int &getMetaHandle();
    inline int getMetaHandle() const;

public:
    inline bool supportsMeta() const;
    inline void checkSupportsMeta() const;
    inline bool hasMeta(TMetaId const) const;
    inline TMetaValue getMeta(TMetaId const i) const;
    inline TMetaValue getMetaIfExists(TMetaId const) const;
    inline void setMeta(TMetaValue const &);
    inline void setMeta(TMetaId const, TValue const);
    inline void setMeta(TMetaId const, PyObject *);
    inline void removeMeta(TMetaId const);
    inline void removeMetas();
    inline TMetaValue nextMeta(int &);

    int cmp(TExample const &other) const;
    inline bool operator==(TExample const &other) const;

    /// @cond Python
    static PExample fromDomainAndPyObject(PDomain const &, PyObject *, 
                                          bool const checkDomain=true);
    PyObject *convertToPythonNative(int const natvt=1) const;

    string toString(int limitFeatures=9999, int limitMeta=9999) const;

    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(domain[, values]); Construct a new example");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    PyObject *__item__(ssize_t idx);
    Py_ssize_t __len__() const;
    PyObject *__subscript__(PyObject *index);
    int __ass_subscript__(PyObject *index, PyObject *value);
    PyObject *__repr__() const;
    PyObject *__str__() const;
    long __hash__() const;
    PyObject *__richcmp__(PyObject *other, int op) const;

    static PyObject *__get__id(OrExample *self); PYDOC("weak id of example: equal id's represent examples from the same source\n(e.g. same line in the file, although possible with different attributes)");

    PyObject *py_getclass() const PYARGS(METH_NOARGS, "(); return the example's class");
    PyObject *py_setclass(PyObject *val) PYARGS(METH_O, "(value); set the class");
    PyObject *native(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(level); convert an example into a Python list");

    PyObject *py_set_meta(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(variable, value); set meta value (obsolete)");
    PyObject *py_get_meta(PyObject *args) const PYARGS(METH_O, "(attr); get meta value (obsolete)");
    PyObject *py_get_metas(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "");
    PyObject *py_has_meta(PyObject *args) const PYARGS(METH_O, "(id or string or variable)");
    PyObject *py_remove_meta(PyObject *arg) PYARGS(METH_O, "(id or string or variable)");
    PyObject *py_get_weight() const PYARGS(METH_NOARGS, "()");
    PyObject *py_set_weight(PyObject *args) PYARGS(METH_O, "(weight)");
    /// @endcond
};

/*! A macro for iteration over meta values of an example; the macro leaves
    an open block.
    \param example The example
    \param value The name of the variable that will contain the meta value
    Example
    \code
ITERATE_METAS(example, mr)
    if (mr.isPrimitive) {
        // do something
    }
    else {
        // do something else
    }
} // do not forget to add the closing parentheses
    \endcode
*/
#define ITERATE_METAS(example,value) \
    for(;;) { \
         int _iterate_metas_handle = 0; \
         TMetaValue value = (example).nextMeta(_iterate_metas_handle); \
         if (value.id) { \
             break; \
         }


/*! Destructor. Frees #values for non-#Direct examples, meta chain for
    #Free examples and reference count for #table for non-#Free ones. */
TExample::~TExample()
{
    if (referenceType != TExample::Direct) {
        delete values;
    }
    if (referenceType == TExample::Free) {
        MetaChain::freeChain(firstMeta);
    }
    else {
        table->referenceCount--;
    }
}

/*! Copies the given example.
    \param example Example to be copied */
inline PExample TExample::constructCopy(TExample const &example)
{
    return PExample(new TExample(example));
}

/*! Convenience function for copying examples; calls #constructCopy(TExample const &)
    \param example Example to be copied */
inline PExample TExample::constructCopy(PExample const &example)
{
    return constructCopy(*example);
}

/*! Return an iterator over the example's values. Raises an exception
    if the example is \c Indirect to ensure that the values are not
    changed without changing the underlying table. The corresponding
    method for const examples also works for indirect examples.

    \see begin() const 
    \see end() const */
TExample::iterator TExample::begin()
{ 
    if (referenceType == Indirect) {
        raiseError(PyExc_SystemError,
            "Examples which indirectly refer to tabels cannot provide non-const iterators");
    }
    return const_cast<TValue *>(values);
}

/*! Return an iterator pointing to the end of example's values.

    \see begin()
    \see begin() const */
TExample::iterator TExample::end()
{ return const_cast<TValue *>(values_end); }


/*! Return an iterator pointing to the end of values of example's features.

    \see begin()
    \see begin() const */
TExample::iterator TExample::end_features()
{ return const_cast<TValue *>(domain->classVar ? values_end-1 : values_end); }


/*! Return an iterator over the example's values. Works for all types
    of examples, including the \c Direct,

    \see begin() const
    \see end() const  */
TExample::const_iterator TExample::begin() const
{ return values; }


/*! Return an iterator pointing to the end of example's values.

    \see begin()
    \see begin() const */
TExample::const_iterator TExample::end() const
{ return values_end; }


/*! Return an iterator pointing to the end of values of example's features.

    \see begin()
    \see begin() const */
TExample::const_iterator TExample::end_features() const
{ return domain->classVar ? values_end-1 : values_end; }


/*! Return the value of the given attribute with index \c i. Index can
    also be -1, representing the class, or negative, for meta
    attributes. The meta attribute must exist and be primitive
    (discrete or numeric). 

    \param i Attribute index */
TValue const TExample::operator [](TAttrIdx const i) const
{
    if (i >= 0) {
        return values[i];
    }
    if (i == -1) {
        return values_end[-1];
    }
    else {
        TMetaValue val = getMeta(i);
        if (!val.isPrimitive) {
            raiseError(PyExc_SystemError,
                "TExample::operator[] can only retrieve primitive values");
        }
        return val.value;
    }
}


/*! Compares two examples; calls #cmp. */
bool TExample::operator==(TExample const &other) const
{
    return !cmp(other);
}

/*! Return the value of the given attribute. Alias for
  #operator[](TAttrIdx const i) const 

    \see getValue(string const &)
    \see getValue(PVariable const &)
*/
TValue const TExample::getValue(TAttrIdx const i) const
{
    return operator[](i);
}


/*! Return the example's class. */
TValue const TExample::getClass() const
{
    return values_end[-1];
}


/*! Set the example's class. */
void TExample::setClass(TValue const v)
{
    if (referenceType == Indirect) {
        setValue_indirect(-1, v);
    }
    const_cast<TValue &>(values_end[-1]) = v;
}


/*! Get the value of the variable with the given name. Works also for
    meta attributes.
    \param name The name of the variable
    \see getValue(PVariable const &)
    \see getValue(TAttrIdx const)
*/

TValue const TExample::getValue(string const &name) const
{
    return getValue(domain->getVarNum(name));
}


/*! Get the value of the variable with the given descriptor. Works
    also for meta attributes.
    \param var The variable's descriptor
    \see getValue(string const &)
    \see getValue(TAttrIdx const)
*/
TValue const TExample::getValue(PVariable const &var) const
{
    return getValue(domain->getVarNum(var));
}


/*! Set the value of the variable with the given name.
    \param name The name of the variable
    \param value The assigned value
    \see setValue(PVariable const &, TValue const)
    \see setValue(TAttrIdx const, TValue const)
 */
void TExample::setValue(string const &name, TValue const value)
{
    setValue(domain->getVarNum(name), value);
}


/*! Set the value of the variable with the given descriptor.
    \param var The variable's descriptor
    \param value The assigned value
    \see setValue(TAttrIdx const, TValue const)
    \see setValue(string const &, TValue const)
*/
void TExample::setValue(PVariable const &var, TValue const value)
{
    setValue(domain->getVarNum(var), value);
}


/*! Set the value of the variable at the given index.
    \param i The variable index
    \param value The assigned value
    \see setValue(PVariable const &, TValue const)
    \see setValue(string const &, TValue const)
 */
void TExample::setValue(TAttrIdx const i, TValue const value) 
{
    if (i < -1) {
        PVariable var = domain->getMetaVar(i);
        if (var && !var->isPrimitive()) {
            raiseError(PyExc_SystemError,
                "TExample::setValue can only set primitive values");
        }
        setMeta(i, value);
    }
    if (referenceType == Indirect) {
        setValue_indirect(i, value);
    }
    if (i >= 0) {
        const_cast<TValue &>(values[i]) = value;
    }
    else {
        const_cast<TValue &>(values_end[-1]) = value;
    }
}


/*! Indicate whether the example supports meta attributes. Always \c
  true for \c Free examples, for others it depends upon the table.
  \see checkSupportsMeta() const */ 
bool TExample::supportsMeta() const
{
    return (referenceType == Free) || table->supportsMeta();
}


/*! Raise an exception if the example does not support meta
    attributes.
    \see supportsMeta() const */
void TExample::checkSupportsMeta() const
{
    if (referenceType != Free) {
        table->checkSupportsMeta();
    }
}


/*! Return a reference to the variable containing the first meta
    attribute. For \c Free examples, this is #firstMeta, for others
    the #TExampleTable::getMetaHandle is called. Raises an exception
    if meta attributes are not supported. 
    /see getMetaHandle() const
    /see supportsMeta() const */
int &TExample::getMetaHandle()
{
    return referenceType == Free ? 
        firstMeta : table->getMetaHandle(exampleIndex);
}


/*! Return a reference to the variable containing the first meta
    attribute or 0 if meta attributes are not supported. The latter
    has the same effect as if the meta attributes were supported but
    the example had none. For \c Free examples, this is #firstMeta, for others
    the #TExampleTable::getMetaHandle is called.

    /see getMetaHandle() */
int TExample::getMetaHandle() const
{
    if (referenceType == Free) {
        return firstMeta;
    }
    if (table->supportsMeta()) {
        return table->getMetaHandle(exampleIndex);
    }
    return 0;
}

/*! Return the meta value corresponding to the given id. Exception is
    raised if the attribute does not exist.
    \param id Id of the meta value. 
    \see hasMeta(TMetaId const) const
    \see getMetaIfExists(TMetaId const) const */
TMetaValue TExample::getMeta(TMetaId const id) const
{
    return MetaChain::get(getMetaHandle(), id);
}


/*! Return the meta value corresponding to the given id. If the example
    has no meta value with such id, a MetaValue with zero id is returned.
    \param id Id of the meta value. 
    \see hasMeta() const */
TMetaValue TExample::getMetaIfExists(TMetaId const id) const
{
    return MetaChain::get(getMetaHandle(), id, false);
}


/*! Indicate whether the example has a meta value with the given
  id.
  \param id Id of the meta value */
bool TExample::hasMeta(TMetaId const id) const
{
    return MetaChain::has(getMetaHandle(), id);
}

/*! Set the value of the meta attribute with the given id.
     Raises exception of the
     example does not support meta attributes.
    \param id Id of the meta value
    \param value Assigned value
    \see setMeta(TMetaId const id, PyObject *)
    \see setMeta(TMetaValue const &)
*/
void TExample::setMeta(TMetaId const id, TValue const value)
{
    MetaChain::set(getMetaHandle(), id, value);
}

/*! Set the value of the meta attribute with the given id.  Raises exception of the
    example does not support meta attributes.
    \param id Id of the meta value
    \param value Assigned value
    \see setMeta(TMetaId const id, TValue const)
    \see setMeta(TMetaValue const &)
*/
void TExample::setMeta(TMetaId const id, PyObject *value)
{
    MetaChain::set(getMetaHandle(), id, value);
}


/*! Set the value of the meta attribute. Raises exception of the
    example does not support meta attributes.
    \param value Assigned value; also includes the id
    \see setMeta(TMetaId const id, TValue const &val)
    \see setMeta(TMetaId const id, PyObject *val)
*/
void TExample::setMeta(TMetaValue const &value)
{
    MetaChain::set(getMetaHandle(), value);
}


/*! Remove the meta attribute with the given id
    \param id Id of the meta attribute
    \see removeMetas() */
void TExample::removeMeta(TMetaId const id)
{
    MetaChain::remove(getMetaHandle(), id);
}


/*! Remove all example's meta attributes.
    \see removeMeta(TMetaId const) */
void TExample::removeMetas()
{
    MetaChain::freeChain(getMetaHandle());
}


/*! Function used for iteration over meta values. The function is difficult
    to use; use macro #ITERATE_METAS instead.    
    \param handle A handle that should be initially set to 0 and is then
     changed by function
    \returns Meta value; the end of iteration is indicated by a dummy
      meta value with an id of 0. */
TMetaValue TExample::nextMeta(int &handle)
{
    handle = handle ? MetaChain::advance(handle) : getMetaHandle();
    return handle ? MetaChain::get(handle) : TMetaValue(0, 0.0);
}


/*! Get the weight of the example */
double TExample::getWeight() const
{
    return referenceType == Free ? weight : table->getWeight(exampleIndex);
}


/*! Set the example's weight */
void TExample::setWeight(double const aWeight)
{
    if (referenceType == Free) {
        weight = aWeight;
    }
    else {
        table->setWeight(exampleIndex, aWeight);
    }
}


/*! Compute a checksum of the example. 
    \param includeMetas If true, meta values included in the sum */
unsigned int TExample::checkSum(const bool includeMetas) const
{ 
    unsigned int crc;
    INIT_CRC(crc);
    addToCRC(crc, includeMetas);
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


#endif

