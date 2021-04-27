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

/*! \file

    Macros used for marking code for pyprops. Most macros, except for
    #__REGISTER_ABSTRACT_CLASS, #__REGISTER_CLASS and #PYVECTOR contain
    no code.
*/

#ifndef __PYXTRACT_MACROS_HPP
#define __PYXTRACT_MACROS_HPP

/*! Marks an abstract T-class. The macro also declares the Or-class as a
    friend of this class. */
#define __REGISTER_ABSTRACT_CLASS(x) \
    friend class Or##x; \

/*! Marks a class pyprops. The macro also defines the positional and
    non-positional operators new and delete and declares methods
    \c clone, \c __traverse__, \c __clear__ and \c __dealloc. The definitions
    for these methods are provided by prprops and stored in the px file. */
#define __REGISTER_CLASS(x) \
    __REGISTER_ABSTRACT_CLASS(x) \
    static inline void *operator new(size_t size) { return &PyObject_GC_New(OrOrange, &Or##x##_Type)->orange; } \
    static inline void *operator new(size_t size, PyTypeObject *type) { \
        TOrange *orange = &PyObject_GC_New(OrOrange, type)->orange;  \
        memset(orange, 0, type->tp_basicsize - offsetof(OrOrange, orange)); \
        if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) Py_INCREF(type); \
        return orange; } \
    static inline void operator delete(void *obj) { TOrange::operator delete(obj); } \
    static inline void operator delete(void *obj, PyTypeObject *) { TOrange::operator delete(obj); } \
\
    virtual T##x *clone() const; \
    static int __traverse__(Or##x *self, visitproc visit, void *args); \
    static int __clear__(Or##x *self); \
    static void __dealloc__(Or##x *); 

/*! Declare a PYVECTOR to pyprops and also defines the corresponding P-class. */
#define PYVECTOR(classname, wrapped) \
class P##classname : public POrange \
{ \
public: \
    inline P##classname() {}; \
    inline explicit P##classname(T##classname *const o) : POrange(o) {} \
    inline static P##classname fromBorrowedPtr(T##classname *const o) { return (P##classname)POrange::fromBorrowedPtr(o); } \
    inline explicit P##classname(PyObject *o) : POrange(o) {} \
    inline P##classname(P##classname const &o) : POrange(o) {} \
    explicit P##classname(POrange const &o); \
\
    inline T##classname *borrowPtr() const { return (T##classname *)orange; } \
    inline T##classname *getPtr() const { incref(); return (T##classname *)orange; } \
    inline T##classname &operator *() const { checkNull(); return *(T##classname *)orange; }; \
    inline T##classname *operator ->() const { checkNull(); return (T##classname *)orange; }; \
\
    static int argconverter(PyObject *obj, P##classname *addr); \
    static int argconverter_n(PyObject *obj, P##classname *addr); \
    static int setter(PyObject *whom, PyObject *mine, size_t *offset); \
};

#define PYFUNCTION(pyname,cname,args,doc)
#define PYCONSTANT_INT(pyname,ccode)
#define PYCONSTANT_FLOAT(pyname,ccode)
#define PYCONSTANTFUNC(pyname,cname)

#define PYMODULECONSTANT(pyname,ccode)
#define PYMODULEFUNCTION

#define PYCLASSCONSTANT_INT(constname, intconst)
#define PYCLASSCONSTANT_FLOAT(constname, intconst)
#define PYCLASSCONSTANT(constname, oconst)

#define PYCLASSCONSTANTS
#define PYCLASSCONSTANTS_UP

#define PYARGS(x,doc)
#define PYDOC(x)

#define PICKLING_ARGS(x)



#define METH_BOTH METH_VARARGS | METH_KEYWORDS

#define PYXTRACT_IGNORE
#define NOPYPROPS

/*! pyprops provides classes that are marked with this (empty) macro with a
    constructor that creates an instance and calls it if the caller provides
    appropriate arguments. See documentation of #Orange_GenericNewWithCall
    for details. 
    
    The macro itself is empty. */
#define NEW_WITH_CALL(callBase)

/*! pyprops provides classes that are marked with this (empty) macro with a
    constructor that constructs a new instance using the class' new operator.
    See documentation on #Orange_GenericNewWithoutArgs for details. */
#define NEW_NOARGS

#define NO_PICKLE(type)

#endif
