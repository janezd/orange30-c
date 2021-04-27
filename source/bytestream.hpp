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


#ifndef __BYTESTREAM_HPP
#define __BYTESTREAM_HPP

#include <Python.h>
#include <vector>

/// Buffer used for storing/retrieving binary data when picling.
class TByteStream {
public:
    /// Beginning of the buffer
    char *buf;
    
    /*! The end of currently allocated buffer; if NULL, the data is not
        owned by this object and can't be reallocated or freed. */
    char *bufe;

    /// The current writing or reading point
    char *bufptr;

    inline TByteStream(const int size=0);
    inline TByteStream(char *abuf);
    inline TByteStream(TByteStream &other);
    inline TByteStream &operator =(TByteStream &other);
    inline TByteStream &operator =(PyObject *bytestream);
    inline ~TByteStream();
    inline Py_ssize_t length() const;
    inline void ensure(const Py_ssize_t &size);

    template<class T> inline void write(T const &c);
    template<class T> void write(const std::vector<T> &v);

    inline void writeBuf(const void *abuf, size_t size);

    template<class T> inline void read(T &c);
    template<class T> inline void readVector(std::vector<T> &v);
    inline void readBuf(void *abuf, size_t size);

    inline char readChar();
    inline unsigned short readShort();
    inline int readInt();
    inline long readLong();
    inline float readFloat();
    inline double readDouble();
    inline size_t readSizeT();
    static int argconverter(PyObject *, void *);
};


/*! Constructs the object and allocates the buffer of a given size,
    or none if size is 0. */
TByteStream::TByteStream(const int size)
{
    if (size) {
        buf = bufptr = (char *)malloc(size);
        bufe = buf + size;
    }
    else {
        buf = bufptr = bufe = NULL;
    }
}


/// Constructor from borrowed data; sets #bufe to \c NULL
TByteStream::TByteStream(char *abuf)
: buf(abuf),
bufe(NULL),
bufptr(abuf)
{}


/*! Copy constructor; steals the data from the other buffer by setting
    its pointers to \c NULL. */
TByteStream::TByteStream(TByteStream &other)
: buf(other.buf),
bufe(other.bufe),
bufptr(other.bufptr)
{
    other.buf = other.bufe = other.bufptr = NULL;
}


/*! Copy operator; steals the data from the other buffer by setting
    its pointers to \c NULL. */
TByteStream &TByteStream::operator =(TByteStream &other)
{
    if (this == &other) {
        return *this;
    }
    if (buf && bufe) {
        free(buf);
    }
    buf = other.buf;
    bufe = other.bufe;
    bufptr = other.bufptr;
    other.buf = other.bufe = other.bufptr = NULL;
    return *this;
}


/*! Copy operator from Python object; the data is copied.
    \todo
    Would it decrease the memory usage if TByteStream just referenced the
    data in the PyObject and kept a reference to it to prevent destruction?
    This wouldn't be unacceptably unsafe since these buffers are constructed
    at unpickling and are cannot changed by an evil user.
    */
TByteStream &TByteStream::operator =(PyObject *obj)
{
    if (buf && bufe) {
        free(buf);
    }
    const size_t sze = PyBytes_Size(obj);
    buf = bufptr = (char *)malloc(sze);
    memcpy(buf, PyBytes_AsString(obj), sze);
    bufe = buf + sze;
    return *this;
}


/*! Frees the buffer if it is owned by the object */
TByteStream::~TByteStream()
{
    if (buf && bufe) {// if there's no bufe, we don't own the buffer
        free(buf);
    }
}

/*! The buffer length (valid only for writing; return #bufptr-#buf). */
Py_ssize_t TByteStream::length() const
{
    return bufptr - buf;
}


/*! Ensures that the buffer has enough space for at least \c size bytes. */
void TByteStream::ensure(const Py_ssize_t &size)
{ 
    if (!buf) {
        Py_ssize_t rsize = size > 1024 ? size : 1024;
        buf = bufptr = (char *)malloc(rsize);
        bufe = buf + rsize;
    }
    else {
        if (!bufe) {
            raiseError(PyExc_SystemError,
                "cannot resize a ByteStream buffer that does not own its data");
        }
        if (bufe - bufptr < size) {
            int tsize = bufe - buf;
            tsize = tsize < 65536 ? tsize << 1 : tsize + 65536;
            const int tpos = bufptr - buf;
            buf = (char *)realloc(buf, tsize);
            bufe = buf + tsize;
            bufptr = buf + tpos;
        }
    }
}


/*! Writes an object to the buffer */
template<class T>
void TByteStream::write(T const &c)
{
    ensure(sizeof(T));
    memcpy((void *) bufptr, (void *) &c, sizeof(T));
    bufptr += sizeof(T);
}


/*! Writes a vector to the buffer */
template<class T>
void TByteStream::write(const std::vector<T> &v)
{
    size_t size = v.size();
    ensure(size*sizeof(T) + sizeof(size_t));
    write(size);
    if (size > 0) {
        // This is legal as &v[0] is guaranteed to point to a
        // contiguous memory block
        memcpy((void *)bufptr, (void *)&v[0], size*sizeof(T));
        bufptr += size*sizeof(T);
    }
}


/*! Writes a block of memory to the buffer */
void TByteStream::writeBuf(const void *abuf, size_t size)
{
    ensure(size);
    memcpy(bufptr, abuf, size);
    bufptr += size;
}


/*! Reads an object from the buffer */
template<class T>
void TByteStream::read(T &c)
{
    c = *((T *&)bufptr)++;
}


/*! Reads a vector from the buffer */
template<class T>
void TByteStream::readVector(std::vector<T> &v)
{
    size_t size = *((size_t *&)bufptr)++;
    v.resize(size);
    memcpy((void *)&v[0], (void *)bufptr, size*sizeof(T));
    bufptr += size*sizeof(T);
}


/*! Reads a block of memory from the buffer */
void TByteStream::readBuf(void *abuf, size_t size)
{
    memcpy(abuf, bufptr, size);
    bufptr += size;
}


/*! Reads and returns a \c char from the buffer */
char TByteStream::readChar()
{ 
    return *bufptr++;
}

/*! Reads and returns a \c short from the buffer */
unsigned short TByteStream::readShort()
{
    return *((unsigned short *&)bufptr)++;
}


/*! Reads and returns an \c int from the buffer */
int TByteStream::readInt()
{
    return *((int *&)bufptr)++;
}


/*! Reads and returns a \c long from the buffer */
long TByteStream::readLong()
{
    return *((long *&)bufptr)++;
}

/*! Reads and returns a \c float from the buffer */
float TByteStream::readFloat()
{
    return *((float *&)bufptr)++;
}

/*! Reads and returns a \c double from the buffer */
double TByteStream::readDouble()
{
    return *((double *&)bufptr)++;
}


/*! Reads and returns a \c size_t from the buffer */
inline size_t TByteStream::readSizeT()
{
    return *((size_t *&)bufptr)++;
}

#endif
