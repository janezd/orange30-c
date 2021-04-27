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


#ifndef __EXAMPLETABLE_INLINES_HPP
#define __EXAMPLETABLE_INLINES_HPP

/*! Construct an empty table. 
    
    \param domain Domain of the new table
    \param reserveRows The number of allocated rows.
        If 0, rows will be allocated as needed.
    \param type The type of the constructed object
*/
PExampleTable TExampleTable::constructEmpty(PDomain const &domain,
    int const reserveRows, PyTypeObject *type)
{
    return PExampleTable(new(type) TExampleTable(domain, reserveRows));
}

/*! Construct an empty table that references another table

    \param base The table whose data is referenced
    \param type The type of the constructed object

This increases the #referenceCount of the referenced table so that its #data
cannot be resized.

If the referenced table itself references another table
(i.e. #base->#base` is not \c NULL), this table will reference #base->#base. 
*/
PExampleTable TExampleTable::constructEmptyReference(
    PExampleTable const &base, PyTypeObject *type)
{
    return PExampleTable(new(type) TExampleTable(base));
}

/*! Construct a table of references to examples in the existing table.

    \param base the table to be referenced
    \param type the type of the constructed object
*/
PExampleTable TExampleTable::constructReference(
    PExampleTable const &base, PyTypeObject *type)
{
    TExampleTable *me = new(type) TExampleTable(base);
    me->examples = base->examples;
    me->weights = base->weights;
    return PExampleTable(me);
}

/*! Construct a new table by converting the data instances from the
    existing table to the given domain.

    \param base The table whose data is converted to the new table
    \param domain New domain for the data
    \param skipMetas If \c true, meta attributes that are not included
            in the new domain are not copied
    \param type The type of the constructed object
*/
PExampleTable TExampleTable::constructConverted(PExampleTable const &base,
    PDomain const &domain, bool const skipMetas, PyTypeObject *type)
{
    return PExampleTable(new(type) TExampleTable(domain, base, skipMetas));
}

#ifndef NO_NUMPY
/*! Construct a table from numpy array, either by referencing or copying the data.

    \param np Numpy array
    \param domain Domain of the new table
    \param copy If \c false (default) the table references the array's
    data; otherwise it copies it
    \param type The type of the constructed object
*/
PExampleTable TExampleTable::constructFromNumpy(PyObject *np,
    PDomain const &domain, bool const copy, PyTypeObject *type)
{
    return PExampleTable(new(type) TExampleTable(domain, np, copy));
}
#endif

/*! Raises an exception if the table is referenced by another object, such as
    another #TExampleTable, numpy's array or #TExample.

    \see referenceCount
*/
void TExampleTable::testLocked()
{
    if (referenceCount) {
        raiseError(PyExc_RuntimeError, 
            "operation is not permitted since the table's data is shared");
    }
}


/*! Return \c true if the table references data in another table */
bool TExampleTable::isReference() const
{
    return base != NULL;
}


/*! Return the pointer to #data and increases the #referenceCount. */
char *TExampleTable::getRawDataPtr()
{
    referenceCount++;
    return data;
}


/*! Raises an exception if the table does not support meta attributes. */
void TExampleTable::checkSupportsMeta() const
{
    if (!supportsMeta()) {
        raiseError(PyExc_ValueError, "this table does not allow meta attributes");
    }
}

/*! Raises an exception if the table does not support meta attributes. */
bool TExampleTable::supportsMeta() const
{
    return metaOffset >= 0;
}


/*! Return the reference to the index of the given row; raises exception
    if meta values are not supported. */
int &TExampleTable::getMetaHandle(char *const row)
{
    checkSupportsMeta();
    return *(int *)(row + metaOffset);
}


/*! Return the index of meta value for the given row or 0 if meta values
    are not supported. */
int TExampleTable::getMetaHandle(char *const row) const
{
    return supportsMeta() ? *(int *)(row + metaOffset) : 0;
}


/*! Return the reference to the index of the first meta attribute for the
    given example; raises exception if meta values are not supported.
    \param exampleIdx Index of the example
*/
int &TExampleTable::getMetaHandle(int const exampleIdx)
{
    return getMetaHandle(examples[exampleIdx]);
}


/*! Return the of the first meta attribute for the given example or 0 if
    meta vaues are not supported.
    \param exampleIdx Index of the example
*/
int TExampleTable::getMetaHandle(int const exampleIdx) const
{
    return getMetaHandle(examples[exampleIdx]);
}
/*! Return the value of the given attribute for the given example

    \param idx Index of example
    \param attr Index of the attribute

    Method currently works only for data tables represented with doubles
    (#dataType 'd').

    If the index is negative, the meta attribute must exist and have a primitive
    value.
*/
TValue const &TExampleTable::at(int const idx, TAttrIdx const attr) const
{
    // nothing really ugly here: the method itself does not change the instance,
    // it's only the result that is const
    return const_cast<TExampleTable *>(this)->at(idx, attr);
}

/*! Add the example given as PExample. See #push_back(TExample const *example)
    for details. 
    
    \param example Example that is added to the table
*/
void TExampleTable::push_back(PExample const &example)
{
    push_back(example.borrowPtr());
}


/*! A quick push_back for reference examples. This function does not perform
    any checks; use with great care!
    
    \param example Example that is added to the table
*/
void TExampleTable::fast_push_back(TExample const *example)
{
    examples.push_back(example->referenced_row ?
        example->referenced_row : (char *)example->values);
    if (hasWeights()) {
        weights.push_back(example->getWeight());
    }
}

/*! Return an iterator initialized to the first example.

    Calling this method prevents reallocation of #data and removal of
    examples until the iterator is destructed.
*/
TExampleTable::iterator TExampleTable::begin()
{
    return iterator(PExampleTable::fromBorrowedPtr(this));
}


/*! Return the number of examples in the table.

    \see empty() const
*/
int TExampleTable::size() const
{ 
    return examples.size();
}


/*! Tells whether the table is empty.

    \see size() const
*/
bool TExampleTable::empty() const
{ 
    return !examples.size();
}


/*! \c true if the table has weights.*/
bool TExampleTable::hasWeights() const
{
    return !weights.empty();
}


/*! If #weights are empty, initialize them to a vector of 1.0 */
void TExampleTable::ensureWeights()
{
    if (!hasWeights()) {
        weights.resize(size(), 1.0);
    }
}

/*! Remove example weights */
void TExampleTable::removeWeights()
{
    weights.clear();
}

/*! Return the weight of the given example 

    \param idx Example index

    \see setWeight(int const, double const)
*/
double TExampleTable::getWeight(int const idx) const
{
    return hasWeights() ? weights[idx] : 1.0;
}

/*! Set the weight of the given example 

    \param idx Example index
    \param weight Weight

    \see getWeight(int const)
    \see setWeights(int const)
*/
void TExampleTable::setWeight(int const idx, double const weight)
{
    ensureWeights();
    weights[idx] = weight;
}

/*! Set weights for all examples
    \param weight Weight
*/
void TExampleTable::setWeights(double const weight)
{
    weights.assign(size(), weight);
}

#endif

