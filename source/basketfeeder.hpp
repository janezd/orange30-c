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

#ifndef __BASKETFEEDER_HPP
#define __BASKETFEEDER_HPP

#include "basketfeeder.ppp"

/*! Helper class for reading baskets. See #addItem for detailed description. */
class TBasketFeeder : public TOrange {
public:
    __REGISTER_CLASS(BasketFeeder);
    PICKLING_ARGS(NO_PICKLING);

    /// Disables storing of items to the global #itemCache
    bool dontStore; //P disables items storing

    /// Disables checking the global #itemCache
    bool dontCheckStored; //P disables items lookup in the global cache

    /// Domain into which the meta attributes are stored
    PDomain domain; //P Domain in which the meta attributes are stored

    /// Domain with attributes that can be reused
    PDomain sourceDomain; // Domain with items that can be reused

    TBasketFeeder(PDomain const &,
                  bool const dontCheckStored=false,
                  bool const dontStore=false);

    void addItem(string const &atom,
                 int const lineno,
                 TMetaId &, double &quantity);
private:
    /// Words and the corresponding id's for the current file
    map<string, int> localStore;

    /// A global, static dictionary with words and meta descriptors
    static map<string, TMetaDescriptor> itemCache;
};

#endif
