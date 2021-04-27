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


#ifndef __METACHAIN_HPP
#define __METACHAIN_HPP

typedef int TMetaId;

class TMetaValue {
public:
    bool isPrimitive;
    TMetaId id;
    union {
        TValue value;
        PyObject *object; // borrowed object!
    };

    inline TMetaValue(TMetaId const id, TValue const);
    inline TMetaValue(TMetaId const id, PyObject *);
    inline TMetaValue(TMetaValue const &);
    inline TMetaValue &operator =(TMetaValue const &);

    inline bool isDefined() const;
};

namespace MetaChain {
    inline int advance(int const handle);

    inline void add(int &handle, TMetaId const id, TValue const value);
    inline void add(int &handle, TMetaId const id, PyObject *const object);
    inline void add(int &handle, TMetaValue const &metaValue);
//    void add(int &handle, int const sourceHandle);

    inline void set(int &handle, TMetaId const id, TValue const value);
    inline void set(int &handle, TMetaId const id, PyObject *const object);
    inline void set(int &handle, TMetaValue const &metaValue);
    inline void set(int &handle, int const sourceHandle);

    inline bool has(int handle, TMetaId const id);

    inline TMetaValue get(int handle);
    inline TMetaValue get(int handle, TMetaId const id, bool throwExc = true);
    inline TValue *getDoublePtr(int handle, TMetaId const id);
    inline TValue &getDouble(int handle, TMetaId const id);
    inline TValue &setDefault(int &handle, TMetaId const id, TValue const defaultVal);
    inline PyObject **getObjectPtr(int handle, TMetaId const id);
    inline PyObject *&getObject(int handle, TMetaId const id);

    inline void remove(int &handle, TMetaId id, bool throwExc = true);

    void freeChain(int &handle);
    void copyChain(int &handle, int source);
    void insertChain(int &handle, int source);
    inline void killChain(int &handle);

    PyObject *packChain(int handle, PDomain const &);
    void unpackChain(PyObject *chain, int &handle, PDomain const &);

} // end of namespace


// inline functions
#include "metachain-private.hpp"

#endif

