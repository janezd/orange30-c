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


#include "Python.h"

#ifndef NO_NUMPY
#ifdef _MSC_VER
#include "../Lib/site-packages/numpy/core/include/numpy/arrayobject.h"
#else
#include "numpy/arrayobject.h"
#endif
#endif

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <exception> 

#include <math.h>

#include "crc32.hpp"

#include "stladdon.hpp"

#include "pyxtract_macros.hpp"
#include "errors.hpp"
#include "converts.hpp"
#include "module.hpp"
#include "orange.hpp"
#include "orvector.hpp"
#include "bytestream.hpp"

#include "stat.hpp"

#include "randomgenerator.hpp"
#include "simplerandomgenerator.hpp"

#include "variable.ppp"
#include "example.ppp"
#include "classifier.ppp"
#include "domain.ppp"
#include "distribution.ppp"
#include "exampletable.ppp"
#include "filter.ppp"

#include "values.hpp"
#include "metachain.hpp"
#include "variable.hpp"
#include "discretevariable.hpp"
#include "continuousvariable.hpp"
#include "stringvariable.hpp"
#include "pyvalue.hpp"
#include "valuelist.hpp"
#include "domain.hpp"
#include "exampletable.hpp"
#include "example.hpp"
#include "exampleiterator.hpp"
#include "exampletable-inlines.hpp"

#include "distribution.hpp"
#include "discdistribution.hpp"
#include "contdistribution.hpp"
#include "domaindistributions.hpp"
#include "gaussiandistribution.hpp"

#include "basicattrstat.hpp"
#include "domainbasicattrstat.hpp"

#include "contingency.hpp"
#include "contingencyclass.hpp"
#include "contingencyclassattr.hpp"
#include "contingencyattrclass.hpp"
#include "contingencyattrattr.hpp"
#include "domaincontingency.hpp"

#include "orattributedvector.hpp"

#include "classifier.hpp"
#include "learner.hpp"

#include "transformvalue.hpp"
#include "classifierfromvar.hpp"