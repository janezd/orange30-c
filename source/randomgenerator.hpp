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


#ifndef __RANDOMGENERATOR_HPP
#define __RANDOMGENERATOR_HPP

#include "cMersenneTwister.h"
#include "randomgenerator.ppp"

class cMersenneTwister;

class TRandomGenerator : public TOrange {
public:
    __REGISTER_CLASS(RandomGenerator)

    int initseed; //P initial random seed
    int uses;     //P #times a number was returned

    cMersenneTwister mt;

    TRandomGenerator(const int &aninitseed=0)
      : initseed(aninitseed),
        uses(0),
        mt((unsigned long)aninitseed << 1)  // multiply by 2 since the generator sets the 0th bit
      {}

    virtual void reset()      
      { uses=0; 
        mt.Init((unsigned long)initseed << 1); }   // multiply by 2 since the generator sets the 0th bit

    inline unsigned long operator()() 
      { uses++;
        return mt.Random(); }


    #define crand ( (unsigned long)operator()() )
    #define irand ( (unsigned int)operator()() )

    int randbool(const unsigned int &y=2)
    { return (irand % y) == 0; }

    inline int randint()
    { return int(irand >> 1); }

    inline int randint(const unsigned int &y)
    { return int(irand % y); }

    inline int randint(const int &x, const int &y)
    { return int(irand % ((unsigned int)(y-x+1)) + x); }

    inline long randlong()
    { return long(crand >> 1); }

    inline long randlong(const unsigned long y)
    { return crand % y; }

    inline long randlong(const long &x, const long &y)
    { return crand % ((unsigned long)(y-x+1)) + x; }

    inline double randdouble(const double &x, const double &y)
    { return double(crand & 0x7fffffff)/double(0x7fffffff)*(y-x) + x; }

    inline float randfloat(const double &x, const double &y)
    { return float(randdouble(x, y)); }

    inline double operator()(const double &x, const double &y)
    { return randdouble(x, y); }

    inline double randdouble(const double &y=1.0)
    { return double(crand & 0x7fffffff)/double(0x7fffffff)*y; }

    inline float randfloat(const float &y=1.0)
    { return float(randdouble(y)); }

    static TOrange *__new__(PyTypeObject *type, PyObject *, PyObject *) PYDOC("([seed])");
    PyObject *__getnewargs__() const PYARGS(METH_NOARGS, "(); prepare arguments for unpickling");
    PyObject *__call__(PyObject *args, PyObject *kw) PYDOC("() -> 32-bit random int");
    PyObject *py_reset(PyObject *args, PyObject *kw) PYARGS(METH_BOTH, "([seed]) -> None");
};


// Same as or_random_shuffle, but uses TRandomGenerator
template<typename RandomAccessIter>
void rg_random_shuffle(RandomAccessIter first, RandomAccessIter last, TRandomGenerator &rand)
{
  if (first == last)
    return;
  
  for (RandomAccessIter i = first + 1; i != last; ++i)
    iter_swap(i, first + rand.randint((i - first)));
}
    

extern TRandomGenerator *globalRandom;

#endif
