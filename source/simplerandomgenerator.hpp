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


#ifndef __SIMPLERANDOMGENERATOR_HPP
#define __SIMPLERANDOMGENERATOR_HPP

#include "simplerandomgenerator.ppp"

/* This is to be used when you only need a few random numbers and don't want to
   initialize the Mersenne twister.

   DO NOT USE THIS WHEN YOU NEED 32-BIT RANDOM NUMBERS!

   The below formula is the same as used in MS VC 6.0 library. */

class TSimpleRandomGenerator {
public:
  unsigned int seed;
  
  TSimpleRandomGenerator(int aseed = 0)
  : seed(aseed)
  {}

  unsigned int rand ()
  { return (((seed = seed * 214013L + 2531011L) >> 16) & 0x7fff); }

  int operator()(const int &y)
  { return rand() % y; }
  
  int randbool(const int &y=2)
  { return (rand()%y) == 0; }

  int randsemilong()
  { return rand()<<15 | rand(); }

  float randfloat()
  { return float(rand())/0x7fff; }
};

#endif
