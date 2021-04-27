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


#ifndef __CONVERTS_HPP
#define __CONVERTS_HPP

using namespace std;

string PyUnicode_As_string(PyObject *);
string *PyUnicode_As_new_string(PyObject *);

#ifdef _MSC_VER
  #undef min
  #undef max
#endif

//WRAPPER(Contingency)
//WRAPPER(Distribution)

#define MEMBER(tpe, obj, ofs) (*(tpe *)(((char *)(obj)) + ofs))
#define CONST_MEMBER(tpe, obj, ofs) (*(tpe const *)(((char const *)(obj)) + ofs))

PyObject *getter_string(PyObject *self, int *offset);
int setter_string(PyObject *self, PyObject *value, int *offset);

bool convertFromPython(PyObject *, string &);
bool convertFromPython(PyObject *, double &);
bool convertFromPython(PyObject *, pair<double, double> &);
bool convertFromPython(PyObject *, int &);
bool convertFromPython(PyObject *, unsigned char &);
bool convertFromPython(PyObject *, bool &);
bool convertFromPython(PyObject *, vector<int> &);
//bool convertFromPython(PyObject *, PContingency &, bool allowNull=false, PyTypeObject *type=NULL);

PyObject *convertToPython(const string &);
PyObject *convertToPython(const double &);
PyObject *convertToPython(const pair<double, double> &);
PyObject *convertToPython(const int &);
PyObject *convertToPython(const unsigned char &);
PyObject *convertToPython(const bool &);

PyObject *convertToPython(const vector<int> &v);


//string convertToString(const PDistribution &);
string convertToString(const string &);
string convertToString(const double &);
string convertToString(const pair<double, double> &);
string convertToString(const int &);
string convertToString(const unsigned char &);
//string convertToString(const PContingency &);

class TOrangeType;
bool convertFromPythonWithML(PyObject *obj, string &str, const TOrangeType &base);

bool PyNumber_ToDouble(PyObject *o, double &);

extern const char *defMsg;
double PyNumber_AsDouble(PyObject *o, const char *msg = defMsg);

template<class T>
PyObject *convertToPython(const T &);

template<class T>
string convertToString(const T &);

int getBool(PyObject *args, void *isTrue);

inline PyObject *PyBool_FromBool(const bool b) {
    return PyBool_FromLong(b ? 1 : 0);
}

// This is defined by Python but then redefined by STLPort
#undef LONGLONG_MAX
#undef ULONGLONG_MAX

#if PY_VERSION_HEX >= 0x03020000
        #define SLICE_CAST reinterpret_cast<PyObject *>
#else
        #define SLICE_CAST reinterpret_cast<PySliceObject *>
#endif


bool convertFromPython(PyObject *, vector<double> &);
bool convertFromPython(PyObject *, map<double, double> &);

// Steals a reference and releases it upon destruction
class PyObjectWrapper {
	mutable PyObject *object;

public:
	PyObjectWrapper(PyObject *o)
		: object(o)
	{}

	PyObjectWrapper(PyObjectWrapper const &old)
		: object(old.object)
	{ Py_XINCREF(old.object); }

	PyObjectWrapper &operator =(PyObjectWrapper const &old)
	{
		Py_XINCREF(old.object);
		Py_XDECREF(object);
		object = old.object;
		return *this;
	}

	~PyObjectWrapper()
	{ Py_XDECREF(object); }

    void release()
    { object = NULL; }
};

#define GUARD(o) const PyObjectWrapper _Py_guardian_##o(o)

#endif
