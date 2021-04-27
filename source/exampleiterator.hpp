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


#ifndef __EXAMPLEITERATOR_HPP
#define __EXAMPLEITERATOR_HPP

class TExampleIterator {
    PExampleTable table; /*!< Table through which the object iterates */
    unsigned int exampleIndex; /*!< Index of the current example */
    TExample example;   /*!< Current example; it can be a copy or reference */
    bool exampleOutdated; /*!< Flag telling that the example needs to be updated at dereferencing */
    int nAttrs; /*!< The number of variables in the domain */
    int classOffset; /*!< Class offset faster access to the class */

public:
    inline TExampleIterator(PExampleTable tab);
    inline TExampleIterator(const TExampleIterator &old);
    inline TExampleIterator &operator =(TExampleIterator const &old);

    inline operator bool() const;
    inline int index() const;
    inline void seek(const int);
    inline TExampleIterator &operator ++();
    inline TExampleIterator &operator --();
    inline TExample *operator *();
    inline TExample *operator ->();

    inline int &getMetaHandle();
    inline int getMetaHandle() const;

    inline double getClass() const;
    inline double value_at(const int &pos) const;
    inline double getMeta(const int &pos) const;
    inline double getWeight() const;
    inline void setWeight(double const);
};

#define EITERATE(it,co) for(TExampleTable::iterator it(&co); it; ++it)
#define PEITERATE(it,co) for(TExampleTable::iterator it(co); it; ++it)


/*! Construct a new iterator for the given table
    \param table The table over which the object will iterate
*/
TExampleIterator::TExampleIterator(PExampleTable table)
    : table(table),
      exampleIndex(0),
      example(table->isContiguous && (table->dataType == 'd') ? TExample::Direct : TExample::Indirect,
               table->domain, table),
      exampleOutdated(true),
      nAttrs(table->domain->variables->size()),
      classOffset((table->domain->variables->size()-1) * sizeof(double))
{ 
    if (example.referenceType == TExample::Indirect) {
        example.values = new double[nAttrs];
        example.values_end = example.values + nAttrs;
    }
}


/*! Copy constructor for the iterator */
TExampleIterator::TExampleIterator(const TExampleIterator &old)
    : table(old.table),
      example(old.example.referenceType, old.table->domain, old.table),
      exampleIndex(old.exampleIndex),
      exampleOutdated(true),
      nAttrs(old.nAttrs),
      classOffset(old.classOffset)
{ 
    if (example.referenceType == TExample::Indirect) {
        example.values = new double[nAttrs];
        example.values_end = example.values + nAttrs;
    }

}

/*! Copy operator for the iterator */
TExampleIterator &TExampleIterator::operator =(TExampleIterator const &old)
{
    if (this == &old) {
        return *this;
    }
    table = old.table;
    exampleIndex = old.exampleIndex;
    example = TExample(old.example.referenceType, old.table->domain, old.table);
    nAttrs = old.nAttrs;
    classOffset = old.classOffset;
    if (example.referenceType == TExample::Indirect) {
        example.values = new double[nAttrs];
        example.values_end = example.values + nAttrs;
    }
    exampleOutdated = true;
    return *this;
}


/*! Tells whether the iterator is still inside the table; \c false if
   iteration is finished */
TExampleIterator::operator bool() const
{ 
    return exampleIndex < table->examples.size();
}

/*! Return the index of the example within the table */
int TExampleIterator::index() const
{ 
    return exampleIndex;
}

/*! Move the iterator to the given position in the table */
void TExampleIterator::seek(int const pos)
{
    exampleIndex = pos;
    exampleOutdated = true;
}

/*! Increment the iterator to point to the next example */
TExampleIterator &TExampleIterator::operator ++()
{
    exampleIndex++;
    exampleOutdated = true;
    return *this;
}


/*! Decrement the iterator to point to the previous example */
TExampleIterator &TExampleIterator::operator --()
{
    exampleIndex--;
    exampleOutdated = true;
    return *this;
}


/*! Dereference the iterator
    Returns a pointer to the stored example. If example is not up-to-date, 
    it is updated. If the table is not contiguous or the table's
    #TExampleTable::dataType is not 'd',
    the values are copied to the iterator's #example. Otherwise, the example's
    #TExample::values is updated to point to the corresponding row of the
    table.

    \see exampleOutdated
*/
TExample *TExampleIterator::operator *()
{
    if (exampleOutdated) {
        if (example.referenceType == TExample::Indirect) {
            table->copyDataToExample(
                const_cast<TValue *>(example.values),
                table->examples[exampleIndex]);
        }
        else {
            example.values = (double *)table->examples[exampleIndex];
            example.values_end = example.values + nAttrs;
        }
        example.exampleIndex = exampleIndex;
        exampleOutdated = false;
    }
    return &example;
}


/*! Dereference the iterator

    Returns a reference to the stored example, which is updated if necessary.

    Return a pointer to the stored example. If example is not up-to-date, 
    it is updated. If the table is not contiguous or the table's
    #TExampleTable::dataType is not 'd',
    the values are copied to the iterator's #example. Otherwise, the example's
    #TExample::values is updated to point to the corresponding row of the
    table.

    \see exampleOutdated
*/
TExample *TExampleIterator::operator ->()
{
    return operator *();
}

/*! Return the reference to the index of the first meta value; raises
    exception if meta values are not supported

    \see getFirstMetaIndex() const
    \see TExampleTable::getMetaHandle(char *const row)
    \see TExampleTable::getMetaHandle(int const i)
*/
int &TExampleIterator::getMetaHandle()
{
    return table->getMetaHandle(exampleIndex);
}

/*! Return the reference to the index of the first meta value

    \see getFirstMetaIndex() const
    \see TExampleTable::getMetaHandle(char *const row)
    \see TExampleTable::getMetaHandle(int const i)
*/
int TExampleIterator::getMetaHandle() const
{
    return table->getMetaHandle(exampleIndex);
}

/*! Return the example's class; if example is not updated, the value is retrieved
    from the table.

    \see value_at(const int &)
*/
double TExampleIterator::getClass() const
{
/*  This is here just so that I find it when I'll search for "PyExc_NotImplemented" ;)
        if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError, "Only arrays of doubles are currently supported");
    }
*/
    if (exampleOutdated) {
        if ((example.referenceType == TExample::Indirect) && !table->isContiguous) {
            return *(double *)(table->examples[exampleIndex] + table->attributeOffsets.back());
        }
        else {
            return *(double *)(table->examples[exampleIndex] + classOffset);
        }
    }
    else {
        return example.values_end[-1];
    }
}


/*! Return the example's weight or 1.0 if examples are unweighted */
double TExampleIterator::getWeight() const
{
    return table->hasWeights() ? table->weights[exampleIndex] : 1.0;
}


/*! Set the example's weight */
void TExampleIterator::setWeight(double const weight)
{
    table->ensureWeights();
    table->weights[exampleIndex] = weight;
}


/*! Return the i-th value of the example; if example is not updated, the value
    is retrieved from the table

    \param i The index of the attribute; can also be -1 or meta id

    \see getClass()
*/
double TExampleIterator::value_at(const int &i) const
{
/*  This is here just so that I find it when I'll search for "PyExc_NotImplemented" ;)
        if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError, "Only arrays of doubles are currently supported");
    }
*/
    if (i >= 0) {
        if (exampleOutdated) {
            if ((example.referenceType == TExample::Indirect) && !table->isContiguous) {
                return *(double *)(table->examples[exampleIndex] + table->attributeOffsets[i]);
            }
            else {
                return ((double *)(table->examples[exampleIndex]))[i];
            }
        }
        else {
            return example.values[i];
        }
    }
    else {
        return getMeta(i);
    }
}

/*! Return the meta value with the given id.

    \param id The meta attribute Id

    If the attribute does not exist, exception is raised.

    \see getClass() const
    \see value_at(const int &) const
*/
double TExampleIterator::getMeta(const int &id) const
{
    if (id == -1) {
        return value_at(nAttrs-1);
    }
    TMetaValue mr = MetaChain::get(table->getMetaHandle(exampleIndex), id);
    if (!mr.isPrimitive) {
        raiseError(PyExc_ValueError,
            "value of meta attribute %i is not primitive", id);
    }
    return mr.value;
}

#endif

