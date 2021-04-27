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


#ifndef __ORANGE_HPP
#define __ORANGE_HPP

// Do not include it! We define it here below!
// #include "orange.ppp"

using namespace std;

/*! \file

    Provides the definition of the base T-class TOrange, the base P-class
    POrange, the base Or-class OrOrange and several macros and global
    functions. */
    
/*! Raises an exception if either tuple \c args or dictionary \c kw is non-NULL
    and non-empty. Used by functions that implement Python constructors or
    call operators but receive no arguments. */
#define NO_ARGS { \
    if (args && PyTuple_Size(args) || kw && PyDict_Size(kw)) \
        return PyErr_Format(PyExc_AttributeError, "this function accepts no arguments"); \
    }

PyObject *getter_orange(PyObject *self, size_t *offset);

class OrOrange;
class POrange;

/// @cond Python
extern PyTypeObject OrOrange_Type;
/// @endcond Python


/*! The root of the T-class hierarchy. Besides the method #clone and circular
    reference resolving, the class provides mostly methods related to Python
    interface, in particular setting and getting attributes and pickling.

    Each T-class has a corresponding P-class and Or-class. P-classes have a
    hierarchy derived from POrange, which represent referencing-counting wrappers
    for pointers to T-objects. Or-classes are PyObjects; they do not have a
    hierarchy but are binary compatible with OrOrange class (that is, they are
    derived in a C-like fashion, like PyObjects).

    Instances of TOrange should always be created on the heap, using the
    #operator new, and never on the stack. The #operator new allocates the memory
    for PyObject head, so the object may be passed to Python as PyObject. Even
    if the object will not be exposed to Python, the head is needed for garbage
    collection; objects without head may cause the circular garbage collection
    to crash.

    Pointers to TOrange can be used locally within functions, but should not be
    stored, passed as arguments or returned as results. C++ code should use
    POrange and the derived classes instead, while Or-objects should be used for
    returning Orange instances to Python.
*/
class TOrange {
public:
    friend class POrange;
    friend PyObject *PyObject_FromNewOrange(TOrange *orange);

    __REGISTER_ABSTRACT_CLASS(Orange);
    /*! Dictionary for storing non-built-in attributes. */
    PyObject *orange_dict; /* PYXTRACT: dict_field */

    static void *operator new(size_t ssize, PyTypeObject *type = &OrOrange_Type) throw();
    static void operator delete(void *obj, PyTypeObject *);
    virtual ~TOrange();
    virtual TOrange *clone() const;
protected:
    static void operator delete(void *obj);
    TOrange();

    inline static void start_gc_tracking(TOrange *const );
    virtual int traverse_references(visitproc proc, void *arg);
    virtual int clear_references();

public:
    inline static TOrange *setterConversion(PyObject *);

    static PyObject *__repr__(PyObject *);
    long __hash__() const;
    static PyObject *__newobj__(PyObject *cls, PyObject *args) PYARGS(METH_VARARGS, "unpickle helper function");

    static PyObject *__getattr__(PyObject *self, PyObject *name);
    static int __setattr__(PyObject *self, PyObject *name, PyObject *value);
    static PyObject *__getstate__(PyObject *self) PYARGS(METH_NOARGS, "(); packs the object's attribute in a dictionary for pickling");
    static PyObject *__setstate__(PyObject *self, PyObject *dict) PYARGS(METH_O, "(); unpacks the dictionary into objects attributes");
};


/*! OrOrange is a class with the PyObject_HEAD in the beginning of the structure,
    so it can be used as PyObject. The object contains an instance of TOrange
    (not a pointer to it, but actual data). The #TOrange::operator new is
    overload to allocate memory for OrOrange overhead, so all (dynamically
    allocated) TOrange objects are also OrOrange objects.

    OrOrange is used for passing Orange objects to and from Python.
*/
class OrOrange {
public:
    PyObject_HEAD;
    TOrange orange; ///< An instance (not a pointer!) of Orange object
};


/*! POrange is a smart pointer that wraps #TOrange. Every class in Orange
 has its corresponding wrapper class arranged in the same hierarchy. For
 example, #TBayesLearner is derived from #TLearner which is in turn derived
 from #TOrange. The wrapper of \c TBayesLearner, \c PBayesLearner is derived
 from \c PLearner which is derived from \c POrange. The shadow hierarchy of
 smart pointers is implemented so that smart pointers from derived classes
 can be passed as arguments; e.g. a function that expects <tt>PLearner const
 &</tt> can be given <tt>PBayesLearner const &</tt> without explicit
 (re)casting.

We refer to wrappers as P-classes, as opposed to the "true" T-classes
(e.g. #TBayesLearner) and Py-classes (e.g. \c OrBayesLearner) that are
<tt>PyObject</tt>s.
 
P-classes are created automatically by pyprops and cannot be modified or extended manually.

Instances of P-classes can be created by copying P-classes or wrapping
T-classes or Py-classes. By wrapping a pointer to T-class, the P-class takes
ownership of reference. Pointers that are not owned should not be wrapped,
except by using named constructor #fromBorrowedPtr. Constructors that take
pointers to T-classes and PyObjects are explicit to prevent accidental
wrapping.

Since P-classes are semantically pointers, dereferencing them gives
(references to) T-classes. P-classes can give pointers to corresponding
instances of T-class or Py-class; these pointers can be given as borrowed
references (#borrowPtr, #borrowPyObject) or with a new reference
(#getPtr, #getPyObject, #toPython). Be careful when dealing with borrowed
references: they should be used only locally and never rewrapped, except by
#fromBorrowedPtr).
*/
class POrange {
public:
    mutable TOrange *orange; /*!< A pointer to the corresponding T-instance */
public:
    inline POrange();
    inline explicit POrange(TOrange *const o);
    inline static POrange fromBorrowedPtr(TOrange *const o);
    inline explicit POrange(PyObject *o);
    inline POrange(POrange const &old);
    inline ~POrange();
    inline void clear();

    inline void incref() const;
    inline void decref() const;

    inline POrange &operator =(POrange const &old);
    inline operator bool() const;
    inline bool operator == (POrange const &other) const;
    inline bool operator != (POrange const &other) const;
    inline bool operator == (TOrange const *const other) const;
    inline bool operator != (TOrange const *const other) const;

    inline PyObject *borrowPyObject() const;
    inline PyObject *getPyObject() const;
    inline PyObject *toPython() const;
    inline TOrange *borrowPtr() const;
    inline TOrange *getPtr() const;

    inline void checkNull() const;
    inline TOrange &operator *() const;
    inline TOrange *operator ->() const;

    /* The following functions raise exceptions; defined just for
       documentation */
    inline static int argconverter(PyObject *, POrange *);
    inline static int argconverter_n(PyObject *, POrange *);
};


/// Checks whether the object is an instance of TOrange (or a derived type)
#define OrOrange_Check(op) PyObject_TypeCheck(op, &OrOrange_Type)


/*! Casts TOrange * to PyObject; returns a borrowed reference.
    This macro should be used only by TOrange,
    all others should call PyObject_FromOrange. */
#define AS_PyObject(x) (x ? (PyObject *)(((char *)x) - offsetof(OrOrange, orange)) : NULL)

/// Casts this (which should be a pointer to TOrange) to a pointer to PyObject.
#define THIS_AS_PyObject AS_PyObject(this)

/// Gets a pointer to PyTypeObject for \c this
#define OB_TYPE THIS_AS_PyObject->ob_type

/// Gets the type's name (as presented to Python) for \c this
#define MY_NAME THIS_AS_PyObject->ob_type->tp_name

/// Returns a clone of the given object, cast to the given P-class
#define CLONE(tpe,o) ((o) ? (tpe)POrange((o)->clone()) : tpe())

PyObject *PyObject_FromNewOrange(TOrange *orange);

/*! Returns a PyObject (Or-class) corresponding to the given instance of TOrange,
    or \c None if the object is NULL. Returns a new reference. */
inline PyObject *PyObject_FromOrange(TOrange *orange)
{
    PyObject *res = orange ? AS_PyObject(orange) : Py_None;
    Py_INCREF(res);
    return res;
}

/// Raises an exception if the given field of \c this is NULL
#define checkProperty(name) \
{ if (!name) raiseError(PyExc_ValueError, "'%s."#name"' not set", MY_NAME); }


bool setAttr_FromDict(PyObject *self, PyObject *dict);
int Orange_GenericSetAttrNoDict(PyObject *self, PyObject *name, PyObject *value);

typedef TOrange *newerfunc(PyTypeObject *, PyObject *, PyObject *);

PyObject *Orange_GenericNewWithKeywords(PyTypeObject *type, newerfunc newer, PyObject *args, PyObject *kw, char *keywords[]);
PyObject *Orange_GenericNewWithoutArgs(TOrange *obj, PyObject *args, PyObject *kw);
PyObject *Orange_GenericNewWithCall(TOrange *obj, PyObject *args, PyObject *kw, char *call_keywords[]);

/*! A virtual method through which a class can provide a class-specific
    conversion for setters and for argument conversion. The method should
    return \c NULL if it cannot convert the object. */
TOrange *TOrange::setterConversion(PyObject *)
{ return NULL; }


/*! Adds the TOrange * to Python's garbage collection tracking. The method
    is called when the object is wrapped into a P-class using
    POrange::POrange(TOrange *const) or cast to PyObject using
    PyObject_FromNewOrange.
*/
void TOrange::start_gc_tracking(TOrange *orange)
{
    PyObject *const &self = AS_PyObject(orange);
    if (!_PyObject_GC_IS_TRACKED(self)) {
        PyObject_GC_Track(self);
    }
}


/*! Create a NULL pointer */
POrange::POrange()
: orange(NULL)
{}


/*POrange::POrange(int const i)
: orange(NULL)
{ 
    if (i) {
        raiseError(PyExc_SystemError, "cannot cast int to POrange");
    }
}*/

/*! Wrap a PyObject into a P-instance. The object can be NULL. The
    constructor does not steal the reference; it increases the reference
    count of the object.
*/
POrange::POrange(PyObject *o)
: orange(o ? &((OrOrange *)o)->orange : NULL)
{
    incref();
}

/*! Wrap a T-instance into P-instance. The method takes ownership of the
    object. The caller must own the reference. The method also enables
    Python's garbage collection tracking of the reference. */
POrange::POrange(TOrange *const o)
: orange(o)
{
    if (o) {
        TOrange::start_gc_tracking(orange);
    }
}

/*! A named constructor that wraps a T-instance without stealing the
    reference; the method increases the reference count of the corresponding
    Py-object. */
POrange POrange::fromBorrowedPtr(TOrange *const o)
{
    POrange t(o);
    t.incref();
    return t;
}

/*! Copy constructor. Derived classes define this same constructor, e.g. a
    constructor that accepts an instance of \c POrange, but check (using
    dynamic casting) the type of the argument. An exception is raised in
    case of mismatch. */
POrange::POrange(POrange const &old)
: orange(old.orange)
{ 
    incref();
}

/*! Copy operator. */
POrange &POrange::operator =(POrange const &old)
{
    old.incref();
    decref();
    orange = old.orange;
    return *this;
}


/*! Destructor; decreases the reference count of the corresponding
    \c PyObject. */
POrange::~POrange()
{ 
    decref();
}


/*! Increase the reference count of the corresponding \c PyObject */
void POrange::incref() const
{
    Py_XINCREF(AS_PyObject(orange));
}


/*! Decrease the reference count of the corresponding \c PyObject */
void POrange::decref() const
{
    Py_XDECREF(AS_PyObject(orange));
}


/*! Make the object a NULL pointer: decreases the reference count and sets
    #orange to NULL. */
void POrange::clear()
{
    decref();
    orange = NULL;
}


/*! Return \c true is pointer is not \c NULL. */
POrange::operator bool() const
{
    return orange != NULL;
}


/*! Compare two smart pointers; \c true if they refer to the same object. */
bool POrange::operator == (POrange const &other) const
{
    return orange == other.orange;
}


/*! Compare two smart pointers; \c true if they refer to different
    objects. */
bool POrange::operator != (POrange const &other) const
{
    return orange != other.orange;
}


/*! Compare a smart pointer with an ordinary pointer; \c true if they refer
    to the same object. */
bool POrange::operator == (TOrange const *const other) const
{
    return orange == other;
}


/*! Compare a smart pointer with an ordinary pointer; \c true if they refer
    to different objects. */
bool POrange::operator != (TOrange const *const other) const
{
    return orange != other;
}


/*! Return a borrowed reference to the corresponding \c PyObject or \c NULL
    if <tt>#orange=NULL</tt>. */
PyObject *POrange::borrowPyObject() const
{
    return AS_PyObject(orange);
}


/*! Return a new reference to the corresponding \c PyObject or \c NULL if
    the pointer is \c NULL. */
PyObject *POrange::getPyObject() const
{
    incref();
    return AS_PyObject(orange);
}

/*! Return a new reference to the corresponding \c PyObject or \c None if
    the pointer is \c NULL. */
PyObject *POrange::toPython() const
{
    if (!orange) {
        Py_RETURN_NONE;
    }
    return getPyObject();
}


/*! Return a pointer to the corresponding T-object. The caller owns a
    reference to the pointer so the pointer needs to be wrapped to avoid
    memory leaks. This method should be avoided if possible. */
TOrange *POrange::getPtr() const
{
    incref();
    return orange;
}


/*! Return a borrowed pointer to the corresponding T-object. This returned
    pointer should not be rewrapped, except by #POrange::fromBorrowedPtr. It
    should never be passed as argument. Use carefully. */
TOrange *POrange::borrowPtr() const
{
    return orange;
}


/*! Raise an exception if the pointer is \c NULL. The method is called to
    guard dereferencing of objects. */
void POrange::checkNull() const
{
    if (!orange) {
        raiseError(PyExc_ValueError, "null pointer");
    }
}


/*! Derefrencing operator. Raises exception if the pointer is \c NULL. */
TOrange &POrange::operator *() const
{
    checkNull();
    return *orange;
}


/*! Derefrencing operator. Raises exception if the pointer is \c NULL. */
TOrange *POrange::operator ->() const
{
    checkNull();
    return orange;
}



/*! Converter from <tt>PyObject *</tt> to <tt>POrange</tt> for use in \c
    PyArg_ParseTuple and similar functions. Example:
    \code
    PDomain dom;
    char foo;
    if (!PyArg_ParseTuple(args, "O&c",
            &PDomain::argconverter, &dom, &c)) {
        return NULL;
    }
    \endcode
    Note: for \c POrange, the function raises an exception. 
*/
int POrange::argconverter(PyObject *, POrange *)
{
    raiseError(PyExc_SystemError,
        "cannot convert to instances of abstract object");
    return 0;
}

/*! Similar to #argconverter, except that it also converts \c None to
    null-pointers. */
int POrange::argconverter_n(PyObject *, POrange *)
{
    raiseError(PyExc_SystemError,
        "cannot convert to instances of abstract object");
    return 0;
}

#endif
