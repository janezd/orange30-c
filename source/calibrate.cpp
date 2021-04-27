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

double findOptimalCAThreshold(PClassifier const &classifier,
                              PExampleTable const &data,
                              double &optCA, int const targetValue)
{
    if (!data->domain->classVar) {
        raiseError(PyExc_ValueError, "data has no class attribute");
    }
    TDiscreteVariable *classVar = dynamic_cast<TDiscreteVariable *>(classifier->classVar.borrowPtr());
    if (!classVar) {
        raiseError(PyExc_ValueError,
            "cannot measure classification accuracy in regression");
    }
    if (data->domain->classVar != classVar) {
        raiseError(PyExc_ValueError,
            "classifier has a different class attribute than the given data");
    }

    int wtarget;
    if (targetValue >= 0) {
        wtarget = targetValue;
    }
    else if (classVar->baseValue >= 0) {
        wtarget = classVar->baseValue;
    }
    else if (classVar->values->size() == 2) {
        wtarget = 1;
    }
    else {
        raiseError(PyExc_ValueError, "cannot determine target class: none is given, class is not binary and its 'baseValue' is not set");
    }

    typedef map<double, double> tmfpff;
    tmfpff dists;
    double N = 0.0, corr = 0.0;
    PEITERATE(ei, data) {
        const double cls = ei.getClass();
        if (!isnan(cls)) {
            double wei = ei.getWeight();
            N += wei;
            if (cls == wtarget) {
                corr += wei;
                wei = -wei;
            }
            const double prob = classifier->classDistribution(*ei)->at(wtarget);
            pair<tmfpff::iterator, bool> elm = dists.insert(make_pair(prob, wei));
            if (!elm.second) {
                elm.first->second += wei;
            }
        }
    }
    optCA = 0;
    if (dists.size() < 2) {
        return 0.5;
    }
    double optthresh;
    for(tmfpff::const_iterator ni(dists.begin()), ie(dists.end()), ii(ni++); ni != ie; ii = ni++) {
        corr += ii->second;
        if ((corr > optCA) || ((corr == optCA) && (ii->first < 0.5))) {
            optCA = corr;
            optthresh = (ii->first + ni->first) / 2.0;
        }
    }
    optCA /= N;
    return optthresh;
} 
