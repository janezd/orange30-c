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
#include "exampletablereader.hpp"
#include "filter.hpp"

#include "exampletable.px"

/*! \class PExampleTable
\brief P-wraper for TExampleTable

TExampleTable does not use a generic wrapper like other classes. The wrapper
is defined manually to override argconverted and setter to accept also a numpy
object. The object is converted to a PExampleTable with a domain that is
compatible with any other domain with the same types of attributes.
*/

/*! 
Constructor; the same as for generic P-classes. It calls the
inherited constructor and raises an exception if the type does not match.
*/
PExampleTable::PExampleTable(POrange const &o)
: POrange(o)
{ 
    if (o && !dynamic_cast<TExampleTable *>(o.orange)) {
        raiseError(PyExc_SystemError, "invalid cast to 'ExampleTable'");
    }
}

/*! A converter for PyArg_ParseTuple (and related functions), which differs
from generic ones in that it also accepts a numpy array. It calls the
appropriate constructor for TExampleTable and wraps it.
*/
int PExampleTable::argconverter(PyObject *obj, PExampleTable *addr)
{
    if (!obj) {
        addr->clear();
    }
    else if (OrExampleTable_Check(obj)) {
        *addr = PExampleTable(obj);
    }
#ifndef NO_NUMPY
    else if (PyArray_Check(obj)) {
        try {
            *addr = PExampleTable(new TExampleTable(PDomain(), obj));
        }
        PyCATCH_r(0);
    }
#endif
    else {
        TExampleTable *o = (TExampleTable *)TExampleTable::setterConversion(obj);
        if (!o) {
            PyErr_Format(PyExc_TypeError,
                "cannot convert '%s' to 'ExampleTable'", obj->ob_type->tp_name);
            return 0;
        }  
        *addr = PExampleTable(o);
    }
    return Py_CLEANUP_SUPPORTED;
}
  
/*! A copy of the generic argconverter_n. */
int PExampleTable::argconverter_n(PyObject *obj, PExampleTable *addr)
{
    if (obj != Py_None) {
        return argconverter(obj, addr);
    }
    addr->clear();
    return Py_CLEANUP_SUPPORTED;
}

/*! Attribute setter, which differs from generic ones in that it also accepts a
numpy array. It calls the appropriate constructor for TExampleTable and wraps it.
*/
int PExampleTable::setter(PyObject *whom, PyObject *mine, size_t *offset)
{
    try {
        bool not_null = (*offset & 0x4000000) != 0;
        const size_t real_offset = *offset & 0x1fffffff;
        PExampleTable *fld = (PExampleTable *)((char *)(whom)+real_offset);
        if (!mine || (mine == Py_None)) {
            if (not_null) {
                PyErr_Format(PyExc_ValueError, "attribute must not be None");
                return -1;
            }
            fld->clear();
            return 0;
        }
        else if (OrExampleTable_Check(mine)) {
            *fld = PExampleTable(mine);
        }
#ifndef NO_NUMPY
        else if (PyArray_Check(mine)) {
            *fld = PExampleTable(new TExampleTable(PDomain(), mine));
        }
#endif
        else {
            TExampleTable *o = (TExampleTable *)TExampleTable::setterConversion(mine);
            if (!o) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(PyExc_TypeError,
                        "cannot convert '%s' to 'ExampleTable'",
                        mine->ob_type->tp_name);
                }
                return -1;
            }  
            *fld = PExampleTable(o);
        }
        return 0;
    }
    PyCATCH_r(-1); 
}



int TExampleTable::tableVersion = 0;

/*! Construct a new, empty table that owns its data.
    Equivalent to #constructEmpty with an additional parameter for the row size
    that is (and should) be used only at unpickling. If the row size is given,
    #metaOffset is set to -1 and should be initialized by the caller if needed.
    
    In all other cases, the size is computed from the domain.
*/
TExampleTable::TExampleTable(PDomain const &dom,
                             int const reserveRows,
                             int const arowSize)
: base(NULL),
  data(NULL),
  allocatedRows(0),
  rowSize(arowSize < 0 ? dom->variables->size()*sizeof(double) + sizeof(size_t) : rowSize),
  metaOffset(arowSize < 0 ? dom->variables->size()*sizeof(double) : -1),
  domain(dom),
  dataType('d'),
  referenceCount(0),
  isContiguous(true),
  version(++tableVersion)
{
    if (reserveRows) {
        reserve(reserveRows);
    }
}



/*! Construct an empty table for referencing the given table.
    Equivalent to #constructEmptyReference. */
TExampleTable::TExampleTable(PExampleTable const &abase)
: base(abase->base ? abase->base : abase.borrowPyObject()), // borrow now, incref below
  data(NULL),
  allocatedRows(abase->allocatedRows),
  rowSize(abase->rowSize),
  attributeOffsets(abase->attributeOffsets),
  metaOffset(abase->metaOffset),
  domain(abase->domain),
  dataType(abase->dataType),
  referenceCount(0),
  isContiguous(abase->isContiguous),
  version(++tableVersion)
{
    Py_INCREF(base);
    abase->referenceCount++;
}



/*! Construct a new table by converting an existing table.
    Equivalent to #constructConverted. */
TExampleTable::TExampleTable(PDomain const &dom,
                             PExampleTable const &orig,
                             bool const skipMetas)
: base(NULL),
  data(NULL),
  allocatedRows(orig->examples.size()),
  rowSize(dom->variables->size()*sizeof(double) + sizeof(size_t)),
  attributeOffsets(orig->attributeOffsets),
  metaOffset(dom->variables->size()*sizeof(double)),

  domain(dom),
  dataType('d'),
  referenceCount(0),
  isContiguous(true),
  version(++tableVersion)
{
    data = (char *)malloc(allocatedRows * rowSize);
    char *datai = data;
    examples.resize(orig->examples.size());
    ITERATE(vector<char *>, ci, examples) {
        *ci = datai;
        datai += rowSize;
    }
    for(iterator ei(begin()), oi(orig->begin()); oi; ++oi, ++ei) {
        MetaChain::killChain(ei.getMetaHandle());
        domain->convert(*ei, *oi, skipMetas);
    }
    weights = orig->weights;
}


#ifndef NO_NUMPY


bool loadNumPy()
{
    if (numpy_module) {
        return true;
    }
    numpy_module = PyImport_ImportModule("numpy");
    if (!numpy_module) {
        PyErr_Clear();
        return false;
    }
    import_array();
    PyObject *mdict = PyModule_GetDict(numpy_module);
    NDArray_TypePtr = PyDict_GetItemString(mdict, "ndarray");
    if (!NDArray_TypePtr) {
        Py_DECREF(numpy_module);
        numpy_module = NULL;
        return false;
    }
    ma_module = PyDict_GetItemString(mdict, "ma");
    if (!ma_module) {
        PyErr_Clear();
        Py_DECREF(numpy_module);
        numpy_module = NULL;
        NDArray_TypePtr = NULL;
        return false;
    }
    MaskedArray_TypePtr = PyDict_GetItemString(PyModule_GetDict(ma_module), "MaskedArray");
    if (!MaskedArray_TypePtr) {
        Py_DECREF(numpy_module);
        Py_DECREF(ma_module);
        numpy_module = ma_module = NULL;
        NDArray_TypePtr = NULL;
        return false;
    }
    Py_INCREF(NDArray_TypePtr);
    Py_INCREF(MaskedArray_TypePtr);
    return true;
}


PyObject *numpy_module = NULL;
PyObject *ma_module = NULL;
PyObject *NDArray_TypePtr = NULL;
PyObject *MaskedArray_TypePtr = NULL;
bool numpyLoaded = loadNumPy();

/*! Construct a table from numpy array. Equivalent to #constructFromNumpy. */
TExampleTable::TExampleTable(PDomain const &dom, PyObject *np, bool const copy)
: base((PyObject *)np),
  data(PyArray_BYTES(np)),
  allocatedRows(PyArray_DIM(np, 0)),
  rowSize(PyArray_STRIDE(np, 0)),
  referenceCount(1), // permanent lock
  isContiguous(true),
  domain(dom),
  dataType(((PyArrayObject *)np)->descr->type)
{
    if (copy) {
        raiseError(PyExc_NotImplementedError, "Copying is not supported yet");
    }
    if (PyArray_NDIM(np) != 2) {
        raiseError(PyExc_ValueError, "Array has to be two-dimensional");
    }
    // When implementing other types be careful that the size of the meta
    // column must be at least sizeof(int)! Don't allow metas otherwise!
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "ExampleTable currently works only with arrays of doubles");
    }

    int const columns = PyArray_DIM(np, 1);
    int dimdiff;
    if (domain) {
        dimdiff = columns - dom->variables->size();
        if (dimdiff < 0) {
            raiseError(PyExc_IndexError,
                "the number of variables in the domain exceeds the number of columns");
        }
        else if (dimdiff > 1) { // can exceed by one - the meta attribute
            raiseError(PyExc_IndexError,
                "the number of columns exceeds the number of variables in the domain");
        }
    }
    else {
        dimdiff = 0;
    }
    examples.resize(allocatedRows);
    char *example = PyArray_BYTES(np);
    metaOffset = dimdiff ? columns - sizeof(double) : -1;
    for(vector<char *>::iterator ei(examples.begin()), ee(examples.end());
        ei != ee; ei++, example += rowSize) {
            *ei = example;
            if (dimdiff) {
                *(int *)(*ei + metaOffset) = 0;
            }
    }
    attributeOffsets.resize(columns);
    size_t off = 0;
    vector<size_t>::iterator ai(attributeOffsets.begin()),
        ae(attributeOffsets.end());
    int const value_stride = PyArray_STRIDE(np, 1);
    for(; ai != ae; ai++, off += value_stride) {
        *ai = off;
    }
    if (!domain) {
        char varname[16];
        PVarList variables(new TVarList());
        for(int i = 0; i != columns; i++) {
            snprintf(varname, 15, "var%05i", i);
            char const *attrdata = data + attributeOffsets[i];
            char const *const attrdatae = attrdata + allocatedRows*rowSize;
            double const *&value = (double const *&)attrdata;
            int maxval = 0;
            for(; (attrdata != attrdatae) && 
                    (*value >= 0) && (*value < 10) && (int(*value) == *value)
                ; ((char const *&)attrdata) += rowSize) {
                if (*value > maxval) {
                    maxval = int(*value);
                }
            }
            TVariable *var;
            if (attrdata == attrdatae) {
                var = new TDiscreteVariable(varname);
                for(int v = 0; v <= maxval; v++) {
                    sprintf(varname, "v%i", v);
                    ((TDiscreteVariable *)var)->addValue(varname);
                }
            }
            else {
                var = new TContinuousVariable(varname);
            }
            variables->push_back(PVariable(var));
        }
        domain = PDomain(new TDomain(variables));
        domain->anonymous = true;
    }
    Py_INCREF(base);
}

#endif


/*! Construct a copy of an existing table.

    \param base The table to be copied
    \param skipMetas If true meta attributes that are not included
            in the new domain are not copied
    \param type The type of the constructed PyObject
*/
PExampleTable TExampleTable::constructCopy(PExampleTable const &base,
                                           bool const skipMetas,
                                           PyTypeObject *type)
{
    PExampleTable const me(new(type) TExampleTable(
        base->domain, base->examples.size()));
    me->examples.resize(me->allocatedRows);
    vector<char *>::const_iterator odi(base->examples.begin()),
        ode(base->examples.end());
    vector<char *>::iterator mex(me->examples.begin());
    int idx = 0;
    char *datai = me->data;
    // If there are no meta attributes in the base, we don't try to copy them
    bool const askipMetas = skipMetas || !base->supportsMeta();
    bool const memCopy = base->isContiguous && (base->dataType == 'd');
    for(; odi!=ode; odi++, datai += me->rowSize, mex++, idx++) {
        *mex = datai;
        if (memCopy) {
            memcpy(datai, *odi, me->rowSize);
        }
        else {
            base->copyDataToExample((double *)datai, *odi);
        }
        int &myMetaPtr = *(int *)(datai + me->metaOffset);
        myMetaPtr = 0;
        if (!askipMetas) {
            MetaChain::copyChain(myMetaPtr, base->getMetaHandle(idx));
        }
    }
    me->weights = base->weights;
    return me;
}


/*! Construct a table of references to the given table with a new domain;
    the new domain may contain only variables from the original domain.
    The constructor can also convert the examples form the original table
    or leave the table empty.

    \param base The table whose data is referenced
    \param domain New domain for the data
    \param copyData If \c true, the examples are copied to the new table
    \param type The type of the constructed PyObject
*/
PExampleTable TExampleTable::constructConvertedReference(
    PExampleTable const &base,
    PDomain const &domain,
    bool copyData,
    PyTypeObject *type)
{
    vector<int> attributes;
    const_PITERATE(TVarList, vi, domain->variables) {
        attributes.push_back(domain->getVarNum(*vi));
    }
    return constructByColumns(base, domain, attributes, copyData, type);
}


/*! Construct a table of references to the given table but retaining
    only the selected columns.

    \param base The table whose data is referenced
    \param domain New domain for the data
    \param attributes Indices of columns to be used
    \param copyData If true, the constructed table contains
    references to the data instances in the old table, otherwise
    it is empty 
    \param type The type of the constructed PyObject

    Features can be "reinterpreted" since the argument attributes gives the
    column and the argument domain provides the corresponding variable
    descriptions that do not necessarily match the original attributes.
        
    It is the caller's responsibility to ensure that reinterpretation
    makes sense.
*/
PExampleTable TExampleTable::constructByColumns(PExampleTable const &base,
                                                PDomain const &domain,
                                                vector<int> const &attributes,
                                                bool copyData,
                                                PyTypeObject *type)
{
    if (base->dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "extracting columns is currently supported only for tables of doubles");
    }
    if (domain->variables->size() != attributes.size()) {
        raiseError(PyExc_ValueError,
            "mismatching number of variables and their indices");
    }
    PExampleTable me(new(type) TExampleTable(base)); 
    me->domain = domain;
    me->metaOffset = base->metaOffset;
    me->isContiguous = false;
    if (base->attributeOffsets.size()) {
        const_ITERATE(vector<int>, ai, attributes) {
            if (*ai >= base->attributeOffsets.size()) {
                raiseError(PyExc_IndexError,
                    "attribute index %i out of range", *ai);
            }
            me->attributeOffsets.push_back(base->attributeOffsets[*ai]);
        }
    }
    else {
        const_ITERATE(vector<int>, ai, attributes) {
            if (*ai >= base->domain->variables->size()) {
                raiseError(PyExc_IndexError,
                    "attribute index %i out of range", *ai);
            }
            me->attributeOffsets.push_back(*ai * sizeof(double));
        }
    }
    if (copyData) {
        me->examples = base->examples;
        me->weights = base->weights;
    }
    return me;
}


/*! Deallocate the data or release the reference and decrease the reference count. */
TExampleTable::~TExampleTable()
{
    if (!base) {
        if (data) {
            if (supportsMeta()) {
                ITERATE(vector<char *>, ei, examples) {
                    MetaChain::freeChain(*(int *)(*ei + metaOffset));
                }
            }
            free(data);
        }
    }
    else {
        if (OrExampleTable_Check(base)) {
            ((OrExampleTable *)base)->orange.referenceCount--;
        }
        Py_DECREF(base);
    }
}


/*! Resize #data to accomodate i rows, if possible.

    \param i The new size of the table (in rows)

    If the table already contains enough space for i rows (or more) it is
    not resized.

    Resizing is not possible if the table does not own its data or if some other
    objects references this object's data.

    \see referenceCount allocatedRows
    \see testLocked()
    \see grow()
*/
void TExampleTable::reserve(int const i)
{ 
    if (i < allocatedRows) {
        return;
    }
    if (base) {
        raiseError(PyExc_SystemError,
            "cannot allocate memory for table that refers to another table");
    }
    testLocked();
    char *newdata = (char *)realloc(data, i*rowSize);
    if (!newdata && i && rowSize) {
        raiseError(PyExc_MemoryError, "out of memory");
    }
    const int datadiff = newdata - data;
    for(vector<char *>::iterator ei(examples.begin()), ee(examples.end());
        ei != ee; *ei++ += datadiff);
    for(vector<char *>::iterator ei(freeRows.begin()), ee(freeRows.end());
        ei != ee; *ei++ += datadiff);
    data = newdata;
    allocatedRows = i;
    reserveRefs(i);
}

/*! Reserve space in #examples; calls <tt>examples.reserve(i)</tt>

    \param i number of data instances
*/
void TExampleTable::reserveRefs(int const i)
{
    examples.reserve(i);
    if (hasWeights()) {
        weights.reserve(i);
    }
}


/*! Increases the allocated #data size by one half.

    Calls reserve to allocate 1.5 * #allocatedRows. If the number of currently
    allocated rows is below 256, 256 rows are allocated.
    
    \see resize
*/
void TExampleTable::grow()
{
    reserve(allocatedRows >= 256 ? allocatedRows + (allocatedRows>>2) : 256);
}

/*! Copy the data from a general format (as described by #dataType and
    #attributeOffsets) to array of doubles

    \param values pointer to example's values
    \param rowPtr pointer to data in the table

    The function provides the interface for handling different types of data
    (currently only 'd' is supported) and for non-contiguous data. It is called
    by TExampleIterator and by TExampleTable's constructors that copy the data.
    Several functions (e.g. indexing TExampleIterators) bypass this function
    for speed.

    The function does not copy the meta attribute index and weight.

    \see copyDataToTable(char *, TValue const *)
*/
void TExampleTable::copyDataToExample(TValue const *values,
                                      char const *const rowPtr) const
{
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    if (isContiguous) {
        memcpy(const_cast<TValue *>(values), rowPtr, rowSize);
    }
    else {
        const_ITERATE(vector<size_t>, ai, attributeOffsets) {
            const_cast<double &>(*values++) = *(double *)(rowPtr+*ai);
        }
    }
}


/*! Copies the data from array of doubles to a general format
    described by #dataType and #attributeOffsets.

    \param rowPtr pointer to data in the table
    \param values pointer to example's values

    The function provides the interface for handling different types
    of data (currently only 'd' is supported) and for non-contiguous
    data. It is called by TExampleIterator and by TExampleTable's
    constructors that copy the data.  Several functions (e.g. indexing
    TExampleIterators) bypass this function for speed.

    The function does not copy the meta attribute index and weight.

    \see copyDataToExample(TValue const *, char const *const)
*/
void TExampleTable::copyDataToTable(char *rowPtr, TValue const *values)
{
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    if (isContiguous) {
        memcpy(rowPtr, values, rowSize);
        if (supportsMeta()) {
            *(int *)(rowPtr + metaOffset) = 0;
        }
    }
    else {
        const_ITERATE(vector<size_t>, ai, attributeOffsets) {
            *(double *)(rowPtr+*ai) = *values++;
        }
    }
    version = ++tableVersion;
}


/*! Add a data instance to the table; works only for tables that own
    their data and are not referenced by other objects.

    The new instance is allocated by reusing one the free rows (rows
    that were earlier freed), it can be in one of the allocated rows
    past the last used row; if none of these is available, #data is
    resized ba calling #grow. The latter will fail if the table is
    referenced by another table or object. If this is expected, enough
    space should be allocated in advance by calling #reserve.

    \see push_back(TExample const *)
    \see reserve(int const)
    \see grow()
*/
int TExampleTable::new_example()
{
    if (base) {
        raiseError(PyExc_SystemError,
            "cannot add new examples to a table referencing another table");
    }
    const int preSize = examples.size();
    char *ptr;
    if (freeRows.size()) {
        ptr = freeRows.back();
        freeRows.pop_back();
    }
    else {
        if (allocatedRows == preSize) {
            grow();
        }
        ptr = data + rowSize*preSize;
    }
    if (supportsMeta()) {
        *(int *)(ptr + metaOffset) = 0;
    }
    examples.push_back(ptr);
    if (hasWeights()) {
        weights.push_back(1.0);
    }
    version = ++tableVersion;
    return preSize;
}


/*! Add an example to the table.

    \param example Example to be added to the table

    If the table references another table, the given example must be
    from that table.

    Otherwise, the example is converted to the table's #domain and
    added.  The method calls #new_example to get the space for the new
    example, which may fail if #data is full and the table is locked.

    Tables that reference another tables can always add new examples
    since they have now #data that would require reallocation.

    \see extend(PExampleTable const &, bool const)
    \see new_example()
*/
void TExampleTable::push_back(TExample const *example)
{
    testLocked();
    if (base) {
        TExampleTable *extable = example->table.borrowPtr();
        if (   (example->referenceType == TExample::Free) 
            || (extable->base != base) &&
               (example->table.borrowPyObject() != base)) {
            raiseError(PyExc_ValueError,
                "table that references another table cannot contain examples from a different table");
        }
        examples.push_back(example->referenced_row ?
            example->referenced_row : (char *)example->values);
        if (hasWeights()) {
            weights.push_back(example->getWeight());
        }
    }
    else {
        if (dataType != 'd') {
            raiseError(PyExc_NotImplementedError,
                "only arrays of doubles are currently supported");
        }
        int firstMeta = example->getMetaHandle();
        if (firstMeta) {
            checkSupportsMeta();
        }
        int newind = new_example();
        // shortcut for the most common case
        if (domain == example->domain) {
            copyDataToTable(examples[newind], example->values);
            if (firstMeta) {
                MetaChain::copyChain(getMetaHandle(newind), firstMeta);
            }
        }
        else {
            PExample dex = at(newind);
            domain->convert(dex.borrowPtr(), example);
        }
    }
    if (hasWeights()) {
        weights.push_back(example->getWeight());
    }
    version = ++tableVersion;
}


/*! Add examples from another table to this table.

    \param table Example that are added to the table
    \param skipMetas If true, metas that are not in the new domain are
    not copied. The argument is ignored for tables that reference
    another table

    If the table references another table, only examples from that
    table or from other tables referencing this same table can be
    added.

    Otherwise, examples are converted to the table's #domain and
    added.  The method calls #new_example to get the space for new
    examples, which may fail if #data is full and the table is locked.

    \see push_back(TExample const *)
    \see new_example()
*/
void TExampleTable::extend(PExampleTable const &table, bool const skipMetas)
{
    testLocked();
    if (base) {
        if ((table->base != base) && (table.borrowPyObject() != base)) {
            raiseError(PyExc_ValueError,
                "table that references another table cannot contain examples from a different table");
        }
        examples.insert(
            examples.end(), table->examples.begin(), table->examples.end());
        if (table->hasWeights()) {
            ensureWeights();
            weights.insert(
                weights.end(), table->weights.begin(), table->weights.end());
        }
        else if (hasWeights()) {
            weights.insert(weights.end(), table->size(), 1.0);
        }
    }
    else {
        if (dataType != 'd') {
            raiseError(PyExc_NotImplementedError,
                "Only arrays of doubles are currently supported");
        }
        // Don't provoke exceptions by calling getFirstMetaPtr without need.
        // But if we will need metas, verify that we accept them before we start
        // adding examples
        bool copyMetas = false;
        if (!skipMetas && table->supportsMeta()) {
            PEITERATE(ei, table) {
                if (ei.getMetaHandle()) {
                    checkSupportsMeta();
                    copyMetas = true;
                    break;
                }
            }
        }
        PEITERATE(ei, table) {
            int newind = new_example();
            // fast shortcut for the most common case
            if (domain == table->domain) {
                copyDataToTable(examples[newind], ei->values);
                if (copyMetas) {
                    MetaChain::copyChain(
                        getMetaHandle(newind), ei.getMetaHandle());
                }
            }
            else {
                PExample dex = at(newind);
                domain->convert(dex.borrowPtr(), *ei);
            }
        }
        if (table->hasWeights()) {
            ensureWeights();
            vector<double>::iterator wei = weights.end() - table->size();
            const_ITERATE(vector<double>, weio, table->weights) {
                *wei++ = *weio;
            }
        }
    }
    version = ++tableVersion;
}


/*! Remove the example for the table

    \param idx Index of the removed example

    If the table owns its data, the freed row is stored in #freeRows. The size
    of the #data is unchanged.

    Examples cannot be removed from tables that are referenced by other objects,
    such as tables or examples.

    \see erase(const int, const int)
    \see clear()
*/
void TExampleTable::erase(const int idx)
{
    testLocked();
    vector<char *>::iterator const eptr = examples.begin()+idx;
    if (base) {
        examples.erase(eptr);
    }
    else {
        if (supportsMeta()) {
            MetaChain::freeChain(getMetaHandle(*eptr));
        }
        freeRows.push_back(*eptr);
        examples.erase(eptr);
    }
    if (hasWeights()) {
        weights.erase(weights.begin() + idx);
    }
    version = ++tableVersion;
}


/*! Remove examples in the given range

    \param begidx The index of the first removed example
    \param endidx The index of the example after the last removed one

    If the table owns its data, the freed rows are stored in #freeRows. The size
    of the #data is unchanged.

    Examples cannot be removed from tables that are referenced by other objects,
    such as tables or examples.

    \see erase(const int)
    \see clear()
*/
void TExampleTable::erase(const int begidx, const int endidx)
{
    testLocked();
    vector<char *>::iterator const ebeg = examples.begin();
    vector<char *>::iterator const bi = ebeg+begidx, be = ebeg+endidx;
    if (!base) {
        if (supportsMeta()) {
            for(vector<char *>::iterator eptr = examples.begin() + begidx,
                    eptre = examples.begin() + endidx;
                    eptr != eptre;
                    eptr++) {
                MetaChain::freeChain(*(int *)(*eptr + metaOffset));
            }
        }
        freeRows.insert(freeRows.end(), bi, be);
    }
    examples.erase(bi, be);
    if (hasWeights()) {
        weights.erase(weights.begin()+begidx, weights.begin()+endidx);
    }
    version = ++tableVersion;
}


/*! Remove all examples from the table. If the table owns its data,
    #data is freed. This function fails if the table is referenced by
    other objects.

    \see erase(const int)
    \see erase(const int, const int)
*/
void TExampleTable::clear()
{
    testLocked();
    examples.clear();
    if (!base) {
        if (supportsMeta()) {
            ITERATE(vector<char *>, eptr, examples) {
                MetaChain::freeChain(*(int *)(*eptr + metaOffset));
            }
        }
        freeRows.clear();
        free(data);
        data = NULL;
    }
    weights.clear();
    version = ++tableVersion;
}


/*! Return the example with the given index.
    \param idx Index of the example
    \see at(int const, TAttrIdx const)
*/
PExample TExampleTable::at(int const idx)
{
    if (idx >= size()) {
        raiseError(PyExc_IndexError, "index %i out of range", idx);
    }
    return TExample::constructTableItem(PExampleTable::fromBorrowedPtr(this), idx);
}


/*! Return a reference to the value of the given attribute and example.
    \param idx Example index
    \param attribute Attribute index; this can also be -1 for the class or
    negative for meta attributes.

    Method works only for data tables represented with doubles
    (#dataType 'd').  If the index is negative, the meta attribute
    must exist and have a primitive value.

    \see at(int const)
*/
TValue &TExampleTable::at(int const idx, TAttrIdx const attribute)
{
    if (idx >= size()) {
        raiseError(PyExc_IndexError, "example index %i out of range", idx);
    }
    if (attribute >= 0) {
        if (dataType != 'd') {
            raiseError(PyExc_NotImplementedError,
                "only arrays of doubles are currently supported");
        }
        if (isContiguous) {
            if (attribute >= domain->variables->size()) {
                raiseError(PyExc_IndexError,
                    "attribute index %i out of range", attribute);
            }
            return ((double *)examples[idx])[attribute];
        }
        else {
            if (attribute >= attributeOffsets.size()) {
                raiseError(PyExc_IndexError,
                    "attribute index %i out of range", attribute);
            }
            return *((double *)(examples[idx]+attributeOffsets[attribute]));
        }
    }
    if (attribute == -1) {
        if (!domain->classVar) {
            raiseError(PyExc_IndexError, "data has no class variable");
        }
        if (isContiguous) {
            return ((double *)examples[idx])[domain->attributes->size()];
        }
        else {
            return *((double *)(examples[idx]+attributeOffsets.back()));
        }
    }
    else { // attribute < 0
        return MetaChain::getDouble(getMetaHandle(idx), attribute);
    }
}

/*! Return a CRC32 sum of the table's values
    \param includeMetas If true, the meta attributes are included in the sum.
*/
unsigned int TExampleTable::checkSum(const bool includeMetas) const
{ 
    unsigned int crc;
    INIT_CRC(crc);
    for(iterator ei(const_cast<TExampleTable *>(this)->begin()); ei; ++ei) {
        (*ei)->addToCRC(crc, includeMetas);
    }
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


/*! Return the total weight of examples in the table. */
double TExampleTable::totalWeight() const
{
    if (hasWeights()) {
        double weight = 0;
        const_ITERATE(vector<double>, wi, weights) {
            weight += *wi;
        }
        return weight;
    }
    else {
        return size();
    }
}


/*! Return true if the table has any missing values.
    Only ordinary values are checked. Meta attributes, even those included
    in the domain, are ignored.
    \see hasMissingClass()
*/
bool TExampleTable::hasMissing() const
{
    // fastest case
    if (isContiguous && (dataType == 'd')) {
        const int nAttrs = domain->variables->size();
        const_ITERATE(vector<char *>, ei, examples) {
            double const *di = (double const *)*ei, *de = di+nAttrs;
            for(; di != de; di++) {
                if (isnan(*di)) {
                    return true;
                }
            }
        }
        return false;
    }
    // less fastest case
    if (dataType == 'd') {
        const_ITERATE(vector<char *>, ei, examples) {
            const_ITERATE(vector<size_t>, ai, attributeOffsets) {
                if (isnan(*(double const *)(*ei+*ai))) {
                    return true;
                }
            }
        }
        return false;
    }
    // still fast case ;)
    PEITERATE(ei, PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this))) {
        TValue const *vi = ei->values, *ve = ei->values_end;
        for(; vi != ve; vi++) {
            if (isnan(*vi)) {
                return true;
            }
        }
    }
    return false;
}

/*! Return true if the table has any missing classes.
    Exception is raised if the domain has no class.
    \see hasMissing()
*/
bool TExampleTable::hasMissingClass() const
{
    if (!domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class variable", NULL);
    }
    // fast case
    if (dataType == 'd') {
        const size_t classofs = isContiguous ? 
            sizeof(double)*domain->attributes->size() : attributeOffsets.back();
        const_ITERATE(vector<char *>, ei, examples) {
            if (isnan(*(double const *)(*ei+classofs))) {
                return true;
            }
        }
        return false;
    }
    // still fast case ;)
    PEITERATE(ei, PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this))) {
        if (isnan(ei->values_end[-1])) {
            return true;
        }
    }
    return false;
}


/*! Filter the example table in place.  Exception is raised if the
    table owns its data, but cannot be modified since it is referenced
    by other object.

    \param filter Filter used for selecting the examples.
    \see select(PIntList const &, const int)
    \see selectref(PIntList const &, const int)
*/
void TExampleTable::filterInPlace(PFilter const &filter)
{
    testLocked();
    TFilter &rfilter = *filter;
    bool const clearMetas = !base && supportsMeta();
    int idx = 0;
    vector<double>::iterator const weib(weights.begin());
    vector<char *>::iterator exi(examples.begin());
    for(TExampleIterator ei(PExampleTable::fromBorrowedPtr(this));
            ei; ++ei, ++exi, idx++) {
        if (!rfilter(ei.operator *())) {
            examples.erase(exi);
            if (clearMetas) {
                MetaChain::freeChain(ei.getMetaHandle());
            }
            if (hasWeights()) {
                weights.erase(weib + idx);
            }
        }
    }
    version = ++tableVersion;
}


/*! Randomly shuffles the examples using the table's #randomGenerator
    A random generator is constructed if it does not exist yet.
 
   \see shuffle(PRandomGenerator const &)
*/
void TExampleTable::shuffle()
{
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    shuffle(randomGenerator);
}


/*! Randomly shuffles the examples using the given random generator
    \see shuffle()
*/
void TExampleTable::shuffle(PRandomGenerator const &rgen)
{
    testLocked();
    if (size() <= 1) {
        return;
    }
    TRandomGenerator &randomGenerator = *rgen;
    const int e = size();
    for(int i = 1; i < size(); i++) {
        const int j = randomGenerator.randint(i);
        swap(examples[i], examples[j]);
        if (hasWeights()) {
            swap(weights[i], weights[j]);
        }
    }
    version = ++tableVersion;
}


/*! Add a meta attribute with the given id and a primitive value to all examples

    \param id Id of the new meta attribute
    \param value The value assigned to the new attribute

    \see addMetaAttribute(TMetaId const, PyObject *)
    \see copyMetaAttribute(TMetaId const, TMetaId const, TValue const)
    \see removeMetaAttribute(TMetaId const id)
*/
void TExampleTable::addMetaAttribute(const TMetaId id, TValue const value)
{
    ITERATE(vector<char *>, ei, examples) {
        MetaChain::set(getMetaHandle(*ei), id, value);
    }
    version = ++tableVersion;
}


/*! Add a meta attribute with the given id and a non-primitive value to all
    examples.

    \param id Id of the new meta attribute
    \param obj The value assigned to the new attribute

    \see addMetaAttribute(const TMetaId, TValue const)
    \see copyMetaAttribute(TMetaId const, TMetaId const, TValue const)
    \see removeMetaAttribute(TMetaId const)
*/
void TExampleTable::addMetaAttribute(TMetaId const id, PyObject *obj)
{
    ITERATE(vector<char *>, ei, examples) {
        MetaChain::set(getMetaHandle(*ei), id, obj);
    }
    version = ++tableVersion;
}


/*! Remove the meta attribute with the given id. No exception is raised if
    the attribute does not exist.

    \param id Id of the removed attribute

    \see addMetaAttribute(const TMetaId, TValue const)
    \see addMetaAttribute(TMetaId const, PyObject *)
    \see copyMetaAttribute(TMetaId const, TMetaId const, TValue const)
*/
void TExampleTable::removeMetaAttribute(TMetaId const id)
{
    if (supportsMeta()) {
        ITERATE(vector<char *>, ei, examples) {
            MetaChain::remove(getMetaHandle(*ei), id);
        }
    }
    version = ++tableVersion;
}


/*! Copy the meta attribute's values into a new meta attribute.

    \param id Id of the new attribute
    \param source Id of the source attribute
    \param defaultVal Default value used if the source attribute does not exist

    \see addMetaAttribute(const TMetaId, TValue const)
    \see addMetaAttribute(TMetaId const, PyObject *)
    \see removeMetaAttribute(TMetaId const)
*/
void TExampleTable::copyMetaAttribute(TMetaId const id,
                                      TMetaId const source,
                                      TValue const defaultVal)
{
    ITERATE(vector<char *>, ei, examples) {
        int &metaHandle = getMetaHandle(*ei);
        TMetaValue val = MetaChain::get(metaHandle, source);
        if (val.id) {
            val.id = id;
            MetaChain::set(metaHandle, val);
        }
        else {
            MetaChain::set(metaHandle, id, defaultVal);
        }
    }
    version = ++tableVersion;
}


/*! Reorder examples in the table.

    \param indices Indices giving the new order

    The given indices must include each original example exactly once.
    The method does not check for this; missing indices will cause a
    memory leak and duplicated indices will crash Orange.

    \see sort()
    \see sort(vector<int> const &s)
*/
void TExampleTable::reorderExamples(vector<int> const &indices)
{
    vector<char *> newExamples(size());
    vector<char *>::iterator ni(newExamples.begin());
    const_ITERATE(vector<int>, ii, indices) {
        *ni++ = examples[*ii];
    }
    examples = newExamples;
    newExamples.clear();

    if (hasWeights()) {
        vector<double> newWeights(size());
        vector<double>::iterator wi(newWeights.begin());
        const_ITERATE(vector<int>, ii, indices) {
            *wi++ = weights[*ii];
        }
        weights = newWeights;
    }
}


template<class T>
class TCompareContiguousRows {
public:
    const TExampleTable *table;
    const int nAttrs;

    inline TCompareContiguousRows(const TExampleTable *t)
    : table(t),
      nAttrs(t->domain->variables->size())
    {}

    inline bool operator()(const int &row1, const int &row2) const
    {
        T const *rowPtr1 = (T *)table->examples[row1];
        T const *rowPtr2 = (T *)table->examples[row2];
        for(int i = nAttrs; i--; rowPtr1++, rowPtr2++) {
            if (*rowPtr1 != *rowPtr2) {
                return *rowPtr1 < *rowPtr2;
            }
        }
        return false;
    }
};
    

template<class T>
class TCompareRows {
public:
    const TExampleTable *table;
    const vector<size_t> *offsets;

    inline TCompareRows(const TExampleTable *t, const vector<size_t> *offs)
    : table(t),
      offsets(offs)
    {}

    inline bool operator()(const int &row1, const int &row2) const
    {
        char const *rowPtr1 = table->examples[row1];
        char const *rowPtr2 = table->examples[row2];
        const_PITERATE(vector<size_t>, ai, offsets) {
            const T &v1 = *(T *)(rowPtr1+*ai);
            const T &v2 = *(T *)(rowPtr2+*ai);
            if (v1 != v2) {
                return v1 < v2;
            }
        }
        return false;
    }
};


/*! Sort the table lexycographically by attributes' values Exception
    is raised if the table is referenced by another object, such as
    table or example.

    \see sort(vector<int> const &)
*/
void TExampleTable::sort()
{
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    testLocked();
    vector<int> sortIndices(size());
    vector<int>::iterator sii(sortIndices.begin());
    for(int i = 0; i < size(); *sii++ = i++);
    if (isContiguous) {
        ::stable_sort(sortIndices.begin(), sortIndices.end(),
            TCompareContiguousRows<double>(this));
    }
    else {
        ::stable_sort(sortIndices.begin(), sortIndices.end(),
            TCompareRows<double>(this, &attributeOffsets));
    }
    reorderExamples(sortIndices);
    version = ++tableVersion;
}


/*! Sort the table lexycographically by attributes' values using the
    given order of keys. Exception is raised if the table is
    referenced by another object, such as table or example.
    
    \param sortOrder The order of keys
    
    \see sort()
*/
void TExampleTable::sort(vector<int> const &sortOrder)
{
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    testLocked();
    vector<size_t> offsets;
    if (isContiguous) {
        const_ITERATE(vector<int>, soi, sortOrder) {
            offsets.push_back(*soi*sizeof(double));
        }
    }
    else {
        const_ITERATE(vector<int>, soi, sortOrder) {
            offsets.push_back(attributeOffsets[*soi]);
        }
    }
    vector<int> sortIndices(size());
    vector<int>::iterator sii(sortIndices.begin());
    for(int i = 0; i < size(); *sii++ = i++);
    ::stable_sort(sortIndices.begin(), sortIndices.end(),
        TCompareRows<double>(this, &offsets));
    reorderExamples(sortIndices);
    version = ++tableVersion;
}


bool sameExamples(char *rowPtr1, char *rowPtr2, const size_t dataSize)
{
    return !memcmp(rowPtr1, rowPtr2, dataSize);
}

template<class T>
bool sameExamples(char *rowPtr1, char *rowPtr2, const vector<size_t> &offsets)
{
    const_ITERATE(vector<size_t>, ai, offsets) {
        const T &v1 = *(T *)(rowPtr1+*ai);
        const T &v2 = *(T *)(rowPtr2+*ai);
        if (v1 != v2) {
            return false;
        }
    }
    return true;
}


/*! Remove duplicated examples from the table by merging them into single
    examples.

    \param computeWeights Tells whether to compute weights of merged examples

    This method first sorts the examples, then merges the same
    examples.  Comparison is made only using the ordinary attributes
    and the class; meta attributes are ignored and conflicting values
    are overwritten in random order.

    The resulting example's weight is the sum of the merged
    examples. */
void TExampleTable::removeDuplicates(bool computeWeights)
{
    if (dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    testLocked();
    vector<int> sortIndices(size());
    vector<int>::iterator sii(sortIndices.begin());
    for(int i = 0; i < size(); *sii++ = i++);
    if (isContiguous) {
        ::stable_sort(sortIndices.begin(), sortIndices.end(),
            TCompareContiguousRows<double>(this));
    }
    else {
        ::stable_sort(sortIndices.begin(), sortIndices.end(),
            TCompareRows<double>(this, &attributeOffsets));
    }

    if (computeWeights) {
        ensureWeights();
    }
    bool removed = false;
    size_t const dataSize = domain->variables->size() * sizeof(double);
    bool const fastComparison = isContiguous && dataType == 'd';
    bool const removeMetas = !base && supportsMeta();
    vector<int>::const_iterator currIdx = sortIndices.begin(), nextIdx = currIdx;
    vector<int>::const_iterator const endIdx(sortIndices.end());
    for(; currIdx != endIdx; currIdx = nextIdx) {
        char *currEx = examples[*currIdx];
        int *srcMetaHandle = removeMetas ? &getMetaHandle(*currIdx) : NULL;
        while((++nextIdx != endIdx) && (fastComparison ?
                sameExamples(
                  currEx, examples[*nextIdx], dataSize) 
              : sameExamples<double>(
                  currEx, examples[*nextIdx], attributeOffsets))) {
            if (srcMetaHandle) {
                int &dstHandle = getMetaHandle(*nextIdx);
                MetaChain::copyChain(*srcMetaHandle, dstHandle);
                MetaChain::freeChain(dstHandle);
            }
            if (hasWeights()) {
                weights[*currIdx] += weights[*nextIdx];
            }
            examples[*nextIdx] = NULL;
            removed = true;
        }
    }
    if (!removed) {
        return;
    }

    int srcIdx = -1, dstIdx;
    int const tsize = size();
    for(;examples[++srcIdx];);
    for(dstIdx = srcIdx - 1; srcIdx != tsize; srcIdx++) {
        if (examples[srcIdx]) {
            examples[++dstIdx] = examples[srcIdx];
            if (hasWeights()) {
                weights[dstIdx] = weights[srcIdx];
            }
        }
    }
    examples.erase(examples.begin()+dstIdx+1, examples.end());
    if (hasWeights()) {
        weights.erase(weights.begin()+dstIdx+1, weights.end());
    }
    version = ++tableVersion;
}


/*! Count the number of data instances in the given fold.

    \param folds Fold indices
    \param fold Fold index

    The helper method for #select and #selectref that checks that the
    number of fold indices does not exceed dthe number of examples
    (exception is raised if it does), and returns the number of
    example in the given fold. If fold index is negative, all examples
    in non-zero folds are counted.

    \see select(PIntList const &, const int)
    \see selectref(PIntList const &, const int)
*/
int TExampleTable::countInFold(PIntList const &folds, int const fold) const
{
    if (folds->size() > size()) {
        raiseError(PyExc_IndexError,
            "length of list of folds exceed the number of data instances (%i > %i)",
            folds->size(), size());
    }
    int count = 0;
    if (fold >= 0) {
        const_PITERATE(TIntList, li, folds) {
            if (*li == fold) {
                count++;
            }
        }
    }
    else {
        const_PITERATE(TIntList, li, folds) {
            if (*li) {
                count++;
            }
        }
    }
    return count;
}


/*! Return a table with examples belonging to the given fold.

    \param folds Fold indices
    \param fold Chosen fold

    The new table contains copy of examples. If fold index is
    negative, the new table contains examples belonging to the
    non-zero fold.

    \see selectref(PIntList const &, int const)
*/
PExampleTable TExampleTable::select(PIntList const &folds, const int fold) const
{
    PExampleTable wtable = constructEmpty(domain, countInFold(folds, fold));
    TExampleTable *table = wtable.borrowPtr();
    TExampleIterator ei(const_cast<TExampleTable *>(this)->begin());
    vector<double>::const_iterator wei = weights.begin();
    vector<double>::const_iterator const wee = weights.end();
    TIntList::const_iterator li(folds->begin());
    for(TIntList::const_iterator const le(folds->end()); li != le; li++, ++ei) {
        if (fold >= 0 ? *li == fold : *li) {
            table->push_back(*ei);
            if (wei != wee) {
                table->weights.push_back(*wei);
            }
        }
        if (wei != wee) {
            wei++;
        }
    }
    return wtable;
}


/*! Return a table with references to examples belonging to the given fold.

    \param folds Fold indices
    \param fold Chosen fold

    If fold index is negative, the new table contains examples belonging to the
    non-zero fold.

    \see select(PIntList const &, int const)   
*/
PExampleTable TExampleTable::selectref(PIntList const &folds, int const fold) const
{
    if (folds->size() > size()) {
        raiseError(PyExc_IndexError,
            "The list of folds is larger than the table");
    }
    PExampleTable wtable = constructEmptyReference(
            PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this)));
    TExampleTable *table = wtable.borrowPtr();
    table->reserve(countInFold(folds, fold));
    vector<char *>::const_iterator exi(examples.begin());
    vector<double>::const_iterator wei(weights.begin());
    vector<double>::const_iterator const wee(weights.end());
    TIntList::const_iterator li(folds->begin()), le(folds->end());
    for(; li != le; li++, exi++) {
        if (fold >= 0 ? *li == fold : *li) {
            table->examples.push_back(*exi);
            if (wei != wee) {
                table->weights.push_back(*wei);
            }
        }
        if (wei != wee) {
            wei++;
        }
    }
    return wtable;
}


/*! Construct a table from the given PyObject.
    This function is similar to \c newer, but does not assign keyword arguments
    to attributes.

    \param domain Domain of the new table
    \param obj An existing example table or an iterable object whose elements
    can be converted to examples of the given domain
    \param checkDomain If true and the given object is already an example table,
    its domain is checked
    \param type The type of the new object; ignored if 'obj' is already an ExampleTable

*/
PExampleTable TExampleTable::fromDomainAndPyObject(PDomain const &domain,
                                                   PyObject *obj,
                                                   bool const checkDomain,
                                                   PyTypeObject *type)
{
    if (!obj) {
        return PExampleTable(new(type) TExampleTable(domain));
    }
    if (OrExampleTable_Check(obj)) {
        PExampleTable table(obj);
        if (checkDomain && (table->domain != domain)) {
            raiseError(PyExc_ValueError,
                "example table belongs to a different domain");
        }
        return table;
    }
    PyObject *iter = PyObject_GetIter(obj);
    if (!iter) {
        raiseError(PyExc_TypeError,
            "cannot convert an instance of '%s' to ExampleTable",
            obj->ob_type->tp_name);
        return PExampleTable();
    }
    PyObject *item ;
    GUARD(iter);
    PExampleTable table(new(type) TExampleTable(domain));
    PExample ex;
    for(item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
        PyObject *it2 = item;
        GUARD(it2);
        ex = TExample::fromDomainAndPyObject(domain, item);
        table->push_back(ex);
    }
    return table;
}


#define PUTDELIM { if (ho) putc(delim, file); else ho = true; }

char to_escape[] = "\n\\n\x00\r\\r\x00\t\\t\x00";
char const *escape_ctrl(string &s)
{
    size_t pos;
    for(pos = 0; (pos = s.find("\\", pos)) != string::npos; pos+=2) {
        s.insert(pos, "\\");
    }
    for(char const *c = to_escape; *c; c+=4) {
        for(pos = 0; (pos = s.find(*c, pos)) != string::npos; ) {
            s.replace(pos, 1, c+1);
        }
    }
    return s.c_str();
}

char const *escape_space_ctrl(string &s)
{ 
    for(size_t pos = 0; (pos = s.find(" ", pos)) != string::npos; pos+=2) {
        s.insert(pos, "\\ ");
    }
    return escape_ctrl(s);
}


void TExampleTable::toFile_basketValue(FILE *file,
    PVariable const &var, TMetaValue &val) const
{
    // need to copy - don't want to escape the original!
    string vname =  var->getName();
    if ((var->varType == TVariable::Continuous) && 
        (val.value == 1.0)) {
        fprintf(file, escape_space_ctrl(vname));
    }
    else {
        string s =  var->isPrimitive() ?
            var->val2filestr(val.value) :
            var->pyval2filestr(val.object);
        fprintf(file, "%s=%s", escape_space_ctrl(vname), escape_space_ctrl(s));
    }
}


void TExampleTable::toFile_examples(FILE *file,
    char const delim, char const *const undefined) const
{ 
    TVarList::const_iterator const vb(domain->variables->begin());
    TVarList::const_iterator const ve(domain->variables->end());
    TVarList::const_iterator vi;
    TMetaVector::const_iterator const mb(domain->metas.begin());
    TMetaVector::const_iterator const me(domain->metas.end());
    TMetaVector::const_iterator mi;
    for(iterator ei(const_cast<TExampleTable *>(this)->begin()); ei; ++ei) {
        TExample::const_iterator ri((*ei)->begin());
        bool ho = false;
        for(vi = vb; vi!=ve; vi++, ri++) {
            PUTDELIM;
            fprintf(file, isnan(*ri) ? undefined
                : escape_ctrl((*vi)->val2filestr(*ri)));
        }
        for(mi = mb; mi != me; mi++) {
            if (!mi->optional) {
                PUTDELIM;
                TMetaValue val = (*ei)->getMetaIfExists((*mi).id);
                if (!val.id) {
                    fprintf(file, undefined);
                }
                else {
                    fprintf(file, escape_ctrl(
                        (*vi)->isPrimitive() ? (*vi)->val2filestr(val.value)
                                             : (*vi)->pyval2filestr(val.object)));
                }
            }
        }
        bool first = true;
        for(mi = mb; mi != me; mi++) {
            if (mi->optional) {
                TMetaValue val = (*ei)->getMetaIfExists(mi->id);
                if ((val.id != 0) && val.isDefined()) {
                    if (first) {
                        PUTDELIM;
                        first = false;
                    }
                    else {
                        fprintf(file, " ");
                    }
                    toFile_basketValue(file, mi->variable, val);
                }
            }
        }
        fprintf(file, "\n");
    }
}


void TExampleTable::toFile_examplesBasket(FILE *file) const
{ 
    char const delim = ',';
    TVarList::const_iterator const vb(domain->variables->begin());
    TVarList::const_iterator const ve(domain->variables->end());
    TVarList::const_iterator vi;
    TMetaVector::const_iterator const mb(domain->metas.begin());
    TMetaVector::const_iterator const me(domain->metas.end());
    TMetaVector::const_iterator mi;
    for(iterator ei(const_cast<TExampleTable *>(this)->begin()); ei; ++ei) {
        TExample::const_iterator ri((*ei)->begin());
        bool ho = false;
        for(vi = vb; vi!=ve; vi++, ri++) {
            if (isnan(*ri)) {
                continue;
            }
            PUTDELIM;
            string vname =  (*vi)->getName();
            if (((*vi)->varType == TVariable::Continuous) && (*ri == 1.0)) {
                fprintf(file, escape_space_ctrl(vname));
            }
            else {
                string s =  (*vi)->val2filestr(*ri);
                fprintf(file, "%s=%s",
                    escape_space_ctrl(vname), escape_space_ctrl(s));
            }
        }
        for(mi = mb; mi != me; mi++) {
            TMetaValue val = (*ei)->getMetaIfExists(mi->id);
            if ((val.id != 0) && val.isDefined()) {
                PUTDELIM;
                toFile_basketValue(file, mi->variable, val);
            }
        }
        fprintf(file, "\n");
    }
}


string TExampleTable::toString_varType(
    PVariable const &var, bool const listDiscreteValues)
{
    switch (var->varType) {
        case TVariable::Continuous:
            return "continuous";
        case TVariable::Discrete: {
            TDiscreteVariable *evar =
                dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
            string res;
            if (evar->values->size() && listDiscreteValues) {
                PITERATE(TStringList, vi, evar->values) {
                    if (res.size()) {
                        res += " ";
                    }
                    res += escape_space_ctrl(*vi);
                }
                return res;
            }
            else {
                return "discrete";
            }
        }
        case TVariable::String:
            return "string";
        default:
            raiseError(PyExc_TypeError, "selected file format supports only "
                "discrete, continuous and string variables");
    }
    return string();
}


void TExampleTable::toString_attributes(string &res, PVariable const &var)
{
    PyObject *attrdict = var->orange_dict ? 
        PyDict_GetItemString(var->orange_dict, "attributes") : NULL;
    if ((var->varType == TVariable::Discrete) && var->ordered) {
        if (res.size()) {
            res += " ";
        }
        res += "ordered";
    }
    if (attrdict) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(attrdict, &pos, &key, &value)) {
            if (PyUnicode_Check(key)) {
                Py_INCREF(key);
            }
            else {
                key = PyObject_Repr(key);
            }
            if (PyUnicode_Check(value)) {
                Py_INCREF(value);
            }
            else {
                value = PyObject_Repr(value);
            }
            if (res.size()) {
                res += " ";
            }
            res += PyUnicode_As_string(key) + "=" + PyUnicode_As_string(value);
            Py_DECREF(value);
            Py_DECREF(key);
        }
    }
}


void TExampleTable::toFile_head3(FILE *file,
    char const delim, bool const listDiscreteValues) const
{ 
    TVarList::const_iterator const vb(domain->variables->begin());
    TVarList::const_iterator const ve(domain->variables->end());
    TVarList::const_iterator vi;
    TMetaVector::const_iterator const mb(domain->metas.begin());
    TMetaVector::const_iterator const me(domain->metas.end());
    TMetaVector::const_iterator mi;
    bool ho = false;
    bool hasOptionalFloats = false;

    string first, second, third;
    for(vi = vb; vi!=ve; vi++) {
        if (first.size()) {
            first += delim; second += delim; third += delim;
        }
        string s = (*vi)->getName();
        first += escape_ctrl(s);
        second += toString_varType(*vi, listDiscreteValues);
        string attrs;
        if (*vi == domain->classVar) {
            attrs += "class";
        }
        toString_attributes(attrs, *vi);
        third += attrs;
    }
    for(mi = mb; mi!=me; mi++) {
        if (mi->optional) {
            if ((*mi).variable->varType == TVariable::Continuous) {
                hasOptionalFloats = true;
            }
            continue;
        }
        if (first.size()) {
            first += delim; second += delim; third += delim;
        }
        PVariable const &var = mi->variable;
        string s = var->getName();
        first += escape_ctrl(s);
        second += toString_varType(var, listDiscreteValues);
        string attrs("meta");
        if ((var->varType == TVariable::Discrete) && var->ordered) {
            attrs += " ordered";
        }
        toString_attributes(attrs, var);
        third += attrs;
    }
    if (hasOptionalFloats) {
        if (first.size()) {
            first += delim; second += delim; third += delim;
        }
        first += "optional_meta";
        second += "basket";
  }
  fprintf(file, (first + "\n" + second + "\n" + third + "\n").c_str());
}


/* If discrete value can be mistakenly read as continuous, we need to add the prefix. */

bool TExampleTable::toFile_checkNeedsD(PVariable const &var)
{
    TDiscreteVariable *enumv = dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
    if (!enumv) {
        return false;
    }
    if (!enumv->values->size()) {
        return true;
    }
    bool floated = false;
    PITERATE(TStringList, vi, enumv->values) {
        if (vi->size() > 63) {
            return false;
        }
        if ((vi->size()==1) && ((*vi)[0]>='0') && ((*vi)[0]<='9')) {
            continue;
        }
        string val(*vi);
        size_t comma = val.find(',');
        if (comma != string::npos) {
            val[comma] = '.';
        }
        char *eptr;
        strtod(val.c_str(), &eptr);
        if (*eptr) {
            return false;
        }
        else {
            floated = true;
        }
    }
    // All values were either one digit or successfully interpreted as continuous
    // We need to return true if there were some that were not one-digit...
    return floated;
}


void TExampleTable::toFile_head1(FILE *file, char const delim) const
{
    bool ho = false;
    const_PITERATE(TVarList, vi, domain->attributes) {
        PUTDELIM;
        string vname((*vi)->getName());
        fprintf(file, "%s%s",
            toFile_checkNeedsD(*vi) ? "D#" : "", escape_ctrl(vname));
    }
    if (domain->classVar) {
        PUTDELIM;
        string vname(domain->classVar->getName());
        fprintf(file, "%s%s",
            toFile_checkNeedsD(domain->classVar) ? "cD#" : "c#"),
            escape_ctrl(vname);
    }
    bool hasOptionalFloats = false;
    const_ITERATE(TMetaVector, mi, domain->metas) {
        if (mi->optional) {
            if (mi->variable->varType == TVariable::Continuous) {
                hasOptionalFloats = true;
            }
        }
        else {
            PUTDELIM;
            string vname(mi->variable->getName());
            fprintf(file, "%s%s",
                toFile_checkNeedsD(mi->variable) ? "mD#" : "m#"),
                escape_ctrl(vname);
        }
    }
    if (hasOptionalFloats) {
        PUTDELIM;
        fprintf(file, "B#optional_meta");
    }
    fprintf(file, "\n");
}

void TExampleTable::saveTab(char const *filename) const
{
    FILE *f = fopen(filename, "wt");
    try {
        toFile_head3(f, '\t', false);
        toFile_examples(f, '\t', "?");
        fclose(f);
    }
    catch (...) {
        fclose(f);
        remove(filename);
        throw;
    }
    fclose(f);
}


void TExampleTable::saveTxt(char const *filename) const
{
    FILE *f = fopen(filename, "wt");
    try {
        toFile_head1(f, '\t');
        toFile_examples(f, '\t', "");
        fclose(f);
    }
    catch (...) {
        fclose(f);
        remove(filename);
        throw;
    }
    fclose(f);
}

void TExampleTable::saveCsv(char const *filename) const
{
    FILE *f = fopen(filename, "wt");
    try {
        toFile_head1(f, ',');
        toFile_examples(f, ',', "");
        fclose(f);
    }
    catch (...) {
        fclose(f);
        remove(filename);
        throw;
    }
    fclose(f);
}

void TExampleTable::saveBasket(char const *filename) const
{
    FILE *f = fopen(filename, "wt");
    try {
        toFile_examplesBasket(f);
        fclose(f);
    }
    catch (...) {
        fclose(f);
        remove(filename);
        throw;
    }
    fclose(f);
}

/// @cond Python

static char *ExampleTable_keywords_domain_examples_filtermetas[] =
    {"domain", "data", "filter_metas", NULL};

static char *ExampleTable_keywords_copy[] =
    {"data", "copy_metas", NULL};

PyObject *TExampleTable::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    try {
        if (args) {
            if (PyTuple_GET_SIZE(args) == 9) {
                return unpickle_w_data(type, args);
            }
            if (PyTuple_GET_SIZE(args) == 4) {
                return unpickle_reference(type, args);
            }
        }
        if (   args && PyTuple_GET_SIZE(args) &&
                  PyUnicode_Check(PyTuple_GET_ITEM(args, 0))
            || kw && PyDict_GetItemString(kw, "filename")) {
                PExampleTableReader reader((TExampleTableReader *)
                    TExampleTableReader::__new__(type, args, kw));
                if (!reader)
                    return NULL;
                if (!reader->findFile()) {
                    return PyErr_Format(PyExc_IOError,
                        "File '%s' not found", reader->filename.c_str());
                }
                return reader->read(false);
        }
        PyErr_Clear();
        {
            PExampleTable other;
            int copyMetas = 1;
            if (PyArg_ParseTupleAndKeywords(args, kw, "O&|i:ExampleTable", 
                ExampleTable_keywords_copy,
                &PExampleTable::argconverter, &other, &copyMetas)) {
                    return constructCopy(other, copyMetas==0, type).getPyObject();
            }
        }
        PyErr_Clear();

        {
            PDomain domain;
            PyObject *pyexamples = NULL;
            int filterMetas = 0;
            if (PyArg_ParseTupleAndKeywords(args, kw, "O&|Oi:ExampleTable",
                ExampleTable_keywords_domain_examples_filtermetas,
                &PDomain::argconverter, &domain, &pyexamples, &filterMetas)) {
                    if (!pyexamples) {
                        return PyObject_FromNewOrange(
                            new(type) TExampleTable(domain));
                    }
#ifndef NO_NUMPY
                    if (numpyLoaded &&
                        PyObject_IsInstance(pyexamples, NDArray_TypePtr)) {
                        return PyObject_FromNewOrange(
                            new(type) TExampleTable(domain, pyexamples));
                    }
#endif
                    if (OrExampleTable_Check(pyexamples)) {
                        return PyObject_FromNewOrange(new(type) TExampleTable(
                            domain, PExampleTable(pyexamples), filterMetas!=0));
                    }
                    PExampleTable table = fromDomainAndPyObject(
                        domain, pyexamples, false, type);
                    if (table) {
                        return table.getPyObject();
                    }
            }
            PyErr_Clear();
        }
        return PyErr_Format(PyExc_TypeError,
            "invalid arguments for ExampleTable's constructor");
    }
    PyCATCH
}

PyObject *TExampleTable::__getnewargs__() const
{
    if (base) {
        if (OrExampleTable_Check(base)) {
            PExampleTable him(base);
            char *data = him->data;
            TByteStream buf;
            buf.write((size_t)(examples.size()));
            const_ITERATE(vector<char *>, ei, examples) {
                buf.write((int)(*ei - data));
            }
            buf.write(weights);
            buf.write(attributeOffsets);
            return Py_BuildValue("(NO y# i)",
                domain.getPyObject(), base,
                buf.buf, buf.length(),
                isContiguous);
        }
        return PyErr_Format(PyExc_TypeError,
            "Cannot pickle tables that reference instances of '%s'",
            base->ob_type->tp_name);
    }
    else {
        TByteStream buf;
        buf.write((size_t)(examples.size()));
        const_ITERATE(vector<char *>, ei, examples) {
            buf.write((int)(*ei - data));
        }
        buf.write(weights);
        buf.write(attributeOffsets);
        buf.write((size_t)(freeRows.size()));
        const_ITERATE(vector<char *>, fi, freeRows) {
            buf.write((int)(*fi - data));
        }
        PyObject *metas;
        if (supportsMeta()) {
            metas = PyList_New(0);
            const_ITERATE(vector<char *>, ei, examples) {
                int firstMeta = *(int *)(*ei + metaOffset);
                if (firstMeta) {
                    PyObject *packed = MetaChain::packChain(firstMeta, domain);
                    PyList_Append(metas, packed);
                    Py_DECREF(packed);
                }            
            }
        }
        else {
            metas = Py_None;
        }
        GUARD(metas);
        return Py_BuildValue("(N y# ii iO y# cb)",
            domain.getPyObject(),
            data, rowSize*allocatedRows,
            allocatedRows, rowSize,
            metaOffset, metas,
            buf.buf, buf.length(),
            dataType, isContiguous);
    }
}

PyObject *TExampleTable::unpickle_w_data(PyTypeObject *type, PyObject *args)
{
    PDomain dom;
    TByteStream buf;
    char *data;
    int datasize;
    int allocatedRows, rowSize;
    char dataType;
    int metaOffset;
    PyObject *metas;
    bool isContiguous;
    if (!PyArg_ParseTuple(args, "O&y#iiiOO&cb:ExampleTable.unpickle",
        &PDomain::argconverter, &dom,
        &data, &datasize,
        &allocatedRows, &rowSize,
        &metaOffset, &metas,
        &TByteStream::argconverter, &buf,
        &dataType, &isContiguous)) {
        return NULL;
	}
    PExampleTable me(new(type) TExampleTable(dom, 0, rowSize));
    me->data = (char *)malloc(datasize);
    if (!me->data) {
        raiseError(PyExc_MemoryError, "out of memory");
    }
    memcpy(me->data, data, datasize);
    data = me->data;
    me->allocatedRows = allocatedRows;
    me->metaOffset = metaOffset;
    me->dataType = dataType;
    me->isContiguous = isContiguous;

    me->examples.resize(buf.readSizeT());
    ITERATE(vector<char *>, ei, me->examples) {
        *ei = data + buf.readInt();
    }
    buf.readVector(me->weights);
    buf.readVector(me->attributeOffsets);
    me->freeRows.resize(buf.readSizeT());
    ITERATE(vector<char *>, fi, me->freeRows) {
        *fi = data + buf.readInt();
    }
    
    if (metas != Py_None) {
        int i = 0;
        ITERATE(vector<char *>, ei, me->examples) {
            int &firstMeta = *(int *)(*ei + metaOffset);
            /* if the packed id is non-zero, example had meta attributes */
            if (firstMeta) {
                PyObject *packed = PyList_GetItem(metas, i++);
                MetaChain::unpackChain(packed, firstMeta, me->domain);
            }            
        }
    }
    else if (metaOffset) {
        // this should not happen but let's play it safe
        ITERATE(vector<char *>, ei, me->examples) {
            MetaChain::killChain(*(int *)(*ei + metaOffset));
        }
    }
    return me.getPyObject();
}


PyObject *TExampleTable::unpickle_reference(PyTypeObject *type, PyObject *args)
{
    PDomain dom;
    TByteStream buf;
    bool isContiguous;
    PyObject *base;
    if (!PyArg_ParseTuple(args, "O&OO&b:ExampleTable.unpickle",
        &PDomain::argconverter, &dom, &base,
        &TByteStream::argconverter, &buf,
        &isContiguous)) {
        return NULL;
	}
    if (OrExampleTable_Check(base)) {
        PExampleTable him(base);
        PExampleTable me = TExampleTable::constructEmptyReference(him, type);
        me->isContiguous = isContiguous;

        vector<char *>::iterator ei;
        vector<char *>::const_iterator eei;
        me->examples.resize(buf.readSizeT());
        char *const data = him->data;
        ITERATE(vector<char *>, ei, me->examples) {
            *ei = data + buf.readInt();
        }
        buf.readVector(me->weights);
        buf.readVector(me->attributeOffsets);
        return me.getPyObject();
    }
    return PyErr_Format(PyExc_TypeError,
        "Cannot unpickle tables that reference instances of '%s'",
        base->ob_type->tp_name);
}


PyObject *TExampleTable::__repr__()
{
    string res = "[";
    int cnt = 0;
    iterator ei(begin());
    for(; ei && (cnt < 5); ++ei) {
        if (cnt++) {
            res += ",\n ";
        }
        res += (*ei)->toString(5, 3);
    }
    if (ei) {
        res += ",\n ...,\n " + at(size()-1)->toString(5, 3);
    }
    res += "]";
    return PyUnicode_FromString(res.c_str());
}



PyObject *TExampleTable::__item__(Py_ssize_t idx)
{
    if ((idx > size()) || (idx < 0)) {
        return PyErr_Format(PyExc_IndexError, "index %i is out of range", idx);
    }
    return at(idx).getPyObject();
}


inline int getIndexFromLong(PyObject *index, const int size)
{
    if (!PyLong_Check(index)) {
        raiseError(PyExc_TypeError,
            "indices must be integers, not %s", index->ob_type->tp_name);
    }
    int idx = (int)PyLong_AsLong(index);
    if (idx < 0) {
        idx += size;
    }
    if ((idx >= size) || (idx < 0)) {
        raiseError(PyExc_IndexError, "index %i is out of range", idx);
    }
    return idx;
}


bool TExampleTable::extractIndices(PyObject *pyidx,
                                   bool sorted,
                                   vector<int> &indices) const
{
    if (PyLong_Check(pyidx)) {
        indices.push_back(getIndexFromLong(pyidx, size()));
        return true;
    }

    if (PySlice_Check(pyidx)) {
        Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
        if (PySlice_GetIndicesEx(pyidx, size(),
                     &start, &stop, &step, &slicelength) < 0) {
            return false;
        }
#else
        if (PySlice_GetIndicesEx((PySliceObject *)pyidx, size(),
                     &start, &stop, &step, &slicelength) < 0) {
            return false;
        }
#endif
        indices.resize(slicelength);
        vector<int>::iterator ii(indices.begin());
        for(; slicelength--; start += step) {
            *ii++ = start;
        }
        // For deletion, indices need to be sorted in decreasing order
        if (sorted && (step > 0)) {
            reverse(indices.begin(), indices.end());
        }
        return true;
    }

    if (OrFilter_Check(pyidx)) {
        TFilter &filter = (TFilter &)((OrOrange *)pyidx)->orange;
        iterator ei = const_cast<TExampleTable *>(this)->begin();
        for(int i = 0; ei; ++ei, i++) {
            if (filter(*ei)) {
                indices.push_back(i);
            }
        }
        if (sorted) {
            reverse(indices.begin(), indices.end());
        }
        return true;
    }

    int const msize = size();
    PyObject *iter = PyObject_GetIter(pyidx);
    if (iter) {
        GUARD(iter);
        for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
            PyObject *it2 = item;
            GUARD(it2);
            indices.push_back(getIndexFromLong(item, msize));
        }
        // For deletion, indices need to be sorted in decreasing order
        if (sorted) {
            ::sort(indices.begin(), indices.end(), greater<int>());
        }
        return true;
    }

    PyErr_Format(PyExc_TypeError,
        "indices for must be integers, not %s", pyidx->ob_type->tp_name);
    return false;
}


PyObject *TExampleTable::subscript_slice(PyObject *index) const
{
    Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
    if (PySlice_GetIndicesEx(index, size(),
#else
    if (PySlice_GetIndicesEx((PySliceObject *)index, size(),
#endif
                     &start, &stop, &step, &slicelength) < 0) {
        return NULL;
        }
    PExampleTable newTable = constructEmptyReference(
        PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this)),
        THIS_AS_PyObject->ob_type);
    newTable->examples.reserve(slicelength);
    if (hasWeights()) {
        newTable->weights.reserve(slicelength);
    }
    int si, i;
    for(si = start, i = slicelength; i--; si += step) {
        newTable->examples.push_back(examples[si]);
        if (hasWeights()) {
            newTable->weights.push_back(weights[si]);
        }
    }
    return newTable.getPyObject();
}


PyObject *TExampleTable::subscript_iter(PyObject *iter) const
{
    PExampleTable newTable = TExampleTable::constructEmptyReference(
        PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this)),
        THIS_AS_PyObject->ob_type);
    GUARD(iter);
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
		PyObject *it2 = item;
		GUARD(it2);
        if (!PyLong_Check(item)) {
            return PyErr_Format(PyExc_TypeError,
                "Indices must be integers, not %s", item->ob_type->tp_name, NULL);
        }
        const int idx = getIndexFromLong(item, size());
        newTable->examples.push_back(examples[idx]);
        if (hasWeights()) {
            newTable->weights.push_back(weights[idx]);
        }
    }
    return newTable.getPyObject();
}


PyObject *TExampleTable::subscript_tuple_value(int const idx,
                                               TAttrIdx const attr) const
{
    if (attr < -1) {
        TMetaValue mr = MetaChain::get(getMetaHandle(idx), attr);
        PVariable var = domain->getVar(attr, false);
        if (var) {
            if (var->isPrimitive() != mr.isPrimitive) {
                return PyErr_Format(PyExc_TypeError,
                    "(non)primitive variables must have (non)primitive meta values");
            }
            TPyValue *pyvalue = var->isPrimitive()
                ? new TPyValue(var, mr.value) : new TPyValue(var, mr.object);
            return PyObject_FromNewOrange(pyvalue);
        }
        else {
            return PyFloat_FromDouble(mr.value);
        }
    }
    if (dataType != 'd') {
        return PyErr_Format(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    int const mattr = attr >= 0 ? attr : domain->attributes->size();
    PVariable var = domain->variables->at(mattr);
    TValue const &val = attributeOffsets.size() 
        ? (double &)examples[idx][attributeOffsets[mattr]]
        : ((double *)examples[idx])[mattr];
    TPyValue *pyvalue = new TPyValue(var, val);
    return PyObject_FromNewOrange(pyvalue);
}


PDomain TExampleTable::domainForTranslation(PyObject *pyattrs,
                                            vector<TAttrIdx> &attributes,
                                            int &copy) const
{
    PDomain newDomain;
    if (OrDomain_Check(pyattrs)) {
        newDomain = PDomain(pyattrs);
    }
    PVarList vars;
    PVariable classVar;
    if (!TDomain::varListFromVarList(
        pyattrs, domain->variables, vars, true, false)) {
            return PDomain();
    }
    TVarList::iterator vi, ve;
    for(vi = vars->begin(), ve = vars->end();
        (vi!=ve) && (*vi != domain->classVar); vi++);
    if (vi!=ve) {
        vars->erase(vi);
        classVar = domain->classVar;
    }
    newDomain = PDomain(new TDomain(classVar, vars));
    attributes.clear();
    PITERATE(TVarList, vi, newDomain->variables) {
        const int varNum = domain->getVarNum(*vi, false);
        if ((varNum == ILLEGAL_INT) || (varNum < 0)) {
            if (!copy) {
                raiseError(PyExc_ValueError,
                    varNum == ILLEGAL_INT
                        ? "cannot construct a reference table since variable '%s' is missing"
                        : "cannot construct a reference table since variable '%s' is a meta variable",
                    (*vi)->cname());
            }
            else {
                copy = 1;
            }
        }
        attributes.push_back(varNum);
    }
    if (copy == -1) {
        copy = 0;
    }
    return newDomain;
}


PyObject *TExampleTable::subscript_tuple_table(PyObject *pyrows,
                                               PyObject *pyattrs) const
{
    vector<int> attributes;
    int copy = -1;
    PDomain domain = domainForTranslation(pyattrs, attributes, copy);
    if (!domain) {
        return NULL;
    }
    if (PyLong_Check(pyrows)) {
        const long idx = getIndexFromLong(pyrows, size());
        PExample ex = const_cast<TExampleTable *>(this)->at(idx);
        return ex->convertedTo(domain).getPyObject();
    }
    vector<int> indices;
    if (!extractIndices(pyrows, false, indices)) {
        return NULL;
    }
    PExampleTable table;
    if (copy) {
        table = constructEmpty(domain, indices.size());
    }
    else {
        table = constructByColumns(
            PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this)),
            domain, attributes, false);
    }
    ITERATE(vector<int>, ei, indices) {
        table->examples.push_back(examples[*ei]);
        if (hasWeights()) {
            table->weights.push_back(weights[*ei]);
        }
    }
    return table.getPyObject();
}


PyObject *TExampleTable::subscript_tuple(PyObject *index) const
{
    const int tsize = PyTuple_GET_SIZE(index);
    if ((tsize != 1) && (tsize != 2)) {
        return PyErr_Format(PyExc_IndexError,
            "Invalid index; ExampleTables are two-dimensional");
    }
    PyObject *pyidx = PyTuple_GET_ITEM(index, 0);
    if ((tsize == 1) && PyLong_Check(pyidx)) {
        const long idx = getIndexFromLong(pyidx, size());
        return const_cast<TExampleTable *>(this)->at(idx).getPyObject();
    }
    PyObject *pyattr = PyTuple_GET_ITEM(index, 1);
    int attr = domain->getVarNum(pyattr, false);
    if (PyLong_Check(pyidx) && (attr != ILLEGAL_INT)) {
        const long idx = getIndexFromLong(pyidx, size());
        return subscript_tuple_value(idx, attr);
    }
    else {
        return subscript_tuple_table(pyidx, pyattr);
    }
}


PyObject *TExampleTable::__subscript__(PyObject *index) const
{
    if (OrFilter_Check(index)) {
        TFilter &filter = (TFilter &)((OrOrange *)index)->orange;
        PExampleTable me = PExampleTable::fromBorrowedPtr(
            const_cast<TExampleTable *>(this));
        return filter(me).getPyObject();
    }

    if (PySlice_Check(index)) {
        return subscript_slice(index);
    }
    if (PyTuple_Check(index)) {
        return subscript_tuple(index);
    }
    PyObject *iter = PyObject_GetIter(index);
    if (iter) {
        return subscript_iter(iter);
    }
    PyErr_Clear();
    if (PyLong_Check(index)) {
        const long idx = getIndexFromLong(index, size());
        PExample ex = const_cast<TExampleTable *>(this)->at(idx);
        return ex.getPyObject();
    }
    return PyErr_Format(PyExc_TypeError,
        "invalid index type (%s)", index->ob_type->tp_name);
}



int TExampleTable::ass_subscript_delete(int const attr, vector<int> const &indices)
{
    if (attr == ILLEGAL_INT) {
        const_ITERATE(vector<int>, di, indices) {
            erase(*di);
        }
        return 0;
    }
    if (attr >= -1) {
        PyErr_Format(PyExc_IndexError, "cannot delete attributes from example");
        return -1;
    }
    const_ITERATE(vector<int>, ii, indices) {
        MetaChain::remove(getMetaHandle(*ii), attr);
    }
    return 0;
}


int TExampleTable::ass_subscript_attribute(
    int const attr, PyObject *value, vector<int> const &indices)
{
    PVariable var = domain->getVar(attr, false);
    if (attr >= 0) {
        if (!var) {
			PyErr_Format(PyExc_IndexError, "index %i out of range", attr);
			return -1;
        }
        TValue val = var->py2val(value);
        if (dataType != 'd') {
            PyErr_Format(PyExc_NotImplementedError,
                "Only arrays of doubles are currently supported");
            return -1;
        }
        size_t pos = attributeOffsets.size() ? 
            attributeOffsets[attr] : attr * sizeof(double);
        const_ITERATE(vector<int>, ii, indices) {
            (double &)examples[*ii][pos] = val;
        }
        return 0;
    }

    if (var && !var->isPrimitive()) {
        PyObject *obj = var->py2pyval(value);
        const_ITERATE(vector<int>, ii, indices) {
            MetaChain::set(getMetaHandle(*ii), attr, obj);
        }
        return 0;
    }
    else {
        TValue val;
        if (var) {
            val = var->py2val(value);
        }
        else {
            val = PyNumber_AsDouble(value,
                "anonymous meta attributes can only have numeric values, not '%s'");
        }
        const_ITERATE(vector<int>, ii, indices) {
            MetaChain::set(getMetaHandle(*ii), attr, val);
        }
        return 0;
    }
}


/* What should this function do on views? Replace the example in the view
   or change the example in the base? Both would make sense, but since
   changing an attribute value (in other ass_subscript functions) changes
   the base, let this one do the same. We could however also decide otherwise
   since semantics of indexing is different enough (this one has a single
   index, others have two) */
int TExampleTable::ass_subscript_examples(
    PyObject *value, vector<int> const &indices)
{
    PExample ex = TExample::fromDomainAndPyObject(domain, value);
    if (!ex) {
        return -1;
    }
    int const firstMetaIndex = ex->supportsMeta() ? ex->getMetaHandle() : 0;
    if (firstMetaIndex) {
        checkSupportsMeta();
    }
    vector<int>::const_iterator const bb(indices.begin()), be(indices.end());
    vector<int>::const_iterator bi;
    if (supportsMeta()) {
        for(bi = bb; bi != be; bi++) {
            int &myMetaPtr = getMetaHandle(*bi);
            MetaChain::freeChain(myMetaPtr);
            copyDataToTable(examples[*bi], ex->values);
            MetaChain::copyChain(myMetaPtr, firstMetaIndex);
        }
    }
    else {
        for(bi = bb; bi != be; bi++) {
            copyDataToTable(examples[*bi], ex->values);
        }
    }
    return 0;
}


int TExampleTable::__ass_subscript__(PyObject *index, PyObject *value)
{
    vector<int> indices;
    int attr = ILLEGAL_INT;

    /* If you're thinking not to pre-compute indices, there two reasons for
       doing it (besides being simpler):
       - we want to check that all indices are OK before we start assigning
       - while deleting, we need to sort the indices and remove from the
         back of examples.
    */
    if (PyTuple_Check(index)) {
        const int tsize = PyTuple_GET_SIZE(index);
        if (tsize == 2) {
            attr = domain->getVarNum(PyTuple_GET_ITEM(index, 1));
            if (attr == -1) {
                attr = domain->attributes->size();
            }
        }
        else if (tsize != 1) {
            PyErr_Format(PyExc_IndexError,
                "Invalid index; ExampleTables are two-dimensional");
            return -1;
        }
        PyObject *pyidx = PyTuple_GET_ITEM(index, 0);
        if (!extractIndices(pyidx, !value, indices)) {
            return -1;
        }
    }
    else {
        if (!extractIndices(index, !value, indices)) {
            return -1;
        }
    }

    if (!value) {
        return ass_subscript_delete(attr, indices);
    }
    if (attr != ILLEGAL_INT) {
        return ass_subscript_attribute(attr, value, indices);
    }
    return ass_subscript_examples(value, indices);
}


Py_ssize_t TExampleTable::__len__() const
{
    return size();
}


int TExampleTable::__bool__() const
{
    return empty() ? 0 : 1;
}


PyObject *TExampleTable::native(PyObject *args, PyObject *kw) const
{
    int nativity=1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|i:native",
        ExampleTable_native_keywords, &nativity)) {
            return NULL;
    }
    PyObject *res = PyList_New(size());
    Py_ssize_t i = 0;
    PEITERATE(ei,
        PExampleTable::fromBorrowedPtr(const_cast<TExampleTable *>(this))) {
            PyList_SetItem(res, i++, ei->convertToPythonNative(nativity));
    }
    return res;
}


PyObject *TExampleTable::py_checksum(PyObject *args, PyObject *kw) const
{
    int includeMetas=0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|i:checksum",
        ExampleTable_checksum_keywords, &includeMetas)) {
            return NULL;
    }
    return PyLong_FromLong(checkSum(includeMetas!=0));
}


PyObject *TExampleTable::randomExample()
{
    if (!size()) {
        return PyErr_Format(PyExc_IndexError, "table is empty");
    }
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    const int idx = randomGenerator->randint(size());
    return at(idx).getPyObject();
}


PyObject *TExampleTable::py_clear()
{
    clear();
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_total_weight() const
{
    return PyFloat_FromDouble(totalWeight());
}


PyObject *TExampleTable::py_removeWeights()
{
    removeWeights();
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_setWeights(PyObject *args, PyObject *kw)
{
    if ((!args || !PyTuple_Size(args)) &&
        (!kw || !PyDict_Size(kw))) {
            ensureWeights();
    }
    else {
        double weight = 1.0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|d",
            ExampleTable_setWeights_keywords, &weight)) {
                return NULL;
        }
        setWeights(weight);
    }
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_hasMissing() const
{
    return PyBool_FromBool(hasMissing());
}


PyObject *TExampleTable::py_hasMissingClass() const
{
    return PyBool_FromBool(hasMissingClass());
}


PyObject *TExampleTable::py_shuffle()
{
    shuffle();
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_addMetaAttribute(PyObject *args, PyObject *kw)
{
    PyObject *pyid;
    PyObject *pyvalue=NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O:add_meta_attribute",
        ExampleTable_addMetaAttribute_keywords, &pyid, &pyvalue)) {
            return NULL;
    }
    PVariable var = domain->getMetaVar(pyid, false);
    TMetaId const id = domain->getMetaNum(pyid, true, false);
    if (!pyvalue) {
        if (var && !var->isPrimitive()) {
            raiseError(PyExc_TypeError, "meta value must be specified");
        }
        addMetaAttribute(id, 1.0);
    }
    else {
        if (!var) {
            double const value = PyNumber_AsDouble(pyvalue,
                "anonymous meta attributes must have numeric values, not %s");
            addMetaAttribute(id, value);
        }
        else if (var->isPrimitive()) {
            const int value = var->py2val(pyvalue);
            addMetaAttribute(id, value);
        }
        else {
            PyObject *value = var->pyval2py(pyvalue);
            GUARD(value);
            addMetaAttribute(id, value);
        }
    }
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_removeMetaAttribute(PyObject *arg)
{
    TMetaId const id = domain->getMetaNum(arg, true, false);
    removeMetaAttribute(id);
    Py_RETURN_NONE;
}

PyObject *TExampleTable::py_sort(PyObject *args, PyObject *kw)
{
    PyObject *pyorder = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:sort",
        ExampleTable_sort_keywords, &pyorder)) {
            return NULL;
    }

    if (!pyorder) {
        sort();
    }
    else {
        PVarList vars;
        if (!domain->varListFromVarList(pyorder, domain->variables, vars, true, true)) {
            return NULL;
        }
        vector<int> order;
        PITERATE(TVarList, vi, vars) {
            order.push_back(domain->getVarNum(*vi));
        }
        sort(order);
    }
    Py_RETURN_NONE;
}

PyObject *TExampleTable::py_removeDuplicates() 
{
    removeDuplicates(true);
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_changeDomain(PyObject *arg)
{
    return PyErr_Format(PyExc_NotImplementedError,
        "Domain conversion is no longer supported");
}


PyObject *TExampleTable::py_append(PyObject *arg)
{
    PExample ex(TExample::fromDomainAndPyObject(domain, arg, false));
    if (!ex) {
        return NULL;
    }
    push_back(ex);
    Py_RETURN_NONE;
}


PyObject *TExampleTable::py_extend(PyObject *arg)
{
    PExampleTable table(TExampleTable::fromDomainAndPyObject(domain, arg, false));
    if (!table) {
        return NULL;
    }
    extend(table);
    Py_RETURN_NONE;
}


PyObject *TExampleTable::__inplace_concat__(PyObject *arg)
{
    PyObject *res = py_extend(arg);
    if (!res) {
        return NULL;
    }
    Py_DECREF(res);
    res = THIS_AS_PyObject;
    Py_INCREF(res);
    return res;
}


PyObject *TExampleTable::__get__base(OrExampleTable *me)
{
    if (me->orange.base) {
        Py_INCREF(me->orange.base);
        return me->orange.base;
    }
    Py_RETURN_NONE;
}


PyObject *TExampleTable::__get__columns(OrExampleTable *me)
{
    return PDataColumns(new TDataColumns(me->orange.domain)).getPyObject();
}


PyObject *TExampleTable::py_sample(PyObject *args, PyObject *kw)
{
    PIntList folds;
    int fselect=-1;
    int copy=0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|ii:select",
        ExampleTable_sample_keywords, &PIntList::argconverter, &folds,
        &fselect, &copy)) {
            return NULL;
    }
    if (copy != 0) {
        return select(folds, fselect).getPyObject();
    }
    else {
        return selectref(folds, fselect).getPyObject();
    }
}


PyObject *TExampleTable::py_select(PyObject *args, PyObject *kw)
{
    PIntList folds;
    int fselect=-1;
    if (PyArg_ParseTupleAndKeywords(args, kw, "O&|i:select",
        ExampleTable_select_keywords, &PIntList::argconverter, &folds,
        &fselect)) {
            return select(folds, fselect).getPyObject();
    }
    PyErr_Clear();
    if (args && PyTuple_Size(args)==1 && (!kw || !PyDict_Size(kw))) {
        PyObject *translated = translate_w_flag(args, kw, 1);
        if (translated) {
            return translated;
        }
    }
    PyErr_Clear();
    return PyErr_Format(PyExc_TypeError, "invalid arguments for select");
}


PyObject *TExampleTable::py_selectref(PyObject *args, PyObject *kw)
{
    PIntList folds;
    int fselect=-1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|i:selectref",
        ExampleTable_select_keywords,
        &PIntList::argconverter, &folds, &fselect)) {
            return NULL;
    }
    return selectref(folds, fselect).getPyObject();
}


PyObject *TExampleTable::translate_w_flag(PyObject *args, PyObject *kw, int copy)
{
    PDomain newDomain;
    int keepMetas = 0;
    if (!kw && args &&
        (PyTuple_Size(args) == 1) && OrDomain_Check(PyTuple_GET_ITEM(args, 0))) {
            newDomain = PDomain(PyTuple_GET_ITEM(args, 0));
    }
    else {
        PyObject *pyattrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii:translate",
            ExampleTable_translate_keywords, &pyattrs, &keepMetas, &copy)) {
                return NULL;
        }
        if (OrDomain_Check(pyattrs)) {
            newDomain = PDomain(pyattrs);
        }
        else {
            PVarList vars;
            if (!TDomain::varListFromVarList(
                pyattrs, domain->variables, vars, true, false)) {
                    return NULL;
            }
            TVarList::iterator vi, ve;
            for(vi = vars->begin(), ve = vars->end();
                (vi!=ve) && (*vi != domain->classVar); vi++);
            if (vi==ve) {
                newDomain = PDomain(new TDomain(PVariable(), vars));
            }
            else {
                vars->erase(vi);
                newDomain = PDomain(new TDomain(domain->classVar, vars));
            }
        }
    }
    vector<int> attributes;
    // If unspecified, we copy if we can
    if (copy <= 0) {
        PITERATE(TVarList, vi, newDomain->variables) {
            const int varNum = domain->getVarNum(*vi, false);
            if ((varNum == ILLEGAL_INT) || (varNum < 0)) {
                if (!copy) {
                    raiseError(PyExc_ValueError,
                        varNum == ILLEGAL_INT
                            ? "cannot construct a reference table since variable '%s' is missing"
                            : "cannot construct a reference table since variable '%s' is a meta variable",
                        (*vi)->cname());
                }
                else {
                    copy = 1;
                }
            }
            attributes.push_back(varNum);
        }
        if (copy == -1) {
            copy = 0;
        }
    }
    if (copy) {
        PExampleTable newTable = constructConverted(
            PExampleTable::fromBorrowedPtr(this), newDomain, keepMetas==0);
        return newTable.getPyObject();
    }
    else {
        PExampleTable newTable = constructByColumns(
            PExampleTable::fromBorrowedPtr(this), newDomain, attributes);
        return newTable.getPyObject();
    }
}

PyObject *TExampleTable::py_translate(PyObject *args, PyObject *kw)
{
    return translate_w_flag(args, kw);
}


string getExtension(string const &filename);

PyObject *TExampleTable::py_save(PyObject *args, PyObject *kw) const
{
    PyObject *filename_b;
    if (!PyArg_ParseTupleAndKeywords(args, kw,
             "O&:sabe", ExampleTable_save_keywords, 
             &PyUnicode_FSConverter, &filename_b)) {
                 return NULL;
    }
    char const *filename = PyBytes_AsString(filename_b);
    string ext(getExtension(filename));
    if (ext == ".tab") {
        saveTab(filename);
    }
    else if (ext == ".txt") {
        saveTxt(filename);
    }
    else if (ext == ".csv") {
        saveCsv(filename);
    }
    else if (ext == ".basket") {
        saveBasket(filename);
    }
    else {
        raiseError(PyExc_ValueError,
            "unrecognized file format ('%s')", ext.c_str());
    }
    Py_RETURN_NONE;
}

#ifndef NO_NUMPY

void TExampleTable::parseMatrixContents(const char *contents,
    const int &multiTreatment, bool &hasClass, bool &classVector,
    // bool &multiclassVector,
    bool &weightVector, bool &classIsDiscrete, int &columns, vector<bool> &include)
{
    hasClass = bool(domain->classVar);
    columns = 0;
    int classIncluded = 0, attrsIncluded = 0, weightIncluded = 0, multiclassIncluded = 0;
    bool attrsRequested = false, classRequested = false;
    //  bool multiclassRequested = false;
    const char *cp;
    for(cp = contents; *cp && (*cp!='/'); cp++) {
        switch (*cp) {
        case 'A': attrsRequested = true;
        case 'a': attrsIncluded++;
            break;
        case 'C': classRequested = true;
        case 'c': classIncluded++;
            break;
        case 'W': 
        case 'w': weightIncluded++;
            break;
        /*      case 'M': multiclassRequested = true;
            case 'm': multiclassIncluded++;
            break;
        */
        case '0':
        case '1': columns++;
            break;
        default:
            raiseError(PyExc_ValueError,
                "unrecognized character '%c' in format string '%s')", *cp, contents);
        }
    }
    classVector = false;
    weightVector = false;
    bool classVectorOptional = true;
    if (*cp) {
        while(*++cp) {
            switch (*cp) {
            case 'A':
            case 'a': raiseError(PyExc_ValueError,
                          "invalid format string (attributes on the right side)");
            case '0':
            case '1': raiseError(PyExc_ValueError,
                          "invalid format string (constants on the right side)");
            case 'c': classVector = hasClass; break;
            case 'C': classVector = true; classVectorOptional = false; break;
            case 'w': weightVector = hasWeights(); break;
            case 'W': weightVector = true; break;
            /*        case 'm': multiclassVector = (domain->classVars->size() != 0); break;
                case 'M': multiclassVector = true; break;
            */
            default:
                raiseError(PyExc_ValueError, 
                    "unrecognized character '%c' in format string '%s')", *cp, contents);
            }
        }
    }
    if (classIncluded || classVector) {
        if (hasClass) {
            classIsDiscrete = domain->classVar->varType == TVariable::Discrete;
            if (classIsDiscrete) {
                TDiscreteVariable *eclassVar =
                    dynamic_cast<TDiscreteVariable *>(domain->classVar.borrowPtr());
                if (!eclassVar) {
                    raiseError(PyExc_SystemError,
                        "invalid variable type (non-discrete variable is declared as discrete");
                }
                if ((eclassVar->values->size() > 2) && (multiTreatment != 1)) {
                    if (classIncluded && classRequested ||
                        classVector && !classVectorOptional) {
                            raiseError(PyExc_ValueError,
                                "multinomial classes are allowed only when "
                                "treated as ordinal"); 
                    }
                    else {
                        classIncluded = 0;
                        classVector = false;
                        hasClass = false;
                    }
                }
            }
            columns += classIncluded;
        }
        else if (classRequested || classVector) {
            raiseError(PyExc_ValueError, "data has no class");
        }
    }
    if (weightIncluded || weightVector) {
        if (hasWeights())
            columns += weightIncluded;
    }
/*    if (multiclassIncluded || multiclassVector) {
        columns += multiclassIncluded * egen->domain->classVars->size();
    }
*/
    include.clear();
    if (attrsIncluded) {
        int attrs_in = 0;
        const_PITERATE(TVarList, vi, domain->attributes) {
            if ((*vi)->varType == TVariable::Continuous) {
                attrs_in++;
                include.push_back(true);
            }
            else {
                TDiscreteVariable *evar =
                    dynamic_cast<TDiscreteVariable *>(vi->borrowPtr());
                if (!evar) {
                    raiseError(PyExc_SystemError,
                        "invalid variable type (non-discrete variable is declared as discrete");
                }
                if (evar->values->size() == 2) {
                    attrs_in++;
                    include.push_back(true);
                }
                else {
                    switch (multiTreatment) {
                    case 0:
                        include.push_back(false);
                        break;
                    case 1:
                        attrs_in++;
                        include.push_back(true);
                        break;
                    default:
                        raiseError(PyExc_ValueError, 
                            "attribute '%s' is multinomial", (*vi)->cname());
                    }
                }
            }
        }
        if (attrsRequested && !attrs_in) {
            raiseError(PyExc_ValueError,
                "data has no attributes to include in the array");
        }
        columns += attrs_in * attrsIncluded;
    }
}


inline void storeNumPyValue(double *&p, TValue const &val, signed char *&m,
                            PVariable const &attr, int const &row)
{
    if (isnan(val)) {
        if (m) {
            *p++ = 0;
            *m++ = 1;
        }
        else {
            raiseError(PyExc_ValueError,
                "value of '%s' in row '%i' is undefined (use masked array)",
                attr->cname(), row);
        }
    }
    else {
        *p++ = val;
        if (m)
            *m++ = 0;
    }
}


PyObject *packMatrixTuple(PyObject *X, PyObject *y, /*PyObject *my, */
                          PyObject *w, char *contents)
{
    char *cp = strchr(contents, '/');
    if (!cp) {
        Py_INCREF(X);
        return X;
    }
    int left = (*contents && *contents != '/') ? 1 : 0;
    int right = cp ? strlen(++cp) : 0;
    PyObject *res = PyTuple_New(left + right);
    if (left) {
        Py_INCREF(X);
        PyTuple_SetItem(res, 0, X);
    }
    if (cp) {
        for(; *cp; cp++) {
            if ((*cp == 'c') || (*cp == 'C')) {
                Py_INCREF(y);
                PyTuple_SetItem(res, left++, y);
            }
            else if ((*cp == 'w') || (*cp == 'W')) {
                Py_INCREF(w);
                PyTuple_SetItem(res, left++, w);
            }
/*            else {
                Py_INCREF(my);
                PyTuple_SetItem(res, left++, my);
            }*/
        }
    }
    return res;
}


#define FAIL_IF(cond) \
    if (cond) { \
        Py_XDECREF(X); Py_XDECREF(y); Py_XDECREF(w); \
        return NULL; \
    }

inline bool maskArray(PyObject *&arr, signed char *&mp)
{
    PyObject *mask = PyArray_SimpleNew(
        PyArray_NDIM(arr), PyArray_DIMS(arr), NPY_BOOL);
    if (!mask) {
        return NULL;
    }

    PyObject *kw = PyDict_New();
    PyDict_SetItemString(kw, "mask", mask);
    PyObject *args = PyTuple_Pack(1, arr);
    PyObject *masked = PyObject_Call(MaskedArray_TypePtr, args, kw);
    Py_DECREF(args);
    Py_DECREF(kw);
    if (!masked) {
        Py_DECREF(mask);
        return false;
    }
    mp = (signed char *)PyArray_BYTES(mask);
    Py_DECREF(arr);
    arr = masked;
    return true;
}


PyObject *TExampleTable::toNumpy(PyObject *args, PyObject *kw)
{
    if (!numpyLoaded) {
        return PyErr_Format(PyExc_ImportError,
            "module numpy was not found or is incompatible with Orange");
    }
    char *contents = "a/cw";
    int multinomialTreatment = 1;
    int masked = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|sii:toNumpy",
        ExampleTable_toNumpy_keywords, &contents, &multinomialTreatment, &masked)) {
            return NULL;
    }
    bool hasClass, classVector, weightVector, classIsDiscrete; // , multiclassVector
    vector<bool> include;
    int columns;
    parseMatrixContents(contents, multinomialTreatment,
        hasClass, classVector, /* multiclassVector, */ weightVector,
        classIsDiscrete, columns, include);
    int rows = size();
    PVariable classVar = domain->classVar;
    PyObject *X = NULL, *y = NULL, *w = NULL, *masky = NULL;
//    PyObject *my, *maskmy = NULL;
    double *Xp = NULL, *yp = NULL, *wp = NULL;
//    double *myp;
    signed char *mp = NULL, *mpy = NULL, *mpmy = NULL;
    int xdim[] = {rows, columns};
    X = PyArray_SimpleNew(2, xdim, NPY_DOUBLE);
    FAIL_IF(!X);
    Xp = columns ? (double *)PyArray_BYTES(X) : NULL;
    FAIL_IF(masked && !maskArray(X, mp));
    if (classVector) {
        y = PyArray_SimpleNew(1, &rows, NPY_DOUBLE);
        FAIL_IF(!y);
        yp = (double *)PyArray_BYTES(y);
        FAIL_IF(masked && !maskArray(y, mpy));
    }
    else {
        y = Py_None;
        Py_INCREF(y);
    }
    /*
    if (multiclassVector) {
        int mdim = {rows, domain->classVars->size()};
        my = PyArray_SimpleNew(2, mdim, NPY_DOUBLE);
        FAIL_IF(!my);
        myp = (double *)PyArray_BYTES(my);
        FAIL_IF(masked && !maskArray(my, mpmy));
    }
    else {
        my = Py_None;
        Py_INCREF(my);
    }
    */
    if (weightVector) {
        w = PyArray_SimpleNew(1, &rows, NPY_DOUBLE);
        FAIL_IF(!w);
        wp = (double *)PyArray_BYTES(w);
    }
    else {
        w = Py_None;
        Py_INCREF(w);
    }

    try {
        int row = 0;
        iterator ei(begin());
        TVarList::const_iterator const vb(domain->attributes->begin());
        TVarList::const_iterator const ve(domain->attributes->end());
/*        TVarList::const_iterator const cb(domain->classes->begin());
        TVarList::const_iterator const ce(domain->classes->end());*/
        TVarList::const_iterator vi;
        vector<bool>::const_iterator const ib(include.begin());
        vector<bool>::const_iterator bi;
        for(; ei; ++ei, row++) {
            int col = 0;
            for(const char *cp = contents; *cp && (*cp!='/'); cp++) {
                switch (*cp) {
                case 'A':
                case 'a': {
                    TExample::iterator eei(ei->begin());
                    for(vi = vb, bi = ib; vi != ve; eei++, vi++, bi++) {
                        if (*bi) {
                            storeNumPyValue(Xp, *eei, mp, *vi, row);
                        }
                    }
                    break;
                }
                case 'C':
                case 'c':
                    if (hasClass) { // also false when class is multinomial
                        storeNumPyValue(Xp, ei.getClass(), mp, classVar, row);
                    }
                    break;
 /*             case 'M':
                case 'm': {
                    TValue *eei = ei->values_end;
                    for(vi = cb; vi != ce; eei++, vi++) {
                        storeNumPyValue(Xp, *eei, mp, *vi, row);
                    }
                    break;
                }*/
                case 'W':
                case 'w':
                    *Xp++ = ei.getWeight();
                    if (masked) {
                        *mp++ = 0;
                    }
                    break;
                case '0':
                    *Xp++ = 0.0;
                    if (masked) {
                        *mp++ = 0;
                    }
                    break;
                case '1':
                    *Xp++ = 1.0;
                    if (masked) {
                        *mp++ = 0;
                    }
                    break;
                }
            }
            if (yp) {
                storeNumPyValue(yp, ei.getClass(), mpy, classVar, row);
            }
            if (wp) {
                *wp++ = ei.getWeight();
            }
/*            if (myp) {
                TValue *eei = ei->values_end;
                for(vi = cb; vi != ce; eei++, vi++) {
                    storeNumPyValue(myp, *eei, mpmy, *vi, row);
                }
            }*/
        }
        PyObject *res = packMatrixTuple(X, y, /*my,*/ w, contents);
        Py_DECREF(X);
        Py_DECREF(y);
    //  Py_DECREF(my);
        Py_DECREF(w);
        return res;
    }
    catch (...) {
        Py_DECREF(X);
        Py_DECREF(y);
        Py_DECREF(w);
//        Py_XDECREF(my);
        throw;
    }
}


char const *const TExampleTable::numpyUnviewable() const
{
    if (size() > 2) {
        vector<char *>::const_iterator ei(examples.begin() + 1);
        vector<char *>::const_iterator const ee(examples.end());
        for(; ei != ee; ei++) {
            if (ei[0] - ei[-1] != rowSize) {
                return "rows are not contiguous";
            }
        }
    }
    if (attributeOffsets.size() && (attributeOffsets.size() > 2)) {
        vector<size_t>::const_iterator oi(attributeOffsets.begin() + 1);
        vector<size_t>::const_iterator const oe(attributeOffsets.end());
        size_t const colStride = attributeOffsets[0] - attributeOffsets[-1];
        for(; oi != oe; oi++) {
            if (oi[0] - oi[-1] != colStride) {
                return "columns are not contiguous";
            }
        }
    }
    return NULL;
}


PyObject *TExampleTable::asNumpy_view()
{
    int dims[] = {size(), domain->variables->size()};
    int strides[] = {
        size() < 2 ? rowSize : examples[1] - examples[0],
        attributeOffsets.size() < 2 ? sizeof(double) :
            attributeOffsets[1] - attributeOffsets[0]
    };
    char *const dataStart =
        examples[0] + (attributeOffsets.size() ? attributeOffsets[0] : 0);

    // lock the reference so this table's data never gets reallocated again
    getRawDataPtr();
    PyObject *arr = PyArray_New(&PyArray_Type, 2, dims, NPY_DOUBLE, strides,
        dataStart, strides[0]*size(), NPY_C_CONTIGUOUS | NPY_WRITEABLE, NULL);
    if (!arr) {
        return NULL;
    }
    PyObject *myself = PExampleTable::fromBorrowedPtr(this).borrowPyObject();
    /* TODO This function becomes available in numpy 1.7:
       PyArray_SetBaseObject(arr, myself);
           For now we hack to the field: */
    ((PyArrayObject *)arr)->base = myself;
    Py_INCREF(myself);
    /* TODO remove till here when we go to numpy 1.7 */
    return arr;
}


PyObject *TExampleTable::asNumpy_copy()
{
    int const ncols = domain->variables->size();
    int dims[] = {size(), ncols};
    PyObject *arr = PyArray_SimpleNew(2, dims, NPY_DOUBLE);
    int const rowStride = PyArray_STRIDE(arr, 0);
    int const colStride = PyArray_STRIDE(arr, 1);
    char *data = PyArray_BYTES(arr);
    for(iterator ei=begin(); ei; ++ei, data += rowStride) {
        char *colit = data;
        for(int i=0; i < ncols; i++, colit += colStride) {
            *(double *)colit = ei.value_at(i);
        }
    }
    return arr;
}


PyObject *TExampleTable::asNumpy(PyObject *args, PyObject *kw)
{
    int forceView = 0;
    int makeCopy = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|ii",
        ExampleTable_asNumpy_keywords, &forceView, &makeCopy)) {
            return NULL;
    }
    if (forceView && makeCopy) {
        return PyErr_Format(PyExc_ValueError,
            "incompatible options (force_view and copy)");
    }
    char const * const unviewable = makeCopy ? NULL : numpyUnviewable();
    if (unviewable && forceView) {
        return PyErr_Format(PyExc_ValueError,
            "cannot create a view: %s", unviewable);
    }
    return makeCopy || unviewable ? asNumpy_copy() : asNumpy_view();
}

#else

PyObject *TExampleTable::toNumpy(PyObject *args, PyObject *kw)
{
    return PyErr_Format(PyExc_SystemError,
        "this build of Orange does not support numpy");
}

PyObject *TExampleTable::asNumpy(PyObject *args, PyObject *kw)
{
    return PyErr_Format(PyExc_SystemError,
        "this build of Orange does not support numpy");
}


#endif

/// @endcond
