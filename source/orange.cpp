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
#include "orange.px"

/// Casts <tt>(char *)self + offset</tt> to <tt>POrange *</tt>. Internal use only.
#define POrangePtr_FROM_OFFSET(self, offset) \
    ((POrange *)(((char *)(self)) + ((offset) & 0x1fffffff)))

/// Returns the object referenced by </tt>(char *)self + offset</tt>. Internal use only.
#define POrange_FROM_OFFSET(self, offset) \
    (*POrangePtr_FROM_OFFSET(self, offset))

/// Returns the PyObject pointed to by </tt>(char *)self + offset</tt>
#define MEMBER_FROM_OFFSET(self, offset) \
    (POrange_FROM_OFFSET(self, offset).borrowPyObject())



/*! Cast a new Orange object to PyObject. The difference between this function
    and PyObject_FromOrange is that PyObject_FromNewOrange assumes that the
    reference is new and does not increase the reference count, while
    PyObject_FromOrange, for constrast, increases the reference count.
    
    This function also registers the object with Python's garbage collection
    tracker by calling TOrange::start_gc_tracking.
    
    The function returns a stolen reference.*/
PyObject *PyObject_FromNewOrange(TOrange *orange)
{
    if (!orange) {
        Py_RETURN_NONE;
    }
    TOrange::start_gc_tracking(orange);
    return AS_PyObject(orange);
}

/*! Constructor; does nothing except for setting #orange_dict to \c NULL. */
TOrange::TOrange()
: orange_dict(NULL)
{}


/*! Destructor; decreases reference count of #orange_dict. */
TOrange::~TOrange()
{
    Py_XDECREF(orange_dict);
}


/*! Allocates a PyObject of type OrOrange and returns the pointer to its
    field TOrange::orange. Instead of \c malloc, \c PyObject_GC_New is used
    to allocate the memory. The allocated memory for TOrange instance is set
    to 0. The object is not added to gc tracking since it is not initialized
    yet; this is only done when the object is first wrapped into a P-class or
    cast to PyObject using PyObject_FromNewOrange.

    The operator sets the Python type as specified by the position argument
    (\c type).

    TOrange defines a new operator which raises exception ("cannot instantiate
    abstract class Orange"). Macro __REGISTER_CLASS provides an overloaded
    operator for non-abstract classes.

    TOrange has only this, positional new operator. Non-abstract derived classes
    also have the non-positional operator that calls the positional with the
    default type (<tt>Or(classname)_Type</tt>)
*/
void *TOrange::operator new(size_t ssize, PyTypeObject *type) throw()
{
    raiseError(PyExc_SystemError, "cannot instantiate abstract class Orange");
    return NULL;
}

/*! Returns a (shallow) copy of the object. TOrange's method raises an
    exception. Macro __REGISTER_CLASS declares an overloaded operator for
    non-abstract classes, and pyprops defined it in the px file. */
TOrange *TOrange::clone() const
{
    raiseError(PyExc_SystemError, "cannot clone instances of abstract classes");
    return NULL;
}

/*! Frees the object by calling the type's tp_free. */
void TOrange::operator delete(void *obj)
{
    PyObject *self = AS_PyObject((TOrange *)obj);
    // MEMORY LEAK HERE: if the object is not tracked, may we
    // still call tp_free?
    if (_PyObject_GC_IS_TRACKED(self)) {
        Py_TYPE(self)->tp_free(self);
    }
}

/*! Frees the object by calling the non-positional operator delete. */
void TOrange::operator delete(void *obj, PyTypeObject *)
{   
    TOrange::operator delete(obj);
} 


/*! A standard special method that packs the instance's dictionary for
    pickling. It returns a dictionary with all members that are not read only,
    all attributes that have getters and setters and the content of
    #orange_dict. */
PyObject *TOrange::__getstate__(PyObject *self)
{
    try {
        PyObject *res = PyDict_New();
        for(PyTypeObject *type = self->ob_type;
                type != &OrOrange_Type; type = type->tp_base) {
            PyMemberDef const *member = type->tp_members;
            if (member) {
                for(; member->name; member++) {
                    if ((member->flags & READONLY) != 0) {
                        continue;
                    }
                    PyObject *val = PyObject_GetAttrString(self, member->name);
                    if (!val) {
                        Py_DECREF(res);
                        return NULL;
                    }
                    PyDict_SetItemString(res, member->name, val);
                    Py_DECREF(val);
                }
            }
            PyGetSetDef const *getsetter = type->tp_getset;
            if (getsetter) {
                for(; getsetter->name; getsetter++) {
                    if (!getsetter->get || !getsetter->set) {
                        continue;
                    }
                    PyObject *val = getsetter->get(self, getsetter->closure);
                    if (!val) {
                        Py_DECREF(res);
                        return NULL;
                    }
                    PyDict_SetItemString(res, getsetter->name, val);
                    Py_DECREF(val);
                }
            }
        }
        TOrange &o = ((OrOrange *)self)->orange;
        if (o.orange_dict) {
            PyObject *d_key, *d_value;
            Py_ssize_t i = 0;
            while (PyDict_Next(o.orange_dict, &i, &d_key, &d_value)) {
                PyDict_SetItem(res, d_key, d_value);
            }
        }
        return res;
    }
    PyCATCH
}


/*! A standard special method for unpickling the members from a dictionary.
    The method assigns all attributes using PyObject_SetAttr, without
    distinguishing between different kinds of attributes (members, attributes
    with getters and setters etc.) */
PyObject *TOrange::__setstate__(PyObject *self, PyObject *dict)
{   
    try {
        PyObject *d_key, *d_value;
        Py_ssize_t i = 0;
        while (PyDict_Next(dict, &i, &d_key, &d_value)) {
            PyObject_SetAttr(self, d_key, d_value);
        }
        Py_RETURN_NONE;
    }
    PyCATCH
}


/*! A method for clearing the references if the object is garbage collected.
    The method is assigned to tp_clear slot of OrOrange_Type and needs
    to be overloaded only if the object owns any P-instances that are not
    exposed to Python through getters.

    The method sets all pointers to P-classes to NULL;
    these are recognized as members whose getters are set to #getter_orange.
    It also calls Py_CLEAR for #orange_dict.
    */
int TOrange::clear_references()
{
    PyObject *self = THIS_AS_PyObject;
    for(PyTypeObject *tpe = self->ob_type;
            tpe != &PyBaseObject_Type; tpe = tpe->tp_base) {
        PyGetSetDef *gsi = tpe->tp_getset;
        if (!gsi)
            continue;
        for (; gsi->name; gsi++) {
            if (gsi->get == (getter)getter_orange) {
                size_t const offset = *(ssize_t *)(gsi->closure);
                if ((offset & 0x20000000) == 0) {
                    POrangePtr_FROM_OFFSET(self, offset)->clear();
                }
            }
        }
    }
    Py_CLEAR(orange_dict);
    return 0;
}

/*! A method for traversing the references owned by the object. The method is
    assigned to the tp_traverse slot of OrOrange_Type and only needs to be
    overloaded only if the object owns any P-instancs that are not exposed to
    Python throught getters.

    The method also visits #orange_dict. */
int TOrange::traverse_references(visitproc visit, void *arg)
{
    PyObject *self = THIS_AS_PyObject;
    for(PyTypeObject *tpe = self->ob_type;
            tpe != &PyBaseObject_Type; tpe = tpe->tp_base) {
        PyGetSetDef *gsi = tpe->tp_getset;
        if (!gsi)
            continue;
        for (; gsi->name; gsi++) {
            if (gsi->get == (getter)getter_orange) {
                size_t const offset = *(ssize_t *)(gsi->closure);
                if ((offset & 0x20000000) == 0) {
                    PyObject *o = MEMBER_FROM_OFFSET(self, offset);
                    Py_VISIT(o);
                }
            }
        }
    }
    return orange_dict ? visit(orange_dict, arg) : 0;
}


/*! Prints out the generic description of instance. */
PyObject *TOrange::__repr__(PyObject *self)
{ 
    const char *tp_name = self->ob_type->tp_name;
    return PyUnicode_FromFormat("<%s instance at %p>", tp_name, self);
}


/*! Returns a hash value computed from pointer \c this. The method needs
    to be overloaded if the class overloads the Python comparison operator. */
long TOrange::__hash__() const
{
    return _Py_HashPointer(const_cast<TOrange *>(this));
}


/*! Method for unpickling instances of classes that implement \c reduce.
    The method checks that the type defines \c tp_new and calls it, or raises
    a Python error if it does not exist. */
PyObject *TOrange::__newobj__(PyObject *cls, PyObject *args)
{
    PyTypeObject *tcls = (PyTypeObject *)cls;
    if (!tcls->tp_new) {
        return PyErr_Format(PyExc_SystemError,
            "Class %s has no __new__ method", tcls->tp_name);
    }
    return tcls->tp_new(tcls, args, NULL);
}

/*! Getter for retrieving attribute values that are instances of TOrange.
    The data pointed to by (char *)self + *offset is interpreted as a pointer
    to TOrange. NULL pointers are interpreted as \c Py_None and non-NULLs
    are cast into PyObjects using the #AS_PyObject macro. The function returns
    a new reference to the resulting object.

    Unlike setters which are type specific, this getter is used for all subtypes
    of TOrange. */
PyObject *getter_orange(PyObject *self, size_t *offset)
{
    TOrange *fld = *(TOrange **)((char *)(self)+(*offset & 0x1fffffff));
    PyObject *obj = fld ? AS_PyObject(fld) : Py_None;
    Py_INCREF(obj);
    return obj;
}


/*! Converts a camel-case name to name with underscores by replacing
    all combinations of [a-z][A-Z] with [a-z]_[a-z]. If the name is not
    camel-cased the function returns NULL, otherwise it returns the changed
    string. The caller is responsible for deallocating the string. */
char *camel2underscore(const char *camel)
{
    const char *ci = camel;
    if ((*ci >= 'A') && (*ci <= 'Z')) {
        return NULL;
    }

    char *underscored = (char *)malloc(2*strlen(camel)+1);
    char *ui = underscored;
    bool changed = false;
    *ui = *ci;
    while(*ci) { // just copied
        if (   (*ci >= 'a') && (*ci <= 'z')       // a small letter
            && (ci[1] >= 'A') && (ci[1] <= 'Z')) {  // followed by capital
            *++ui = '_';
            const char of = (ci[2] < 'A') || (ci[2] > 'Z') ? 32 : 0; // 32, if not followed by capital 
            *++ui = *++ci + of;
            changed = true;
        }
        else {
            *++ui = *++ci;
        }
    }
    if (!changed) {
        free(underscored);
        underscored = NULL;
    }
    return underscored;
}


#include "alias_attributes.px"

/*! Gets the object's attribute with all possible translations.
    It first tries the name as given in the argument by calling
    \c PyObject_GenericGetAttr. If this fails, it checks whether the
    attribute name is obsolete or camel-cased and tries to get the
    modified name; in the latter case it uses
    Python's semi-internal \c _PyObject_GenericGetAttrWithDict to prevent
    pulling the attribute from #orange_dict. */
PyObject *TOrange::__getattr__(PyObject *self, PyObject *name)
{
    PyObject *res;
    res = PyObject_GenericGetAttr(self, name);
    if (res)
        return res;
    PyErr_Clear();

    string cname = PyUnicode_As_string(name);
    char const *translated = translateAttributeName(self, cname.c_str());
    if (translated) {
        PyObject *pyname = PyUnicode_FromString(translated);
        res = PyObject_GenericGetAttr(self, pyname);
        if (res) {
            return res;
        }
        PyErr_Clear();
    }

    char *underscored = camel2underscore(cname.c_str());
    if (underscored) {
        PyObject *pyname = PyUnicode_FromString(underscored);
        free(underscored);
        // We don't allow pulling a modified name from the instance's dict!
        PyObject *fake_dict = PyDict_New();
        res = _PyObject_GenericGetAttrWithDict(self, pyname, fake_dict);
        Py_DECREF(fake_dict);
        Py_DECREF(pyname);
        if (res)
            return res;
        PyErr_Clear();
    }

    return PyErr_Format(PyExc_AttributeError,
        "'%s' object has no attribute '%s'",
        self->ob_type->tp_name, cname.c_str());
}


/*! Sets an attribute observing its possible translations.
    It first calls the Python's semi-internal
    \c _PyObject_GenericSetAttrWithDict that sets the attribute but can trap
    the attributes that end up in the instance's #orange_dict. In this case it
    tries the translated obsolete name and conversion from camel-case to
    underscored name. If none of these succeeds, it calls 
    \c PyObject_GenericSetAttr to set the attribute using #orange_dict. */
int TOrange::__setattr__(PyObject *self, PyObject *name, PyObject *value)
{
    PyObject *fake_dict = PyDict_New();
    int ok;
    
    // We supply a trap-dictionary: if the value goes there, we're not done yet
    ok = _PyObject_GenericSetAttrWithDict(self, name, value, fake_dict);
    if ((ok==0) && !PyDict_Size(fake_dict)) {
        Py_DECREF(fake_dict);
        return 0;
    }

    string cname = PyUnicode_As_string(name);
    char const *translated = translateAttributeName(self, cname.c_str());
    if (translated) {
        PyObject *pyname = PyUnicode_FromString(translated);
        // This can fail only if there is an error in the alias file
        ok = _PyObject_GenericSetAttrWithDict(self, pyname, value, fake_dict);
        Py_DECREF(pyname);
        if ((ok == 0) && !PyDict_Size(fake_dict)) {
            Py_DECREF(fake_dict);
            return 0;
        }
        PyErr_Clear();
    }

    // We try to set the underscored, but if it goes to the dictionary, we're not done
    char *underscored = camel2underscore(cname.c_str());
    if (underscored) {
        PyObject *pyname = PyUnicode_FromString(underscored);
        free(underscored);
        PyDict_Clear(fake_dict);
        ok = _PyObject_GenericSetAttrWithDict(self, pyname, value, fake_dict);
        Py_DECREF(pyname);
        if ((ok == 0) && !PyDict_Size(fake_dict)) {
            Py_DECREF(fake_dict);
            return 0;
        }
    }

    // Now we do set into the dictionary, but with the original name
    Py_DECREF(fake_dict);
    return PyObject_GenericSetAttr(self, name, value);
}



/*! Splits the given dictionary in two: the first contain keywords from the
    list 'recognized', the second contains the rest. This is function is used in
    Orange_GenericNewWithKey to pass only the recognized keywords to the
    constructor and setting the rest as attributes. */
void keywordsForConstructor(
    PyObject *kw, char *recognized[], PyObject *&keywords, PyObject *&attributes)
{
    if (!kw) {
        keywords = attributes = NULL;
        return;
    }
    if (!recognized) {
        Py_INCREF(kw);
        attributes = kw;
        keywords = NULL;
        return;
    }

    keywords = PyDict_New();
    attributes = PyDict_New();

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    char *const *ri;
    while (PyDict_Next(kw, &pos, &key, &value)) {
        if (!PyUnicode_Check(key)) {
            PyDict_SetItem(attributes, key, value);
            continue;
        }
        string cname = PyUnicode_As_string(key);
        for(ri = recognized; *ri && strcmp(*ri, cname.c_str()); ri++);
        PyDict_SetItem(*ri ? keywords : attributes, key, value);
    }
}


/*! Sets a value of built-in attribute exposed to Python as a member or
    through get-setters; called by #setBuildInAttributes.
    The function does not check for obsolete and camel-case names.
    \c key The name of the attribute
    \c value The value to set
    \c ckey The key as char const *; must be the same as #key; repeated for efficiency
    \c self The instance to which the attribute is set */
bool setBuiltInAttribute(PyObject *key, PyObject *value,
                         char const *ckey, PyObject *self)
{
    for(PyTypeObject *type = self->ob_type; 
            type != &OrOrange_Type;
            type = type->tp_base) {
        PyMemberDef const *member = type->tp_members;
        if (member) {
            for(; member->name; member++) {
                if (!strcmp(member->name, ckey)) {
                    if ((member->flags & READONLY) != 0) {
                        raiseError(PyExc_TypeError,
                            "%s is a read-only attribute", ckey);
                    }
                    if (PyObject_SetAttr(self, key, value) == -1) {
                        throw PyException();
                    }
                    return true;
                }
            }
        }
        PyGetSetDef const *getsetter = type->tp_getset;
        if (getsetter) {
            for(; getsetter->name; getsetter++) {
                if (!strcmp(getsetter->name, ckey)) {
                    if (!getsetter->set) {
                        raiseError(PyExc_TypeError,
                            "%s is a read-only attribute", ckey);
                    }
                    if (getsetter->set(self, value, getsetter->closure) == -1) {
                        throw PyException();
                    }
                    return true;
                }
            }
        }
    }
    return false;
}
 

/*! Sets the attributes exposed to Python as member or through get-setters.
    All unused attributes are returned in a new dictionary. The function also
    tries translations from obsolete names and camel-case names.
    \param kw A list of attributes
    \param self The instance to which the attributes are set
    \param nonAttributes Unused elements of #kw. */
void setBuiltInAttributes(PyObject *kw, PyObject *self, PyObject *&nonAttributes)
{
    if (!kw) {
        nonAttributes = NULL;
        return;
    }
    nonAttributes = PyDict_New();
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kw, &pos, &key, &value)) {
        bool done = false;
        if (PyUnicode_Check(key)) {
            string cname = PyUnicode_As_string(key);
            done = setBuiltInAttribute(key, value, cname.c_str(), self);
            if (!done) {
                char const *translated = translateAttributeName(self, cname.c_str());
                if (translated) {
                    done = setBuiltInAttribute(key, value, translated, self);
                }
                if (!done) {
                    char *underscored = camel2underscore(cname.c_str());
                    if (underscored) {
                        done = setBuiltInAttribute(key, value, underscored, self);
                        free(underscored);
                    }
                }
            }
        }
        if (!done) {
            PyDict_SetItem(nonAttributes, key, value);
        }
    }
}


/*! Sets all attributes given in the dict using the method from the
    \c tp_setattro slot or \c PyObject_GenericSetAttr. */
bool setAttr_FromDict(PyObject *self, PyObject *dict)
{
    if (dict) {
        setattrofunc f = self->ob_type->tp_setattro;
        if (!f) {
            f = PyObject_GenericSetAttr;
        }
        Py_ssize_t pos = 0;
        PyObject *key, *value;
        while (PyDict_Next(dict, &pos, &key, &value)) {
            f(self, key, value);
        }
    }
    return true;
}


/*! The generic constructor wrapper for Orange classes whose __new__
    accept arguments.

    If the class defines a static method \c __new__ that returns a pointer to
    TOrange, pyprops assigns to \c tp_new a function (classname)_newer that calls
    Orange_GenericNewWithKeywords; the call is in a try-except block that
    converts C++ exceptions to Python errors.

    Orange_GenericNewWithKeywords calls the __new__ defined in the class and
    gives it the type, the positional arguments and keyword arguments that are
    recognized by the constructor. No translation from obsolete and camel-cased
    names is needed since older version of Orange did not support keyword
    arguments. It casts the returned object to PyObject by calling
    #PyObject_FromNewOrange, sets the remaining attributes, and returns it.
    
    \param type The (Python) type for the new instance
    \param newer The \c __new__ method that will construct the object
    \param args Arguments passed to the __new__ method
    \param kw Keyword arguments; those recognized by the constructor are passed
            to __newer__, others are set as attributes
    \param keywords A list of attributes recognized by the constructor
*/
PyObject *Orange_GenericNewWithKeywords(
    PyTypeObject *type, newerfunc newer, PyObject *args,
    PyObject *kw, char *keywords[])
{
    PyObject *kw_constr, *kw_attrs;
    keywordsForConstructor(kw, keywords, kw_constr, kw_attrs);
    TOrange *orangeObj = newer(type, args, kw_constr);
    if (!orangeObj)
        return NULL;
    PyObject *obj = PyObject_FromNewOrange(orangeObj);
    setAttr_FromDict(obj, kw_attrs);
    Py_XDECREF(kw_constr);
    Py_XDECREF(kw_attrs);
    return obj;
}

/*! The generic constructor wrapper for Orange classes whose __new__ accepts
    no arguments, that is, if the class is marked with the #NEW_NOARGS macro.
    For such classes pyprops assigns to \c tp_new a function
    (classname)_new_without_args that constructs a new instance of the
    given type by calling the positional new operator. Then it calls
    Orange_GenericNewWithoutArgs that merely casts the instance into PyObject,
    sets the given attributes and returns the instance.
*/
PyObject *Orange_GenericNewWithoutArgs(TOrange *obj, PyObject *args, PyObject *kw)
{
    PyObject *asPy = PyObject_FromNewOrange(obj);
    if (args && PyTuple_GET_SIZE(args)) {
        PyErr_Format(PyExc_ValueError,
            "%s.__new__ takes no arguments",
            asPy->ob_type->tp_name);
        Py_DECREF(asPy);
        return NULL;
    }
    setAttr_FromDict(asPy, kw);
    return asPy;
}


/*! A generic constructor for Orange objects that can be constructed and called,
    that is, for classes that are marked by the macro #NEW_WITH_CALL.

    For such classes, pyprops fills the \c tp_new slot with a constructor defined
    by pyprops which constructs a new object by calling the (positional) new
    operator, and then calls Orange_GenericWithCall. The call is in a try-except
    block that converts exceptions raised in C++ to Python exceptions.

    Orange_GenericNewWithCall splits the keyword arguments into three groups:
    the built-in attributes of the given TOrange instance, the attributes
    recognized by the instance's \c __call__ operator and the rest. The built-in
    attributes (exposed either as members or via get-setters) are assigned first.
    If there are any attributes recognized by the \c __call__operator or any
    positional arguments (argument \c args), the \c __call__ operator is called.
    The remaining attributes are then assigned to the result of the call or, if
    the operator was not called, to the original instance (\c obj). The function
    then returns the result of the call or the original instance.

    \param obj A pointer to an instance of TOrange
    \param args Position arguments; if present, the obj's __call__ method is called
    \param kw Keyword arguments
    \param call_keywords Keywords recognized by the call operator
 */
PyObject *Orange_GenericNewWithCall(
    TOrange *obj, PyObject *args, PyObject *kw, char *call_keywords[])
{
    PyObject *self = PyObject_FromNewOrange(obj);
    GUARD(self);
    PyObject *kw_nonattrs1, *kw_call, *kw_attrs;
    setBuiltInAttributes(kw, self, kw_nonattrs1);
    GUARD(kw_nonattrs1);
    keywordsForConstructor(kw_nonattrs1, call_keywords, kw_call, kw_attrs);
    GUARD(kw_call);
    GUARD(kw_attrs);
    PyObject *res;
    if (args && PyTuple_GET_SIZE(args) || kw_call && PyDict_Size(kw_call)) {
        res = PyObject_Call(self, args, kw_call);
        if (!res) {
            return NULL;
        }
    }
    else {
        res = self;
        Py_INCREF(res);
    }
    setAttr_FromDict(res, kw_attrs);
    return res;
}


/*! A generic function for setting attribtues (\c __setattr__) that prevents
    storing the attributes into #TOrange::orange_dict. */
int Orange_GenericSetAttrNoDict(PyObject *self, PyObject *name, PyObject *value)
{
    PyObject *fake_dict = PyDict_New();
    int ok = _PyObject_GenericSetAttrWithDict(self, name, value, fake_dict);
    if (ok == -1)
        return -1;
    if (PyDict_Size(fake_dict)) {
        Py_DECREF(fake_dict);
        if (PyUnicode_Check(name)) {
            string cname = PyUnicode_As_string(name);
            PyErr_Format(PyExc_AttributeError,
                "'%s' object has no attribute '%s'",
                self->ob_type->tp_name, cname.c_str());
        }
        else {
            PyErr_Format(PyExc_AttributeError,
                "'%s' object has no such attribute",
                self->ob_type->tp_name);
        }
        return -1;
    }
    Py_DECREF(fake_dict);
    return 0;
}
