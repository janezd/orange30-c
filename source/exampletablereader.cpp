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
#include "progarguments.hpp"
#include "basketfeeder.hpp"
#include "exampletablereader.px"


const TExampleTableReader::TIdentifierDeclaration
    TExampleTableReader::typeIdentifiers[] = {
        {"discrete", 0, TVariable::Discrete},      {"d", 0, TVariable::Discrete},
        {"continuous", 0, TVariable::Continuous},  {"c", 0, TVariable::Continuous},
        {"string", 0, TVariable::String},             {"s", 0, TVariable::String},
        //  {"python", 0, PYTHONVAR},             {"python:", 7, PYTHONVAR},
        {NULL, 0}
};


typedef vector<pair<string::const_iterator, string::const_iterator> > TSplits;

void split(const string &s, TSplits &atoms)
{
  atoms.clear();

  for(string::const_iterator si(s.begin()), se(s.end()), sii; si != se;) {
    while ((si != se) && (*si <= ' '))
      si++;
    if (si == se)
      break;
    sii = si;
    while ((si != se) && (*si > ' '))
      si++;
    atoms.push_back(make_pair(sii, si));
  }
}


bool hasNonEmptyAtom(const vector<string> &atoms)
{
    const_ITERATE(vector<string>, ai, atoms) {
        if ((*ai).length())
            return true;
    }
    return false;
}


TExampleTableReader::TExampleTableReader(
    string const &afname, int const header, int const cno,
    PStringList const &undefs, bool const ncd, bool const nc,
    char const asep, char const aquote, char const acomment,
    bool const anescape, bool const coab)
: filename(afname),
  classPos(-1),
  basketPos(0),
  sep(asep),
  quote(aquote),
  comment(acomment),
  escapes(anescape),
  commentOnlyAtBeginning(coab),
  headerType(header),
  createNewOn(cno),
  noCodedDiscrete(ncd),
  noClass(nc),
  undefineds(undefs),
  nrRowsHint(0)
{}


bool TExampleTableReader::readLine(FILE *f, int &fileLine,
                                   vector<string> &atoms,
                                   bool const skipEmptyLines,
                                   bool const skipEmptyAtomsLines) const
{
    enum {READ_START_LINE, READ_START_ATOM, READ, QUOTED, END_QUOTED,
        COMMENT, CARRIAGE_RETURNED, ESCAPE, DONE} state;
    state = READ_START_LINE;
    atoms.clear();
    string curAtom;
    int col=0;
    char c;
    for(;;) {
        if (state != DONE) {
            c = fgetc(f);
        }
        col++;

        switch (state) {
            case READ_START_LINE:
                fileLine++;
                if (c==comment) {
                    state = COMMENT;
                    break;
                }

            case READ_START_ATOM:
                if (c==quote) {
                    state = QUOTED;
                    break;
                }

            case READ:
                if ((c==comment) && !commentOnlyAtBeginning) {
                    const_ITERATE(string, ci, curAtom) {
                        if (*ci!=' ') {
                            atoms.push_back(curAtom);
                            break;
                        }
                    }
                    state = COMMENT;
                    break;
                }
                if ((c=='\\') && escapes) {
                    state = ESCAPE;
                    break;
                }
                if ((c==sep) || (c=='\r') || (c=='\n') || (c==EOF)) {
                    atoms.push_back(curAtom);
                    curAtom.clear();
                    if ((c=='\n') || (c==EOF))
                        state = DONE;
                    else if (c=='\r')
                        state = CARRIAGE_RETURNED;
                    else
                        state = READ_START_ATOM;
                    break;
                }
                curAtom += c;
                state = READ; // could have come here READ_START_LINE or READ_START_ATOM
                break;

            case QUOTED:
                if ((c=='\r') || (c=='\n')) {
                    raiseError(PyExc_ValueError,
                        "%s(%i, col %i): end of line encountered within a quoted value",
                        filename.c_str(), fileLine, col);
                }
                if (c==EOF) {
                    raiseError(PyExc_ValueError,
                        "%s(%i): end of file encountered within a quoted value",
                        filename.c_str(), fileLine);
                }
                if (c==quote) {
                    state = END_QUOTED;
                    break;
                }
                curAtom += c;
                break;

            case END_QUOTED:
                if ((c==sep) || (c=='\r') || (c=='\n') || (c==EOF)) {
                    atoms.push_back(curAtom);
                    curAtom.clear();
                    if ((c=='\n') || (c==EOF))
                        state = DONE;
                    else if (c=='\r')
                        state = CARRIAGE_RETURNED;
                    else
                        state = READ_START_ATOM;
                    break;
                }
                if ((c==' ') || (c=='\t')) {
                    break;
                }
                raiseError(PyExc_ValueError,
                    "%s(%i, col %i): quoted value should be followed by value separator or end of line",
                    filename.c_str(), fileLine, col);

            case COMMENT:
                if ((c=='\n') || (c==EOF))
                    state = DONE;
                else if (c=='\r')
                    state = CARRIAGE_RETURNED;
                break;

            case CARRIAGE_RETURNED:
                if ((c!='\n') && (c!=EOF)) {
                    ungetc(c, f);
                }
                state = DONE;
                break;

            case ESCAPE:
                switch (c) {
                    case 't': curAtom += '\t'; break;
                    case 'n': curAtom += '\n'; break;
                    case 'r': curAtom += '\r'; break;
                    case '"': curAtom += '"'; break;
                    case '\'': curAtom += '\''; break;
                    case ' ': curAtom += ' '; break;
                    case '\\': curAtom += '\\'; break;
                    case '\r':
                    case '\n': raiseError(PyExc_ValueError,
                                   "%s(%i): end of line encountered in escape sequence",
                                   filename.c_str(), fileLine);
                    case EOF: raiseError(PyExc_ValueError,
                                  "%s(%i): end of file encountered in escape sequence",
                                  filename.c_str(), fileLine);
                    default: raiseError(PyExc_ValueError,
                                 "%s(%i, col %i): invalid escape sequence ('\\%c')",
                                 filename.c_str(), fileLine, col, c);
                }
                state = READ;
                break;

            case DONE:
                if (   (!skipEmptyLines || atoms.size())
                    && (!skipEmptyAtomsLines || hasNonEmptyAtom(atoms))) {
                    return true;
                }
                if (c==EOF) {
                    return false;
                }
                state = READ_START_LINE;
                break;
        }
    }
}                      


void TExampleTableReader::readDomain()
{ 
    // non-NULL when this cannot be tab file (reason given as result)
    // NULL if this seems a valid tab file
    string notTabReason = whyNotTab();

    TDomain::TAttributeDescriptions descriptions;
    switch (headerType) {
    case 1:
        if (!notTabReason.size()) {
            PyErr_WarnFormat(PyExc_UserWarning, 1, 
                "'%s' is being loaded as .txt, but could be .tab file",
                filename.c_str());
        }
        readTxtHeader(descriptions);
        break;
    case 3:
        if (notTabReason.size()) {
            PyErr_WarnFormat(PyExc_UserWarning, 1,
                "'%s' is being loaded as .tab, but looks more like .txt file\n(%s)",
                filename.c_str(), notTabReason.c_str());
        }
        readTabHeader(descriptions);
        break;
    case 0:
        prepareBasket();
        break;
    default:
        raiseError(PyExc_ValueError, "invalid header type (%i)", headerType);
    }

    TDomain::TAttributeDescriptions attributeDescriptions, metaDescriptions;
    if (!headerType) {
        attributeTypes.push_back(0);
    }
    else {
        scanAttributeValues(descriptions);

        vector<int>::iterator ati(attributeTypes.begin());
        int ind = 0, lastRegular = -1;
        for(TDomain::TAttributeDescriptions::iterator
            adi(descriptions.begin()), ade(descriptions.end());
            adi != ade; adi++, ati++, ind++) {
                if (!*ati) {
                    continue;
                }
                if (adi->varType == -1) {
                    switch (detectAttributeType(*adi, noCodedDiscrete)) {
                        case 0:
                        case 2:
                            adi->varType = TVariable::Discrete;
                            break;
                        case 1:
                            adi->varType = TVariable::Continuous;
                            break;
                        case 4:
                            adi->varType = TVariable::String;
                            *ati = 1;
                            break;
                        default:
                            raiseError(PyExc_TypeError,
                                "cannot determine type for attribute '%s'; ",
                                adi->name.c_str());
                            *ati = 0;
                            continue;
                    }
                }
                if (*ati == 1) {
                    metaDescriptions.push_back(*adi);
                }
                else if ((classPos != ind) && (basketPos != ind)) {
                    attributeDescriptions.push_back(*adi);
                    lastRegular = ind;
                }
        }
        if (classPos > -1) {
            attributeDescriptions.push_back(descriptions[classPos]);
        }
        else if ((headerType == 1) && !noClass) {
            classPos = lastRegular;
        }
    }
    domain = TDomain::prepareDomain(&attributeDescriptions, classPos>-1,
        &metaDescriptions, createNewOn, status, metaStatus);
    if (metaStatus) {
        TIntIntList::const_iterator mid(metaStatus->begin());
        ITERATE(vector<int>, ii, attributeTypes) {
            if (*ii == 1) {
                *ii = mid++ ->first;
            }
        }
    }
}


PExampleTable TExampleTableReader::read()
{
    if (!domain) {
        readDomain();
    }
    vector<string> atoms;
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        raiseError(PyExc_IOError,
            "File '%s' not found or cannot be opened", filename.c_str());
    }
    PExampleTable table(new TExampleTable(domain, nrRowsHint));
    const int nAttrs = domain->attributes->size();
    PVariable classVar = domain->classVar;
    PBasketFeeder basketFeeder(basketPos!=-1 ? new TBasketFeeder(domain) : NULL);
    try {
        int fileLine = 0;
        skipHeader(f, fileLine);
        while (!feof(f)) {
            if (!readLine(f, fileLine, atoms))
                break;
            if (atoms.size() < attributeTypes.size())
                atoms.resize(attributeTypes.size());
            const int exampleIdx = table->new_example();
            double *exdata = (double *)table->examples[exampleIdx];
            int *metaHandle = table->supportsMeta() ?  // all do, now; this is for the future
                &table->getMetaHandle(exampleIdx) : NULL;
            TVarList::const_iterator vi(domain->attributes->begin());
            vector<string>::const_iterator ai(atoms.begin());
            vector<int>::const_iterator si(attributeTypes.begin()), se(attributeTypes.end());
            double *exi = exdata;
            int pos = 0;
            for (; (si!=se); pos++, si++, ai++) {
                 if (*si) { // if attribute is not to be skipped and is not a basket
                     string valstr = isUndefined(*ai) ? "?" : *ai;
                     if (*si==-1) {
                         if (pos==classPos) { // if this is class value
                             exdata[nAttrs] = classVar->filestr2val(valstr);
                         }
                         else { // if this is a normal value
                             *exi++ = (*vi++)->filestr2val(valstr);
                         }
                     }
                     else { // if this is a meta value
                         TMetaDescriptor *md = domain->metas[*si];
                         if (md->variable->isPrimitive()) {
                             MetaChain::set(*metaHandle, md->id,
                                 md->variable->filestr2val(valstr));
                         }
                         else {
                             MetaChain::set(*metaHandle, md->id,
                                 md->variable->filestr2pyval(valstr));
                         }
                      }
                 }
                 // the attribute is marked to be skipped or it is a basket
                 else { 
                     if (pos == basketPos) {
                         TSplits splits;
                         split(*ai, splits);
                         int id;
                         double quantity;
                         ITERATE(TSplits, si, splits) {
                             basketFeeder->addItem(string(si->first, si->second),
                                 fileLine, id, quantity);
                             MetaChain::setDefault(*metaHandle, id, 0) += quantity;
                         }
                     }
                 }
            }
            if (pos==classPos) { // if class is the last value in the line, it is set here
                exdata[nAttrs] = classVar->filestr2val(
                    ai==atoms.end() || isUndefined(*ai) ? "?" : *ai++);
            }
            for(; (ai!=atoms.end()) && !(*ai).length(); ai++); // line must be empty from now on
            if (ai!=atoms.end()) {
                raiseError(PyExc_ValueError,
                    "example of invalid length in line %i", fileLine);
            }
        }
        fclose(f);
        return table;
    }
    catch (...) {
        fclose(f);
        throw;
    }
}


void TExampleTableReader::prepareBasket()
{
    classPos = -1;
    basketPos = 0;
    headerLines = 0;
}

void TExampleTableReader::readTabHeader(TDomain::TAttributeDescriptions &descs)
{
    classPos = -1;
    basketPos = -1;
    headerLines = 3;

    vector<string> varNames, varTypes, varFlags;

    int fileLine = 0;
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        raiseError(PyExc_IOError,
            "File '%s' not found or cannot be opened", filename.c_str());
    }
    try {
        if (!readLine(f, fileLine, varNames))
            raiseError(PyExc_ValueError, "empty file");
        if (feof(f) || !readLine(f, fileLine, varTypes))
            raiseError(PyExc_ValueError, "cannot read types of attributes");
        if (feof(f) || !readLine(f, fileLine, varFlags, false, false)) 
            raiseError(PyExc_ValueError, "cannot read types of attributes");
        fclose(f);
    }
    catch (...) {
        fclose(f);
        throw;
    }

    if (varNames.size() != varTypes.size())
        raiseError(PyExc_ValueError,
            "mismatching number of attributes and their types");
    if (varNames.size() < varFlags.size())
        raiseError(PyExc_ValueError,
            "too many flags (third line of the header too long)");

    attributeTypes.assign(varNames.size(), -1);
    vector<string>::iterator vni(varNames.begin()), vne(varNames.end());
    vector<string>::iterator ti(varTypes.begin());
    vector<string>::iterator fi(varFlags.begin()), fe(varFlags.end());
    vector<int>::iterator attributeType(attributeTypes.begin());
    int ind = 0;
    for(; vni!=vne; vni++, ti++, attributeType++, ind++) { // fi increased only if fi!=fe
        descs.push_back(TVariable::TAttributeDescription(*vni, 0));
        TVariable::TAttributeDescription &desc = descs.back();
        bool ordered = false;

        if (fi!=fe) {
            TProgArguments args("ordered", *fi++, false, true);
            if (args.direct.size()) {
                if (args.direct.size()>1) {
                    raiseError(PyExc_ValueError,
                        "invalid flags for attribute '%s'", (*vni).c_str());
                }
                string direct = args.direct.front();
                if ((direct=="s") || (direct=="skip") ||
                    (direct=="i") || (direct=="ignore")) {
                        *attributeType = 0;
                }
                else if ((direct=="c") || (direct=="class")) {
                    if (classPos != -1) {
                        raiseError(PyExc_ValueError,
                            "multiple attributes are specified as class attribute ('%s' and '%s')",
                            vni->c_str(), vni->c_str());
                    }
                    classPos = ind;
                }
                else if ((direct=="m") || (direct=="meta")) {
                    *attributeType = 1;
                }
            }
            ordered = args.exists("ordered");
            desc.userFlags = args.unrecognized;
        }

        if (!strcmp((*ti).c_str(), "basket")) {
            if (basketPos > -1) {
                raiseError(PyExc_ValueError,
                    "there can be only one basket attribute");
            }
            if (ordered || (classPos == ind) || (*attributeType != -1)) {
                raiseError(PyExc_ValueError,
                    "'basket' flag is incompatible with other flags");
            }
            basketPos = ind;
            *attributeType = 0;
        }

        if (!*attributeType)
            continue;

        if (!(*ti).length())
            raiseError(PyExc_ValueError,
            "type for attribute '%s' is empty", (*vni).c_str());
        const TIdentifierDeclaration *tid = typeIdentifiers;
        for(; tid->identifier; tid++) {
            if (!(tid->matchRoot ? strncmp(tid->identifier, (*ti).c_str(), tid->matchRoot)
                : strcmp(tid->identifier, (*ti).c_str()))) {
                    desc.varType = tid->varType;
                    desc.typeDeclaration = *ti;
                    break;
            }
        }
        if (!tid->identifier) {
            desc.varType = TVariable::Discrete;
            string vals;
            ITERATE(string, ci, *ti) {
                if (*ci==' ') {
                    if (vals.length()) {
                        desc.addValue(vals);
                    }
                    vals="";
                }
                else {
                    if ((*ci=='\\') && (ci[1]==' ')) {
                        vals += ' ';
                        ci++;
                    }
                    else {
                        vals += *ci;
                    }
                }
            }

            if (vals.length()) {
                desc.addValue(vals);
            }
        }
        if ((desc.varType != TVariable::Discrete) &&
            (desc.varType != TVariable::Continuous) && 
            (*attributeType != 1)) {
                *attributeType = 1;
                PyErr_WarnFormat(PyExc_UserWarning, 1,
                    "attribute '%s' is neither discrete nor continuous and has "
                    "been placed to meta attributes although not declared as such",
                    desc.name.c_str());
        }

    }
}


void TExampleTableReader::readTxtHeader(TDomain::TAttributeDescriptions &descs)
{ 
    headerLines = 1;
    classPos = -1;
    basketPos = -1;

    int fileLine = 0;
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        raiseError(PyExc_IOError,
            "File '%s' not found or cannot be opened", filename.c_str());
    }
    vector<string> varNames;
    try {
        if (!readLine(f, fileLine, varNames)) {
            raiseError(PyExc_ValueError, "empty file");
        }
        fclose(f);
    }
    catch (...) {
        fclose(f);
        throw;
    }

    attributeTypes.assign(varNames.size(), -1);
    vector<int>::iterator attributeType(attributeTypes.begin());
    vector<string>::const_iterator ni(varNames.begin()), ne(varNames.end());
    int ind = 0;
    for(; ni != ne; ni++, ind++, attributeType++) {
        /* Parses the header line
        - sets *ni to a real name (without prefix)
        - sets varType to TValue::varType or -1 if the type is not specified and -2 if it's a basket
        - sets classPos/basketPos to the current position, if the attribute is class/basket attribute
        (and reports an error if there is more than one such attribute)
        - to attributeTypes, appends -1 for ordinary atributes, 1 for metas and 0 for ignored or baskets*/
        int varType = -1; // varType, or -1 for unnown, -2 for basket

        const char *cptr = (*ni).c_str();
        if (*cptr && (cptr[1]=='#') || (cptr[1] && (cptr[2] == '#'))) {
            switch (*cptr) {
                case 'm':
                    *attributeType = 1;
                    cptr++;
                    break;
                case 'i':
                    *attributeType = 0;
                    cptr++;
                    break;
                case 'c':
                    if (classPos>-1) {
                        raiseError(PyExc_ValueError,
                            "more than one attribute marked as class");
                    }
                    classPos = ind;
                    cptr++;
                    break;
            }
            // cptr may have moved
            switch (*cptr) {
                case 'D':
                    varType = TVariable::Discrete;
                    cptr++;
                    break;
                case 'C':
                    varType = TVariable::Continuous;
                    cptr++;
                    break;
                case 'S':
                    varType = TVariable::String;
                    cptr++;
                    break;
                case 'B':
                    if ((*attributeType != -1) || (classPos == ind)) {
                        raiseError(PyExc_ValueError,
                            "flag 'B' is incompatible with '%c'", cptr[-1]);
                    }
                    if (basketPos > -1) {
                        raiseError(PyExc_ValueError,
                            "more than one basket attribute");
                    }
                    varType = -2;
                    *attributeType = 0;
                    basketPos = ind;
                    cptr++;
                    break;
            }

            if (*cptr != '#') {
                raiseError(PyExc_ValueError,
                    "unrecognized flags in attribute '%s'", cptr);
            }
            cptr++;
        }

        descs.push_back(TVariable::TAttributeDescription(cptr, varType));
    }
}


string TExampleTableReader::whyNotTab()
{
    vector<string> varNames, varTypes, varFlags;
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        raiseError(PyExc_IOError,
            "File '%s' not found or cannot be opened", filename.c_str());
    }
    try {
        int fileLine = 0;
        if (!readLine(f, fileLine, varNames)) {
            fclose(f);
            return "empty file";
        }
        if (feof(f) || !readLine(f, fileLine, varTypes)) {
            fclose(f);
            return "no line with attribute types";
        }
        if (feof(f) || !readLine(f, fileLine, varFlags)) {
            fclose(f);
            return "no line with attribute flags";
        }
        fclose(f);
    }
    catch (...) {
        fclose(f);
        throw;
    }

    vector<string>::const_iterator vi, ai, ei;
    for(vi = varNames.begin(), ei = varNames.end(); vi!=ei; vi++) {
        const char *c = (*vi).c_str();
        if ((*c=='m') || (*c=='c') || (*c=='i'))
            c++;
        if (   ((*c=='D') || (*c=='C') || (*c=='S'))
            && (c[1]=='#')) {
                return "attribute '"+*vi+"'is formatted as a .txt name";
        }
    }

    if (varTypes.size() != varNames.size()) {
        return "the number of attribute types does not match the number of attributes";
    }
    for(vi = varNames.begin(), ai = varTypes.begin(), ei = varTypes.end(); ai != ei; ai++, vi++) {
        const char *c = (*ai).c_str();
        if (!*c) {
            return "empty type entry for attribute '"+*vi+"'";
        }
        if (!strcmp("basket", c))
            continue;

        const TIdentifierDeclaration *tid = typeIdentifiers;
        for(; tid->identifier && (tid->matchRoot ?
            strncmp(tid->identifier, c, tid->matchRoot)
            : strcmp(tid->identifier, c)); tid++);
        if (tid->identifier) {
            continue;
        }

        for(; *c && (*c!=' '); c++);
        if (!*c) {
            return "attribute '"+*vi+"' is defined as having only one value ('"+*ai+"')";
        }
    }

    if (varFlags.size() > varNames.size())
        return "the number of attribute options is greater than the number of attributes";
    for(vi = varNames.begin(), ai = varFlags.begin(), ei = varFlags.end(); ai != ei; ai++, vi++) {
        TProgArguments args("ordered", *ai, false, true);
        if (args.direct.size()>1) {
            return "too many direct options at attribute '"+*vi+"'";
        }
        if (args.direct.size()) {
            static const char *legalDirects[] = {
                "s", "skip","i", "ignore", "c", "class", "m", "meta", NULL};
            string &direct = args.direct.front();
            const char **lc = legalDirects;
            for(; *lc && strcmp(*lc, direct.c_str()); lc++);
            if (!*lc) {
                return "unrecognized option ('"+*ai+"') at attribute '"+*vi+"'";
            }
        }
    }
    return string();
}


bool TExampleTableReader::isUndefined(const string &s) const
{
    if (!s.size()) {
        return true;
    }
    const char c = s[0];
    if ((s.size() == 1) && ((c=='?') || (c=='.') || (c=='~') || (c=='*'))
        || s=="NA") {
        return true;
    }
    if (undefineds) {
        const_PITERATE(TStringList, ui, undefineds) {
            if (s == *ui) {
                return true;
            }
        }
    }
    return false;
}


int TExampleTableReader::detectAttributeType(TVariable::TAttributeDescription &desc,
                                             const bool noCodedDiscrete)
{
  char numTest[64];

  int status = 3;  //  3 - not encountered any values, 2 - can be coded discrete, 1 - can be float, 0 - must be nominal
                   //  4 (set later) - string value
  typedef map<string, int> msi;
  ITERATE(msi, vli, desc.values) {
      const string s(vli->first);
      if (vli->first.length() > 63) {
          status = 0;
          break;
      }
      if (isUndefined(s)) {
          continue;
      }
      if (status == 3) {
          status = 2;
      }
      if ((status == 2) && ((s.size()>1) || (s[0]<'0') || (s[1]>'9'))) {
          status = noCodedDiscrete ? 1 : 2;
      }
      if (status == 1) {
          strcpy(numTest, s.c_str());
          for(char *sc = numTest; *sc; sc++) {
              if (*sc == ',') {
                  *sc = '.';
              }
          }
          char *eptr;
          double foo = strtod(numTest, &eptr);
          while (*eptr==32) {
              eptr++;
          }
          if (*eptr) {
              status = 0;
              break;
          }
      }
  }
  /* Check whether this is a string attribute:
     - has more than 20 values
     - less than half of the values appear more than once */
  if ((status==0) && (desc.values.size() > 20)) {
      int more2 = 0;
      for(map<string, int>::const_iterator dvi(desc.values.begin()),
          dve(desc.values.end()); dvi != dve; dvi++) {
              if (dvi->second > 1) {
                  more2++;
              }
      }
      if (more2*2 < desc.values.size()) {
          status = 4;
      }
  }
  return status;
}


void TExampleTableReader::skipHeader(FILE *f, int &fileLine) const
{
    vector<string> atoms;
    for(int i=headerLines; !feof(f) && i--; ) {
        const bool skipEmpty = (headerLines!=3) || i;
        readLine(f, fileLine, atoms, skipEmpty, skipEmpty);
    }
}


void TExampleTableReader::scanAttributeValues(TDomain::TAttributeDescriptions &desc)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        raiseError(PyExc_IOError,
            "File '%s' not found or cannot be opened", filename.c_str());
    }
    try {
        vector<string> atoms;
        int fileLine = 0;
        skipHeader(f, fileLine);
        vector<string>::const_iterator ai, ae;
        TDomain::TAttributeDescriptions::iterator di, db(desc.begin()), de(desc.end());
        vector<int>::const_iterator ati, atb(attributeTypes.begin());
        nrRowsHint = 0;
        while(!feof(f)) {
            if (!readLine(f, fileLine, atoms))
                break;
            nrRowsHint++;
            for(di = db, ati = atb, ai = atoms.begin(), ae = atoms.end(); (di != de) && (ai != ae); di++, ai++, ati++) {
                if (!*atb || isUndefined(*ai) ||
                    (di->varType > 0) && (di->varType != TVariable::Discrete)) {
                        continue;
                }
                map<string, int>::iterator vf = di->values.lower_bound(*ai);
                if ((vf != di->values.end()) && (vf->first == *ai)) {
                    vf->second++;
                }
                else {
                    di->values.insert(vf, make_pair(*ai, 1));
                }
            }
        }
    }
    catch(...) {
        fclose(f);
        throw;
    }
    fclose(f);
}


bool splitFilename(string const &filename, size_t &stem, size_t &ext)
{
    stem = filename.rfind('/');
#ifdef _WIN32
    size_t lastBackslash = filename.rfind('\\');
    if ((lastBackslash != string::npos) &&
        ((stem == string::npos) || (lastBackslash > stem))) {
            stem = lastBackslash; 
    }
#endif
    if (stem == string::npos) {
        stem = 0;
    }
    ext = filename.rfind('.');
    if ((ext != string::npos) && (stem >= ext)) {
        ext = string::npos;
    }
    return ext != string::npos;
}

string getExtension(string const &filename)
{
    size_t stem, ext;
    return splitFilename(filename, stem, ext) ? filename.substr(ext) : string();
}

string getStem(string const &filename)
{
    size_t stem, ext;
    splitFilename(filename, stem, ext);
    return filename.substr(stem, ext);
}


PyObject *TExampleTableReader::read(bool readerOnly)
{
    PyObject *res;
    if (readerOnly) {
        // Increase reference count!!!
        res = PyObject_FromOrange(this);
    }
    else {
        res = read().getPyObject();
        PyObject_SetAttrString(res, "name",
            PyUnicode_FromString(getStem(filename).c_str()));
        PyObject_SetAttrString(res, "attributeLoadStatus",
            status.borrowPyObject());
        PyObject_SetAttrString(res, "metaAttributeLoadStatus",
            metaStatus.borrowPyObject());
        PyObject_SetAttrString(res, "attribute_load_status",
            status.borrowPyObject());
        PyObject_SetAttrString(res, "meta_attribute_load_status",
            metaStatus.borrowPyObject());
    }
    return res;
}

void TExampleTableReader::setFromFileName(string const &name)
{
    string ext(getExtension(name));
    if (!ext.size())
        raiseError(PyExc_ValueError, "File does not have an extension");

    vector<int> status;
    vector<pair<int, int> > metaStatus;
    if (ext == ".txt") {
        headerType = 1;
        sep = '\t';
    }
    else if (ext == ".tab") {
        headerType = 3;
        sep = '\t';
    }
    else if (ext == ".csv") {
        headerType = 1;
        sep = ',';
    }
    else if (ext == ".basket") {
        headerType = 0;
        sep = -1;
    }
    else {
        raiseError(PyExc_ValueError,
            "File format '%s' is not supported", ext.c_str());
    }
    filename = name;
}


#if defined _WIN32
    #define SEP ';'
    #define PATHSEP '\\'
#else
    #define SEP ':'
    #define PATHSEP '/'
#endif

bool fileExists(string const &filename)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if (f) {
        fclose(f);
    }
    return f != NULL;
}

bool TExampleTableReader::findFile_path(string const &filename,
                                        const char *paths)
{
    if (!paths) {
        return false;
    }
    for(const char *pi = paths, *pe=pi; *pi; pi = pe+1) {
        for(pe = pi; *pe && (*pe != SEP); pe++);
        if (pe==pi)
            continue;
        string name(pe[-1] == PATHSEP ? string(pi, pe) + filename
                                      : string(pi, pe) + PATHSEP + filename);
        if (fileExists(name)) {
            setFromFileName(name);
            return true;
        }
        if (!*pe)
            break;
    }
    return false;
}

const char *knownExtensions[] = {".tab", ".txt", ".csv", ".basket", NULL};


bool TExampleTableReader::findFile(string name)
{
    if (!name.size()) {
        name = filename;
    }
    string ext(getExtension(name.c_str()));
    if (!ext.size()) {
        char const **next;
        // First try all extensions in the current directory
        for(next = knownExtensions; *next; next++) {
            string nname = name + *next;
            if (fileExists(nname)) {
                setFromFileName(nname);
                return true;
            }
        }
        // No luck, try all extensions in all directories
        for(next = knownExtensions; *next; next++) {
            if (findFile(name+*next))
                return true;
        }
        return false;
    }
    // We have an extension
    // Try current directory first
    if (fileExists(name)) {
        setFromFileName(name);
        return true;
    }
    // Now search the path
    PyObject *configurationModule = PyImport_ImportModule("orngConfiguration");
    GUARD(configurationModule);
    if (configurationModule) {
        PyObject *confmodule = PyModule_GetDict(configurationModule);
        PyObject *datasetsPath = PyDict_GetItemString(confmodule, "datasetsPath");
        if (datasetsPath && findFile_path(name, PyUnicode_AS_DATA(datasetsPath)))
            return true;
    }

    // Try fo find the file using Orange.data.io.find_file
	PyObject *ioModule = PyImport_ImportModule("Orange.data.io");
    if (ioModule) {
        PyObject *find_file = PyObject_GetAttrString(ioModule, "find_file");
        if (find_file) {
            PyObject *py_args = Py_BuildValue("(s)", filename.c_str());
            PyObject *ex_filename = PyObject_Call(find_file, py_args, NULL);
            if (ex_filename) {
                if (PyUnicode_Check(ex_filename)) {
                    PyObject *encoded = PyUnicode_EncodeFSDefault(ex_filename);
                    Py_DECREF(ex_filename);
                    setFromFileName(PyBytes_AS_STRING(encoded));
                    Py_DECREF(encoded);
                    return true;
                }
                else {
                    Py_DECREF(ex_filename);
                }
            }
            PyErr_Clear();
            Py_DECREF(py_args);
            Py_DECREF(find_file);
        }
        Py_DECREF(ioModule);
    }

    if (findFile_path(name, getenv("ORANGE_DATA_PATH"))) {
        return true;
    }

    return false;
}


/// @cond Python

TOrange *TExampleTableReader::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *filename_b;
    unsigned int headerType=3;
    int createNewOn=TVariable::Incompatible;
    PStringList undefineds;
    unsigned char noCodedDiscrete=0, noClass=0;
    char sep='\t', quote='"', comment='|';
    unsigned char escapes=1, commentOnlyAtBeginning=1;

    if (!PyArg_ParseTupleAndKeywords(args, kw,
             "O&|iiiO&bbbbbbb:ExampleTableReader", ExampleTableReader_keywords, 
             &PyUnicode_FSConverter, &filename_b,
             &headerType, &createNewOn,
             &PStringList::argconverter_n, &undefineds,
             &noCodedDiscrete, &noClass, &sep, &quote, &comment,
             &escapes, &commentOnlyAtBeginning)) {
                 return NULL;
    }
    char const *filename = PyBytes_AsString(filename_b);
    TOrange *reader = new TExampleTableReader(filename, headerType, createNewOn,
        undefineds, noCodedDiscrete!=0, noClass!=0, sep, quote, comment,
        escapes!=0, commentOnlyAtBeginning!=0);
    return reader;
}

PyObject *TExampleTableReader::py_readDomain()
{
    readDomain();
    Py_RETURN_NONE;
}

PyObject *TExampleTableReader::py_read()
{
    return read().getPyObject();
}

/// @endcond