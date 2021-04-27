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
#include "example.px"


TExample::TExample(int const reftype, PDomain const &dom, PExampleTable const &tab,
                   TValue *vals, int const exidx, char *const refrow)
: values(vals),
  values_end(vals + dom->variables->size()),
  referenced_row(refrow),
  firstMeta(0),
  domain(dom),
  table(tab),
  referenceType(reftype),
  exampleIndex(exidx)
{
    if (table) {
        table->referenceCount++;
    }
}


TExample::TExample(TExample const &old)
: values((TValue *)memcpy(
             new double[old.values_end-old.values],
             old.values,
             (old.values_end-old.values)*sizeof(double))
         ),
  values_end(values + (old.values_end-old.values)),
  referenced_row(NULL),
  firstMeta(0),
  domain(old.domain),
  table(),
  referenceType(TExample::Free),
  exampleIndex(old.exampleIndex)
{
    MetaChain::copyChain(firstMeta, old.getMetaHandle());
}


/*! Constructs a free example with the given domain and initializes it
    with the given data. The size of the data must match the number of
    variables. */
PExample TExample::constructFree(PDomain const &dom, TValue *data)
{
    int const nvars = dom->variables->size();
    PExample ex(new TExample(Free, dom, PExampleTable(),
        new double[nvars], -1, NULL));
    if (data) {
        memcpy(const_cast<TValue *>(ex->values), data, nvars*sizeof(double));
    }
    else {
        for(TValue *vi = const_cast<TValue *>(ex->values);
            vi != ex->values_end;
            *vi++ = undefined_value);
    }
    return ex;
}


/*! Constructs a free example with the given domain and initializes it
    with the given data. The size of the data must match the number of
    variables. */
PExample TExample::constructFree(PDomain const &dom,
                                 const vector<double> &data)
{
    int nvars = dom->variables->size();
    if (data.size() != nvars) {
        raiseError(PyExc_ValueError,
            "length of values list (%i) does not match the number of attributes (%i)",
            data.size(), nvars);
    }
    PExample ex(new TExample(Free, dom, PExampleTable(),
        new double[nvars], -1, NULL));
    vector<double>::const_iterator di(data.begin());
    for(TValue *vi = const_cast<TValue *>(ex->values); nvars--; *vi++=*di++);
    return ex;
}


/*! Bug: does not copy meta values */
PExample TExample::constructCopy(PExampleTable const &tab,
                                 int const exampleIndex)
{
    char *const rowPtr = tab->examples[exampleIndex];
    int const nvars = tab->domain->variables->size();
    PExample ex(new TExample(Free, tab->domain, PExampleTable(),
        new double[nvars], -1, rowPtr));
    tab->copyDataToExample(ex->values, rowPtr);
    return ex;
}

/*! Constructs an example that refers to a row in a table.
    If the table is contiguous and the underlying type is double,
    the result is a #Direct. Otherwise, the function returns an #Indirect
    example.
    
    \param tab ExampleTable to which the example belongs
    \param exampleIndex The index of example (row number) in the table
    */
PExample TExample::constructTableItem(PExampleTable const &tab,
                                      int const exampleIndex)
{
    char *const rowPtr = tab->examples[exampleIndex];
    if(tab->isContiguous && (tab->dataType == 'd')) {
        return PExample(new TExample(Direct, tab->domain, tab,
            (double *)rowPtr, exampleIndex, rowPtr));
    }
    int const nvars = tab->domain->variables->size();
    PExample ex(new TExample(Indirect, tab->domain, tab,
        new double[nvars], exampleIndex, rowPtr));
    tab->copyDataToExample(ex->values, rowPtr);
    return ex;
}


/*! Returns a new free example from the given domain; its values are copied
    or computed from this example.
    \param dom Domain for the new example
    \param filterMetas If set to \c true, meta attributes are removed
*/
PExample TExample::convertedTo(PDomain const &dom,
                               bool const filterMetas) const
{
    if (domain == dom) {
        return PExample::fromBorrowedPtr(const_cast<TExample *>(this));
    }
    PExample newEx(constructFree(dom));
    dom->convert(newEx.borrowPtr(), this, filterMetas);
    return newEx;
}


/*! Sets the attribute value for indirect examples using #referenced_row 
    \param idx Attribute index; can be -1 for the class, but must not negative
    \param value Value
*/
void TExample::setValue_indirect(TAttrIdx const idx, TValue const value)
{
    if (!table) {
        raiseError(PyExc_SystemError,
            "A reference example has no reference table");
    }
    if (table->dataType != 'd') {
        raiseError(PyExc_NotImplementedError,
            "Only arrays of doubles are currently supported");
    }
    if (idx >= 0) {
        (double &)(referenced_row[table->attributeOffsets[idx]]) = value;
    }
    else {
        (double &)(referenced_row[table->attributeOffsets.back()]) = value;
    }
}


/*! Adds the example data to CRC32.
    \param crc CRC sum
    \param includeMetas Include meta attributes 
*/
void TExample::addToCRC(unsigned int &crc, bool const includeMetas) const
{
    for(TValue const *vli = values; vli != values_end; *vli++) {
        add_CRC(*vli, crc);
    }
    if (includeMetas) {
        for(int metaHandle = getMetaHandle();
            metaHandle;
            metaHandle = MetaChain::advance(metaHandle)) {
                TMetaValue mr = MetaChain::get(metaHandle);
                add_CRC(mr.id, crc);
                if (mr.isPrimitive) {
                    add_CRC(mr.value, crc);
                }
                else {
                    add_CRC((const unsigned long)(mr.object), crc);
                }
            }
    }
}


/*! Compares the example with another example; returns -1, 0 or 1.
    Undefined values are treated as greater than defined values.
    The other example is first converted to this example's domain. */
int TExample::cmp(TExample const &other) const
{
    PExample convother = other.convertedTo(domain);
    double const *my = values, *his = convother->values;
    for(; (my != values_end) && (isnan(*my) ? isnan(*his) : (*my == *his));
        my++, his++);
    if (my == values_end) {
        return 0;
    }
    return !isnan(*my) || (*my < *his) ? -1 : 1;
}


/// @cond Python
PExample TExample::fromDomainAndPyObject(PDomain const &domain,
                                         PyObject *pyvalues,
                                         bool const checkDomain)
{
    if (!pyvalues) {
        if (!domain) {
            raiseError(PyExc_TypeError,
                "cannot construct an example (unknown domain)");
        }
        return TExample::constructFree(domain);
    }
    if (OrExample_Check(pyvalues)) {
        PExample ex(pyvalues);
        if (checkDomain && (ex->domain != domain)) {
            raiseError(PyExc_ValueError,
                "example belongs to a different domain");
        }
        return ex;
    }
    PyObject *iter = domain ? PyObject_GetIter(pyvalues) : NULL;
    if (!iter) {
        raiseError(PyExc_TypeError,
            "cannot convert an instance of '%s' to Example",
            pyvalues->ob_type->tp_name);
    }
    GUARD(iter);
    vector<double> cvalues;
    TVarList::const_iterator vi(domain->variables->begin()),
        ve(domain->variables->end());
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter), vi++) {
        PyObject *it2 = item;
        GUARD(it2);
        if (vi == ve) {
            raiseError(PyExc_ValueError,
                "length of value list exceeds the number of variables");
        }
        cvalues.push_back((*vi)->py2val(item));
    }
    if (vi != ve) {
        raiseError(PyExc_ValueError, "not enough values for example");
    }
    return TExample::constructFree(domain, cvalues);
}


PyObject *TExample::convertToPythonNative(int const natvt) const
{
    TValue const *vli = values;
    PyObject *nlist = PyList_New(domain->variables->size());
    GUARD(nlist);
    int i = 0;
    const_PITERATE(TVarList, vi, domain->variables) {
        PyList_SetItem(nlist, i++, (*vi)->val2py(*vli++));
    }
    Py_INCREF(nlist);
    return nlist;
}


TOrange *TExample::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PDomain domain;
    PyObject *pyvalues = NULL;
    PyObject *pymetas = NULL; // for unpickling
    if (PyArg_ParseTuple(args, "O&|OO!:Example", &PDomain::argconverter,
                     &domain, &pyvalues, &PyList_Type, &pymetas)) {
        PExample ex = fromDomainAndPyObject(domain, pyvalues, true);
        if (pymetas) {
            for(Py_ssize_t i = 0, sze = PyList_Size(pymetas); i < sze; i++) {
                PyObject *item = PyList_GET_ITEM(pymetas, i);
                if (!PyTuple_Check(item) || (PyTuple_GET_SIZE(item) != 2)) {
                    raiseError(PyExc_UnpicklingError,
                        "invalid meta values; item %i is not a tuple of size 2", i);
                }
                PyObject *pyvar = PyTuple_GET_ITEM(item, 0);
                PyObject *pyval = PyTuple_GET_ITEM(item, 1);
                if (!OrVariable_Check(pyvar)) {
                    raiseError(PyExc_UnpicklingError,
                        "invalid meta values; item %i does not contain a variable", i);
                }
                PVariable var(pyvar);
                const TMetaId id = domain->getVarNum(var);
                if (var->isPrimitive()) {
                    ex->setMeta(id, pyval == Py_None ? 
                        UNDEFINED_VALUE : PyFloat_AsDouble(pyval));
                }
                else {
                    ex->setMeta(id, pyval == Py_None ?
                        (PyObject *)NULL : pyval);
                }
            }
        }
        return ex.getPtr();
    }
    PyErr_Clear();
    PExample example;
    if (PyArg_ParseTuple(args, "O&:Example", &PExample::argconverter, &example)) {
        return new(type) TExample(*example);
    }
    PyErr_Clear();
    raiseError(PyExc_TypeError, "invalid arguments for 'Example'");
	return NULL;
}


PyObject *TExample::__getnewargs__() const
{
    if (referenceType != Free) {
        raiseError(PyExc_PicklingError,
            "examples that reference table rows cannot be pickled");
    }
    PyObject *pyvalues = convertToPythonNative();
    GUARD(pyvalues);
    if (supportsMeta()) {
        PyObject *pymetas = MetaChain::packChain(getMetaHandle(), domain);
        return Py_BuildValue("(NON)", domain.getPyObject(), pyvalues, pymetas);
    }
    else {
        return Py_BuildValue("(NOO)", domain.getPyObject(), pyvalues, Py_None);
    }
}


Py_ssize_t TExample::__len__() const
{
    return (Py_ssize_t)(values_end - values);
}


PyObject *TExample::__item__(ssize_t idx)
{
    if (idx == -1) {
        return py_getclass();
    }
    if (idx < 0) {
        TMetaValue value = getMeta((long)idx);
        PVariable var = domain->getMetaVar((int)idx, false);
        if (value.isPrimitive) {
            return PyObject_FromNewOrange(new TPyValue(var, value.value));
        }
        else {
            return PyObject_FromNewOrange(
                new TPyValue(var, var->pyval2py(value.object)));
        }
    }
    PVarList variables = domain->variables;
    int const nattrs = variables->size();
    if (idx >= nattrs) {
        raiseError(PyExc_IndexError, "index %i out of range", idx);
    }
    return PyObject_FromNewOrange(new TPyValue(variables->at(idx), values[idx]));
}


PyObject *TExample::__subscript__(PyObject *index)
{
    const TAttrIdx idx = domain->getVarNum(index);
    return __item__(idx);
}


int TExample::__ass_subscript__(PyObject *index, PyObject *value)
{
    const TAttrIdx idx = domain->getVarNum(index);
    if (!value) {
        if (idx < -1) {
            removeMeta(idx);
            return 0;
        }
        else {
            PyErr_Format(PyExc_IndexError, "cannot remove non-meta attributes");
            return -1;
        }
    }
    if (idx == -1) {
        PyObject *sr = py_setclass(value);
        Py_XDECREF(sr);
        return sr ? 0 : -1;
    }
    if (idx < 0) {
        PVariable const var = domain->getMetaVar(TAttrIdx(idx), false);
        if (var) {
            if (var->isPrimitive()) {
                setMeta(idx, var->py2val(value));
            }
            else {
                setMeta(idx, var->py2pyval(value));
            }
        }
        else if (PyNumber_Check(value)) {
            setMeta(idx, PyNumber_AsDouble(value));
        }
        else {
            PyErr_Format(PyExc_TypeError,
                "cannot set anonymous meta attribute to an instance of '%s'",
                value->ob_type->tp_name);
            return -1;
        }
    }
    else {
        PVariable const var = domain->variables->at(idx);
        setValue(idx, var->py2val(value));
    }
    return 0;
}

    
string TExample::toString(int limitFeatures, int limitMeta) const
{
    string res;
    TValue const *vli = values;
    TVarList::const_iterator const vb(domain->variables->begin());
    TVarList::const_iterator const ve(domain->variables->end());
    TVarList::const_iterator vi(vb);
    for(; vi != ve; vi++) {
        if (res.size()) {
            res += ", ";
        }
        res += (*vi)->val2str(*vli++);
        if (--limitFeatures == 0) {
            vi++;
            break;
        }
    }
    if (vi != ve) {
        res += ", ..., " + ve[-1]->val2str(values[ve-vb-1]);
    }
    res = "["+res+"]";

    int mi = getMetaHandle();
    if (mi) {
        string metas;
        char buf[64];
        int cnt = 0;
        for(; mi; mi = MetaChain::advance(mi)) {
            metas += cnt++ ? ", " : " {";
            TMetaValue mr = MetaChain::get(mi);
            PVariable var = domain->getMetaVar(mr.id, false);
            if (var) {
                metas += var->getName() + ": " + (var->isPrimitive() ?
                    var->val2str(mr.value) : var->pyval2str(mr.object));
            }
            else {
                snprintf(buf, 64, "%i: %.3f", mr.id, mr.value);
                metas += buf;
            }
        }
        metas += "}";
        if (cnt > limitMeta) {
            snprintf(buf, 64, ", %i meta attributes", cnt);
            res += buf;
        }
        else {
            res += metas;
        }
    }
    return res;
}


PyObject *TExample::__repr__() const
{
    return PyUnicode_FromString(toString(30, 30).c_str());
}


PyObject *TExample::__str__() const
{
    return PyUnicode_FromString(toString().c_str());
}


long TExample::__hash__() const
{
    return checkSum();
}


// Thanks, PyValue!
PyObject *compare_op(const int cmpr, const int op);

PyObject *TExample::__richcmp__(PyObject *pyother, int op) const
{
    PExample const other = fromDomainAndPyObject(domain, pyother, false);
    return compare_op(cmp(*other), op);
}


PyObject *TExample::__get__id(OrExample *self)
{
    TExample const &me = self->orange;
    return PyLong_FromVoidPtr(me.referenced_row ?
        (void *)me.referenced_row : (void *)me.values);
}

PyObject *TExample::py_getclass() const
{
    PVariable const classVar = domain->classVar;
    if (!classVar) {
        return PyErr_Format(PyExc_AttributeError, "example has no class");
    }
    return PyObject_FromNewOrange(new TPyValue(classVar, values_end[-1]));
}


PyObject *TExample::py_setclass(PyObject *arg)
{
    PVariable const classVar = domain->classVar;
    if (!classVar) {
        return PyErr_Format(PyExc_AttributeError, "example has no class");
    }
    setClass(classVar->py2val(arg));
    Py_RETURN_NONE;
}

PyObject *TExample::native(PyObject *args, PyObject *kw) const
{
    int nativity=1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|i:native",
            Example_native_keywords, &nativity)) {
        return NULL;
    }
    return convertToPythonNative(nativity);
}

PyObject *TExample::py_set_meta(PyObject *args, PyObject *kw)
{
    PyObject *pyvar, *pyvalue;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO:setmeta",
        Example_set_meta_keywords, &pyvar, &pyvalue)) {
            return NULL;
    }
    if (__ass_subscript__(pyvar, pyvalue) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}
    
PyObject *TExample::py_get_meta(PyObject *pyvar) const
{
    PVariable var = domain->getMetaVar(pyvar);
    int const id = domain->getMetaNum(var);
    TMetaValue const &meta = getMeta(id);
    return PyObject_FromNewOrange(new TPyValue(var, meta));
}


static char *Example_getmetas_keywords[] = {"optional", "key_type", NULL};

PyObject *TExample::py_get_metas(PyObject *args, PyObject *kw) const
{
    PyTypeObject *keytype = &PyLong_Type;
    int optional = ILLEGAL_INT;
    if (args && (PyTuple_Size(args)==1)) {
        keytype = (PyTypeObject *)PyTuple_GET_ITEM(args, 0);
    }
    else {
        if (!PyArg_ParseTuple(args, "|iO:getmetas", &optional, &keytype))
            return NULL;
    }
    if ((keytype != &PyLong_Type) &&
        (keytype != &PyUnicode_Type) &&
        (keytype != (PyTypeObject *)&OrVariable_Type)) {
            return PyErr_Format(PyExc_TypeError,
                "invalid key type (%s); must be int, string, Variable or nothing",
                keytype->tp_name);
    }
    PyObject *res = PyDict_New();
    GUARD(res);
    for(int mi = getMetaHandle(); mi; mi = MetaChain::advance(mi)) {
        TMetaValue mr = MetaChain::get(mi);
        PyObject *key, *value;
        PVariable const var = ((keytype == &PyLong_Type) && mr.isPrimitive)
                    ? PVariable() : domain->getMetaVar(mr.id, false);
        if (keytype == &PyLong_Type) {
            key = PyLong_FromLong(mr.id);
        }
        else if (keytype == &PyUnicode_Type) {
            key = PyUnicode_FromString(var->cname());
        }
        else {
            key = var.getPyObject();
        }
        if (mr.isPrimitive) {
            value = PyFloat_FromDouble(mr.value);
        }
        else {
            value = var->py2pyval(mr.object);
        }
        PyDict_SetItem(res, key, value);
        Py_DECREF(key);
        Py_DECREF(value);
    }
    Py_INCREF(res); // Guard took one reference
    return res;
}
                

PyObject *TExample::py_has_meta(PyObject *arg) const
{
    return PyBool_FromBool(hasMeta(domain->getMetaNum(arg, true, false)));
}


PyObject *TExample::py_remove_meta(PyObject *arg)
{
    removeMeta(domain->getMetaNum(arg, true, false));
    Py_RETURN_NONE;
}


PyObject *TExample::py_get_weight() const
{
    return PyFloat_FromDouble(getWeight());
}

PyObject *TExample::py_set_weight(PyObject *args)
{
    setWeight(PyNumber_AsDouble(args));
    Py_RETURN_NONE;
}

/// @endcond