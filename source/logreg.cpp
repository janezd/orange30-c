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
#include "logreg.px"

TLogRegClassifier::TLogRegClassifier(PDomain const &dom) 
: TClassifier(dom)
{};


PDistribution TLogRegClassifier::classDistribution(TExample const *const example)
{   
    checkProperty(domain);
    PExample cexample;
    cexample = example->convertedTo(domain, true);
    if (imputer) cexample = (*imputer)(*cexample);
    if (continuizedDomain) cexample = cexample->convertedTo(continuizedDomain);
    TAttributedFloatList::const_iterator b(beta->begin()), be(beta->end());

    TVarList::const_iterator vi(example->domain->attributes->begin());
    TExample::const_iterator ei(example->begin()), ee(example->end());
    double prob1 = *b++;
    for (; (b!=be) && (ei!=ee); ei++, b++, vi++) {
        if (isnan(*ei)) {
            raiseError(PyExc_ValueError,
                "unknown value for '%s'", (*vi)->cname());
        }
        prob1 += *ei * *b; 
    }
    prob1 = exp(prob1)/(1+exp(prob1));

    if (classVar->varType == TVariable::Discrete) {
        PDiscDistribution dist(new TDiscDistribution(classVar));
        dist->add(0, 1-prob1);
        dist->add(1, prob1);
        return dist;
    }
    else {
        PContDistribution dist(new TContDistribution(classVar));
        dist->add(prob1, 1.0);
        return dist;
    }
}
