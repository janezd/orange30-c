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


#ifndef __EXAMPLETABLE_HPP
#define __EXAMPLETABLE_HPP

#include "exampletable.ppp"

/*! Storage for data instances. The data format is similar to numpy's
arrays, but enhanced with views that include arbitrary rows and columns.

The data can be in different formats, as described by #dataType.  Currently only
\c double is supported. The \c double type is also used internally; when other
formats are supported in the future, the data will often be converted internally
into arrays of doubles.

The table can either own data or reference data stored in another object (see
#base and #data), typically another instance of TExampleTable or a numpy
array. A reference can also include a subset of rows and columns from the
original table. It cannot add new data. The exception are tables that do not
refrence TExampleTable but, say, numpy array; such tables can add meta
attributes to examples, but not new examples (that is, rows) themselves.

Examples can be assigned weights. Unlike in previous versions of Orange, weights
are not meta attributes but reside in vector \c weights whose length is the same
as the number of examples (unless it is empty). Since weights are not meta
attributes, each attribute has only one weight. However, different references to
the same table can have different weights; when the reference table is created,
the weights are copied, not referenced. This simplifies handling weights in
comparison with previous versions since the id of the meta attribute of weight
no longer needs to be passed around as an additional argument.

The memory allocated for the table (#data) can only expand. Removing examples
puts their corresponding rows into a list of free rows for the table
(#freeRows), but does not decrease the table size. The only exception is method
#clear() that frees all the memory used for storing the examples.

Referenced tables cannot reallocate their data or remove examples. If a table is
referred to by a numpy \c array, the lock is permanent since the \c array
does not notify the table of its destruction. This is not a huge limitation
since tables are seldom resized after their construction is finished.

Tables mimic STL vectors by providing methods like #push_back and iterators
(#TExampleIterator). Iterators are lazy: they contain an instance of #TExample
that is returned when the iterator is dereferenced, but its content is only
updated at dereferencing. This is useful when the table is not contiguous
(e.g. contains only a selection of the referenced table's columns) and the
iterator is used to retrieve only values of a few attributes or a class. In
this case, the iterator provides a method for getting only a single value
without copying the entire example.

*/
class TExampleIterator;

extern PyObject *numpy_module, *ma_module;
extern PyObject *NDArray_TypePtr, *MaskedArray_TypePtr;

class TExampleTable : public TOrange
{
    /*! A static variable used for assigning table #version. */
    static int tableVersion;

    friend class TExampleIterator;
    friend class TExample;
    friend class TExampleTableReader;
    friend class PExampleTable;
    template<class T> friend class TCompareContiguousRows;
    template<class T> friend class TCompareRows;

    /*! A reference to the data owner; NULL if the tables owns the data.
        For tables that reference other object's data, this field
        holds the reference to that object. The referenced table
        cannot be a reference to another table. The corresponding
        constructor cuts short any references to references by
        referencing the original table.

        The allocated space can increase; it never decreases but can
        be freed is all examples are removed by calling #clear.

        This field can be used to determine whether the data is owned
        or referenced. */
    PyObject *base;


    /*! The number of allocated data rows. */
    int allocatedRows;
    
    /*! Indices of free rows (if the table owns its data; unused otherwise).
        When a data instance is deleted from the table, the index is stored
		here so the row can be reused. If this is empty, the number of used
		data rows equals ``examples->size()``. If
		``examples->size()==allocatedRows``, the table is full, so ``data``
		needs to be resized to add more examples. */
    vector<int> freeRows;

    /*! Indices of data rows, e.g. the data for the i-th data instance is at
        #data[rowsize*rows[i]]. */
    vector<int> rows;

    /*! Weights of examples; this vector is empty or has the same length as
        #examples. */
    vector<double> weights;


	/* * * * Dense attributes * * * */
    /*! Pointer to the dense data owned by the object; \c NULL if there is no
	    dense data or the object does not own its data. */
    char *data;

    /*! The size of data row (in bytes), e.g. if the object owns its data,
	    #data points to a block of memory of size #rowSize*#allocatedRows */
    int rowSize;

    /*! Offsets (in bytes) of attribute values within a row. Empty, if the table
        is contiguous and #dataType is 'd'. */
    vector<size_t> attributeOffsets;


	/* * * * Dense classes * * * */
    /*! Pointer to the dense class data owned by the object; \c NULL if there is
	    no dense data or the object does not own its data. */
    char *classData;

    /*! The size of class data row (in bytes), e.g. if the object owns its data,
	    #classData points to a block of memory of size #classRowSize*#allocatedRows */
    int classRowSize;

    /*! Offsets (in bytes) of class values within a row. Empty, if the table
        is contiguous and #dataType is 'd'. */
    vector<size_t> classOffsets;


	/* * * * Sparse attributes * * * */
	/*! */
	char *doubleSparse_data;
	int *doubleSparse_indices;
	int *doubleSparse_indPtr;

	vector<string> stringSparse_data;
	int *stringSparse_indices;
	int *stringSparse_indPtr;

	PyObject *objectSparse_data;
	int *objectSparse_indices;
	int *objectSparse_indPtr;

    /* * * * Sparse classes * * * */
	/*! */
	double *classSparse_data;
	int *classSparse_indices;
	int *classSparse_indPtr;

public:
    typedef TExampleIterator iterator;

    __REGISTER_CLASS(ExampleTable)

    /*! Domain description containing the features, class and meta attributes. */
    PDomain domain; //PR domain description

    /*! The data format using the same notation in numpy (currently only 'd' is
        supported). */
    char dataType; //PR data type (as in numpy)

    /*! The number of objects referring to this table. If not zero, it prevents
        removing the examples and resizing the data. New data instances can
        still be added for as long as there are free rows in the current data.

        Orange objects can free the lock by decreasing the reference
        count. Locks by non-Orange objects, such as numpy array, are
        permanent. Some tables are also created with non-zero count, for
        instance a table that uses data from numpy array.
    */
    int referenceCount; //PR number of ExampleTables and Examples referencing this table

    /*! Tells whether the row can be interpreted as an array of feature values.
        If false, #attributeOffsets need to be used to convert from feature
        indices into row offsets.

        \todo Is this useful for other data types at all? If not, we can merge
        it with dataType=='d' and call it something like isContiguousAndDouble.
    */
    bool isContiguous; //P example in the table are contiguous

    /*! Changes when the data is changed. This indicator is unreliable.
        \see tableVersion */
    int version; //P version; changes when the data changes (unreliable)

    /*! Random generator used by #shuffle(). */
    mutable PRandomGenerator randomGenerator; //P random generator used by shuffle

    /*! An enum used to describe how to store the table in classes like
        TTreeLearner or TAssociationRulesInducer */
    enum Store { DontStore, StoreCopy, StoreReference } PYCLASSCONSTANTS_UP;


private:
    TExampleTable(PDomain const &, int const reserveRows=0, int const rowSize=-1);
    TExampleTable(PDomain const &, PExampleTable const &, bool const skipMetas=false);
    TExampleTable(PExampleTable const &base);
#ifndef NO_NUMPY
    TExampleTable(PDomain const &, PyObject *numarray, bool const copy=false);
#endif
    ~TExampleTable();

public:
    static PExampleTable constructCopy(
        PExampleTable const &base,
        bool const skipMetas=false,
        PyTypeObject *type=&OrExampleTable_Type);

    inline static PExampleTable constructReference(
        PExampleTable const &base,
        PyTypeObject *type=&OrExampleTable_Type);

    inline static PExampleTable constructEmpty(
        PDomain const &,
        int const reserveRows=0,
        PyTypeObject *type=&OrExampleTable_Type);

    inline static PExampleTable constructEmptyReference(
        PExampleTable const &base,
        PyTypeObject *type=&OrExampleTable_Type);

    inline static PExampleTable constructConverted(
        PExampleTable const &base,
        PDomain const &,
        bool const skipMetas=false,
        PyTypeObject *type=&OrExampleTable_Type);

    static PExampleTable constructConvertedReference(
        PExampleTable const &base,
        PDomain const &,
        bool copyData = true,
        PyTypeObject *type=&OrExampleTable_Type);

    static PExampleTable constructByColumns(
        PExampleTable const &base,
        PDomain const &,
        vector<int> const &attributes,
        bool copyData = true,
        PyTypeObject *type=&OrExampleTable_Type);

#ifndef NO_NUMPY
    inline static PExampleTable constructFromNumpy(PyObject *np,
        PDomain const &, bool const copy = false,
        PyTypeObject *type=&OrExampleTable_Type);
#endif

    void reserve(int const i);
    void reserveRefs(int const i);
    void grow();

    inline bool isReference() const;

    inline void testLocked();
    inline char *getRawDataPtr();

    inline bool supportsMeta() const;
    inline void checkSupportsMeta() const;
    inline int &getMetaHandle(char *const row);
    inline int getMetaHandle(char *const row) const;
    inline int &getMetaHandle(int const i);
    inline int getMetaHandle(int const i) const;

    void copyDataToExample(TValue const *values, char const *const rowPtr) const;
    void copyDataToTable(char *rowPtr, TValue const *values);

    inline iterator begin();
    PExample at(int const idx);
    TValue &at(int const row, TAttrIdx const);
    inline TValue const &at(int const row, TAttrIdx const) const;
    inline int size() const;
    inline bool empty() const;

    int new_example();
    inline void push_back(PExample const &example);
    void push_back(TExample const *example);
    inline void fast_push_back(TExample const *example);
    void extend(PExampleTable const &table, bool skipMetas = false);
    void erase(const int idx);
    void erase(const int begidx, const int endidx);
    void clear();

    unsigned int checkSum(bool const includeMetas=false) const;
    double totalWeight() const;

    bool hasMissing() const;
    bool hasMissingClass() const;

    void shuffle();
    void shuffle(PRandomGenerator const &);

    void addMetaAttribute(TMetaId const, TValue const);
    void addMetaAttribute(TMetaId const, PyObject *);
    void removeMetaAttribute(TMetaId const);
    void copyMetaAttribute(TMetaId const dst, TMetaId const src, TValue defaultVal);

    inline bool hasWeights() const;
    inline void ensureWeights();
    inline void removeWeights();
    inline double getWeight(int const idx) const;
    inline void setWeight(int const idx, double const weight);
    inline void setWeights(double const weight);

    void reorderExamples(vector<int> const &indices);
    void sort();
    void sort(vector<int> const &sortOrder);
    void removeDuplicates(bool computeWeights);

    int countInFold(PIntList const &folds, int const fold) const;
    PExampleTable select(PIntList const &l, int const fold=-1) const;
    PExampleTable selectref(PIntList const &l, int const fold=-1) const;

    void filterInPlace(PFilter const &);

    void toFile_head3(FILE *file,
        char const delim, bool const listDiscreteValues) const;
    void toFile_head1(FILE *file,
        char const delim) const;
    void toFile_examples(FILE *file,
        char const delim, char const *const undefined) const;
    void toFile_examplesBasket(FILE *file) const;
    void toFile_basketValue(FILE *file,
        PVariable const &var, TMetaValue &val) const;
    static string toString_varType(
        PVariable const &var, bool const listDiscreteValues);
    static void toString_attributes(string &res, PVariable const &var);
    static bool toFile_checkNeedsD(PVariable const &var);
    void saveTab(char const *filename) const;
    void saveTxt(char const *filename) const;
    void saveCsv(char const *filename) const;
    void saveBasket(char const *filename) const;

    static PExampleTable fromDomainAndPyObject(
        PDomain const &, PyObject *,
        bool const checkDomain=true,
        PyTypeObject *type=&OrExampleTable_Type);

    /// @cond Python

    static PyObject *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("initialize a new table");
    static PyObject *unpickle_w_data(PyTypeObject *type, PyObject *args);
    static PyObject *unpickle_reference(PyTypeObject *type, PyObject *args);
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    Py_ssize_t __len__() const;
    int __bool__() const;
    PyObject *__item__(Py_ssize_t idx);
    PyObject *__inplace_concat__(PyObject *arg);

    static PyObject *__get__base(OrExampleTable *self);
    static PyObject *__get__columns(OrExampleTable *);

    PyObject *__repr__();

    bool extractIndices(PyObject *pyidx, bool const sorted, vector<int> &indices) const;
    PDomain domainForTranslation(PyObject *pyattrs, vector<TAttrIdx> &attributes,
                                 int &copy) const;
    PyObject *subscript_slice(PyObject *) const;
    PyObject *subscript_iter(PyObject *) const;
    PyObject *subscript_tuple(PyObject *) const;
    PyObject *subscript_tuple_value(int const idx, TAttrIdx const aidx) const;
    PyObject *subscript_tuple_table(PyObject *index, PyObject *attrs) const;
    PyObject *__subscript__(PyObject *index) const;

    int ass_subscript_delete(int const attr, vector<int> const &);
    int ass_subscript_attribute(int const attr, PyObject *, vector<int> const &);
    int ass_subscript_examples(PyObject *, vector<int> const &);
    int __ass_subscript__(PyObject *index, PyObject *value);

    PyObject *native(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(level); convert an example table into a Python list");
    PyObject *py_checksum(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(include_metas); return a checksum over the table's examplese");
    PyObject *randomExample() PYARGS(METH_NOARGS, "() -> return random example from the table");
    PyObject *py_clear() PYARGS(METH_NOARGS, "() -> remove all examples from the table");
    PyObject *py_hasMissing() const PYARGS(METH_NOARGS, "() -> True if there are missing values in the data");
    PyObject *py_hasMissingClass() const PYARGS(METH_NOARGS, "() -> True if there are missing class values in the data");
    PyObject *py_shuffle() PYARGS(METH_NOARGS, "() -> randomly shuffles examples");
    PyObject *py_addMetaAttribute(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "(attr, value) -> add meta value to all examples");
    PyObject *py_removeMetaAttribute(PyObject *arg) PYARGS(METH_O, "(id or string or descriptor) -> remove meta value from all examples");
    PyObject *py_sort(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "([variables]); sort examples by attribute values");
    PyObject *py_removeDuplicates() PYARGS(METH_NOARGS, "(); remove duplicated examples (ignoring meta values)");
    PyObject *py_changeDomain(PyObject *arg) PYARGS(METH_O, "(domain); change the domain in place");
    PyObject *py_append(PyObject *arg) PYARGS(METH_O, "(instance); add an example");
    PyObject *py_extend(PyObject *arg) PYARGS(METH_O, "(instances); add examples from a table or list");

    PyObject *py_removeWeights() PYARGS(METH_NOARGS, "(); remove all weights");
    PyObject *py_setWeights(PyObject *arg, PyObject *kw) PYARGS(METH_BOTH, "(weight); set weights for all examples");
    PyObject *py_total_weight() const PYARGS(METH_NOARGS, "() -> compute the total weight of examples in the table");

    PyObject *py_sample(PyObject *arg, PyObject *kw) PYARGS(METH_BOTH, "(folds[, select, copy]); select a subset of examples");
    PyObject *py_select(PyObject *arg, PyObject *kw) PYARGS(METH_BOTH, "(folds[, select]); select a subset of examples (obsolete, use sample)");
    PyObject *py_selectref(PyObject *arg, PyObject *kw) PYARGS(METH_BOTH, "(folds[, select]); select a subset of examples (obsolete, use sample)");
    PyObject *translate_w_flag(PyObject *args, PyObject *kw, int copy=-1); // helper for below and select
    PyObject *py_translate(PyObject *arg, PyObject *kw) PYARGS(METH_BOTH, "(variables[, keep_metas, copy]); return a table translated to another domain");

    PyObject *py_save(PyObject *args, PyObject *kw) const PYARGS(METH_BOTH, "(filename); save data to a file");

#ifndef NO_NUMPY
    void parseMatrixContents(const char *contents,
        const int &multiTreatment, bool &hasClass, bool &classVector,
        // bool &multiclassVector,
        bool &weightVector, bool &classIsDiscrete, int &columns,
        vector<bool> &include);

    char const *const numpyUnviewable() const;
    PyObject *asNumpy_view();
    PyObject *asNumpy_copy();
#endif

    PyObject *toNumpy(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "([contents, multinomial, masked]); construct a numpy array from examples");
    PyObject *asNumpy(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "([force_view, copy]); returns a numpy view of a table or a copy if view is impossible");

    /// @endcond
};

#endif

