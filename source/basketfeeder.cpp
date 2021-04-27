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
#include "basketfeeder.px"

map<string, TMetaDescriptor> TBasketFeeder::itemCache;


string trim(const string &s)
{ 
  string::const_iterator si(s.begin()), se(s.end());
  
  while ((si!=se) && (*si==' '))
    si++;
  while ((si!=se) && (se[-1]==' '))
    se--;
  
  return string(si, se);
}


void trim(char *s)
{ 
  char *si = s, *se = s + strlen(s);

  while (*si && (*si==' '))
    si++;
  while ((si!=se) && (se[-1]==' '))
    se--;

  while(si!=se)
    *(s++) = *(si++);
  *s = 0;
}


TBasketFeeder::TBasketFeeder(PDomain const &dom, bool const dcs, bool const ds)
: domain(dom),
  dontStore(ds),
  dontCheckStored(dcs)
{}


/*! Given a string in form "word" or "word=weight", returns the id of the
    meta attribute representing the word and the weight (or 1, if no weight
    is given). Any spaces at the beginning or the end of the string are removed.

    Meta attribute id's are obtained from three sources. #localStore contains
    id's of words encountered in this file. If the word is not found there,
    the #sourceDomain is checked. If this files, the global #itemCache is
    checked, unless #dontCheckStored is set. If the word is still not found, a
    new meta id is created.

    Unless the attribute is found in the local store and thus already in the
    #domain, the meta attribute descriptor is added to the #domain.

    The word and the corresponding id is stored in the local store, and also in
    #itemCache unless #dontStore is set.

    The argument \c lineno is used for error message in case the weight is
    invalid.
*/
void TBasketFeeder::addItem(string const &atom2, int const lineno,
                            TMetaId &id, double &weight)
{
    string atom;
    string::const_iterator bi = atom2.begin();
    string::size_type ei = atom2.find('=');
    if (ei == string::npos) {
        atom = trim(atom2);
        weight = 1.0;
    }
    else {
        atom = trim(string(bi, bi+ei));
        string trimmed = trim(string(bi+ei+1, atom2.end()));
        char *err;
        weight = strtod(trimmed.c_str(), &err);
        if (*err) {
            raiseError(PyExc_ValueError, "invalid number after '%s=' in line %i", atom.c_str(), lineno);
        }
    }

    id = 0;
    // Have we seen this item in this file already?
    map<string, int>::const_iterator item(localStore.find(atom));
    if (item != localStore.end()) {
        id = (*item).second;
    }
    else {
        if (sourceDomain) {
            const TMetaDescriptor *md = sourceDomain->metas[atom];
            if (md) {
                id = md->id;
                TMetaDescriptor nmd(id, md->variable, 1); // optional meta!
                // store to global cache, if allowed and if sth with that name is not already there
                if (!dontStore) {
                    map<string, TMetaDescriptor>::const_iterator gitem(itemCache.find(atom));
                    if (gitem == itemCache.end()) {
                        itemCache[atom] = nmd;
                    }
                }
                domain->metas.push_back(nmd);
            }
        }
        if (!id && !dontCheckStored) { // !sourceDomain or not found there
            map<string, TMetaDescriptor>::const_iterator gitem(itemCache.find(atom));
            if (gitem != itemCache.end()) {
                id = (*gitem).second.id;
                domain->metas.push_back((*gitem).second);
            }
        }
        // Not found anywhere - need to create a new attribute
        if (!id) {
            id = getMetaID();
            PVariable var = TVariable::make(atom, TVariable::Continuous);
            domain->metas.push_back(TMetaDescriptor(id, var, true));
            // store to global cache, if allowed and if not already there
            // Why dontCheckStored? If we have already searched there, we don't have to confirm again it doesn't exist
            if (!dontStore && (!dontCheckStored || (itemCache.find(atom) == itemCache.end()))) {
                itemCache[atom] = domain->metas.back();
            }
        }
        localStore[atom] = id;
    }
}
