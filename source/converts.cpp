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

bool convertFromPython(PyObject *obj, bool &b)
{ b = PyObject_IsTrue(obj) ? true : false;
  return true;
}


PyObject *convertToPython(const bool &b)
{ return PyLong_FromLong(b ? 1 : 0); }



bool convertFromPython(PyObject *obj, int &i)
{ 
    if (PyLong_Check(obj)) {
        i = (int)PyLong_AsLong(obj);
    }
    else if (PyLong_Check(obj)) {
        i  = (int)PyLong_AsLong(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "invalid integer (%s)", obj->ob_type->tp_name);
        return false;
    }
    return true;
}


PyObject *convertToPython(const int &i)
{ return PyLong_FromLong(i); }


string convertToString(const int &i)
{ char is[128];
  sprintf(is, "%d", i);
  return is; }



bool convertFromPython(PyObject *obj, long &i)
{ 
    if (PyLong_Check(obj)) {
        i = (long)PyLong_AsLong(obj);
    }
    else if (PyLong_Check(obj)) {
        i = (long)PyLong_AsLong(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "invalid integer (%s)", obj->ob_type->tp_name);
        return false;
    }
    return true;
}


PyObject *convertToPython(const long &i)
{ return PyLong_FromLong(i); }


string convertToString(const long &i)
{ char is[128];
  sprintf(is, "%d", int(i));
  return is; }


bool convertFromPython(PyObject *obj, unsigned char &i)
{
    if (PyLong_Check(obj)) {
        i = (unsigned char)PyLong_AsLong(obj);
    }
    else if (PyLong_Check(obj)) {
        i = (unsigned char)PyLong_AsLong(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "invalid integer (%s)", obj->ob_type->tp_name);
        return false;
    }
    return true;
}


PyObject *convertToPython(const unsigned char &i)
{ return PyLong_FromLong(i); }


string convertToString(const unsigned char &i)
{ char is[128];
  sprintf(is, "%d", i);
  return is; }


bool convertFromPython(PyObject *obj, double &i)
{ 
    if (PyFloat_Check(obj)) {
        i = (double)PyFloat_AsDouble(obj);
    }
    else if (PyLong_Check(obj)) {
        i = (double)PyFloat_AsDouble(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "invalid number (%s)", obj->ob_type->tp_name);
        return false;
    }
    return true;
}


PyObject *convertToPython(const double &i)
{ return PyFloat_FromDouble(i); }


string convertToString(const double &i)
{ char is[128];
  sprintf(is, "%f", i);
  return is; }


bool convertFromPython(PyObject *obj, pair<double, double> &i)
{ return PyArg_ParseTuple(obj, "dd:float_float", &i.first, &i.second) != 0; }


PyObject *convertToPython(const pair<double, double> &i)
{ return Py_BuildValue("dd", i.first, i.second); }


string convertToString(const pair<double, double> &i)
{ char is[128];
  sprintf(is, "(%5.3f, %5.3f)", i.first, i.second);
  return is; 
}


bool convertFromPython(PyObject *obj, pair<int, double> &i)
{ return PyArg_ParseTuple(obj, "id:int_float", &i.first, &i.second) != 0; }


PyObject *convertToPython(const pair<int, double> &i)
{ return Py_BuildValue("id", i.first, i.second); }


string convertToString(const pair<int, double> &i)
{ char is[128];
  sprintf(is, "(%i, %5.3f)", i.first, i.second);
  return is; 
}


bool convertFromPython(PyObject *obj, string &str)
{ 
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
            "invalid string (%s)", obj->ob_type->tp_name);
        return false;
    }
    str = PyUnicode_As_string(obj);
    return true;
}


PyObject *getter_string(PyObject *self, int *offset)
{
    return PyUnicode_FromString(MEMBER(string, self, *offset).c_str());
}

int setter_string(PyObject *self, PyObject *value, int *offset)
{
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
            "expected a string, not an instance of '%s'",
            value->ob_type->tp_name);
        return -1;
    }
    MEMBER(string, self, *offset) = PyUnicode_As_string(value);
    return 0;
}


PyObject *convertToPython(const string &str)
{ return PyUnicode_FromString(str.c_str()); }


string convertToString(const string &str)
{ return str; }


bool convertFromPython(PyObject *obj, vector<double> &v)
{
    PyObject *iter = PyObject_GetIter(obj);
    if (!iter) {
        return false;
    }
    GUARD(iter);
    v.clear();
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
        if (!PyNumber_Check(item)) {
            v.clear();
            Py_DECREF(item);
            return false;
        }
        v.push_back(PyNumber_AsDouble(item));
        Py_DECREF(item);
    }
    return true;
}

bool convertFromPython(PyObject *obj, vector<int> &v)
{
    PyObject *iter = PyObject_GetIter(obj);
    if (!iter) {
        return false;
    }
    GUARD(iter);
    v.clear();
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
        if (!PyLong_Check(item)) {
            v.clear();
            Py_DECREF(item);
            return false;
        }
        v.push_back(PyLong_AsLong(item));
        Py_DECREF(item);
    }
    return true;
}

bool convertFromPython(PyObject *obj, map<double, double> &v)
{
    if (PyDict_Check(obj)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        v.clear();

        while (PyDict_Next(obj, &pos, &key, &value)) {
            if (!(PyNumber_Check(key) && PyNumber_Check(value))) {
                v.clear();
                return false;
            }
            v[PyNumber_AsDouble(key)] = PyNumber_AsDouble(value);
        }
        return true;
    }

    PyObject *iter = PyObject_GetIter(obj);
    if (!iter) {
        return false;
    }
    GUARD(iter);
    for(PyObject *item = PyIter_Next(iter); item; item = PyIter_Next(iter)) {
        double key, value;
        if (!PyArg_ParseTuple(item, "dd:double_double", &key, &value)) {
            v.clear();
            Py_DECREF(item);
            return false;
        }
        v[key] = v[value];
    }
    return true;
}


bool PyNumber_ToDouble(PyObject *o, double &res)
{ 
    if (!PyNumber_Check(o)) {
        return false;
    }
    PyObject *number=PyNumber_Float(o);
    if (!number) {
        PyErr_Clear();
        return false;
    }
    res = PyFloat_AsDouble(number);
    Py_DECREF(number);
    return true;
}


const char *defMsg = "expected a number, not '%s'";

double PyNumber_AsDouble(PyObject *o, const char *errmsg)
{ 
    if (!PyNumber_Check(o)) {
         raiseError(PyExc_TypeError, errmsg, o->ob_type->tp_name);
    }
    PyObject *number=PyNumber_Float(o);
    GUARD(number);
    return PyFloat_AsDouble(number);
}



PyObject *convertToPython(const vector<int> &v)
{
  const int e = v.size();
  PyObject *res = PyList_New(e);
  vector<int>::const_iterator vi(v.begin());
  for(int i = 0; i<e; i++, vi++)
    PyList_SetItem(res, i, PyLong_FromLong(*vi));
  return res;
}


int getBool(PyObject *arg, void *isTrue)
{ 
  int it = PyObject_IsTrue(arg);
  if (it == -1)
    return 0;

  *(bool *)isTrue = it != 0;
  return 1;
}

string PyUnicode_As_string(PyObject *u)
{
	PyObject *bytes = PyUnicode_EncodeUTF8(
        PyUnicode_AS_UNICODE(u), PyUnicode_GET_SIZE(u), "ignore");
	string s(PyBytes_AS_STRING(bytes));
	Py_DECREF(bytes);
	return s;
}

string *PyUnicode_As_new_string(PyObject *u)
{
	PyObject *bytes = PyUnicode_EncodeUTF8(
        PyUnicode_AS_UNICODE(u), PyUnicode_GET_SIZE(u), "ignore");
	string *s = new string(PyBytes_AS_STRING(bytes));
	Py_DECREF(bytes);
	return s;
}
