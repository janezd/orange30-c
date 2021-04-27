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
#include "randomgenerator.px"

OrRandomGenerator py_globalRandom = {
    PyObject_HEAD_INIT(&OrRandomGenerator_Type)
    TRandomGenerator()
};

TRandomGenerator *globalRandom = &py_globalRandom.orange;

static char *RandomGenerator_keywords_4[] = {"seed", "mtstate", "mtofs", "mtleft", NULL};

TOrange *TRandomGenerator::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    try {
        int seed = 0;
        char *mtstate = NULL;
        int statesize;
        int offs;
        int left;
        if (!PyArg_ParseTupleAndKeywords(args, kw,
            "|iy#ii:RandomGenerator", RandomGenerator_keywords_4, &seed,
            &mtstate, &statesize, &offs, &left)) {
            return NULL;
        }
        TRandomGenerator *rg = new (type)TRandomGenerator(seed);
        // unpickling
        if (mtstate) {
            cMersenneTwister &mt = rg->mt;
            memcpy(mt.state, mtstate, statesize);
            mt.next = mt.state + offs;
            mt.left = left;
        }
        return rg;
    }
    PyCATCH
}

PyObject *TRandomGenerator::__getnewargs__() const
{
  return Py_BuildValue("(iy#ii)", initseed,
                               mt.state, 625 * sizeof(long),
                               mt.next - mt.state,
                               mt.left);
}

PyObject *TRandomGenerator::py_reset(PyObject *args, PyObject *kw)
{ 
    if (   (args && PyTuple_Size(args) || kw && PyDict_Size(kw))
        && !PyArg_ParseTupleAndKeywords(args, kw, "i:reset",
                 RandomGenerator_reset_keywords, &initseed)) {
            return NULL;
    }
    reset();
    Py_RETURN_NONE;
}

PyObject *TRandomGenerator::__call__(PyObject *args, PyObject *kw)
{
    NO_ARGS
    return PyLong_FromLong((*this)());
}
