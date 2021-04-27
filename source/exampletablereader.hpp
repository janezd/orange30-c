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


#ifndef __EXAMPLETABLEREADER_HPP
#define __EXAMPLETABLEREADER_HPP

#include "exampletablereader.ppp"

class TExampleTableReader : public TOrange {
public:
    __REGISTER_CLASS(ExampleTableReader)

    TExampleTableReader(string const &filename,
                        int const headerType,
                        int const createNewOn,
                        PStringList const &undefineds,
                        bool const noCodedDiscrete = false,
                        bool const noClass = false,
                        char const sep='\t',
                        char const quote='"',
                        char const comment='|',
                        bool const escape=true,
                        bool const commentOnlyAtBeginning=true);

    void readDomain();
    PExampleTable read();
    string whyNotTab();

    string filename; //P name of the file being read
    int classPos; //PR position of the class attribute
    int basketPos; //PR position of the (virtual) basket attribute
    char sep; //PR separator between values
    char quote; //PR symbol used for quotes
    char comment; //PR symbol used for starting comments
    bool escapes; //PR flag telling whether escapes are used
    bool commentOnlyAtBeginning; //PR flag telling whether comments should start at the beginning of the line
    int headerType; //PR 0-basket, 1-just names, 3-tab-delimited
    int createNewOn; //PR tells when to create a new attribute
    bool noCodedDiscrete; //PR flag telling whether values 0..9 are considered discrete (in auto detection mode)
    bool noClass; //PR flag telling whether the last attribute is class (in auto detection mode)
    PStringList undefineds; //PR List of symbolic names for undefined values (e.g. NA, 99...)

    PDomain domain; //PR Domain for the new data
    PIntList status; //PR Create status of domain's attributes
    PIntIntList metaStatus; //PR  Create status of domain's meta attributes

private:
    /*  A kind of each attribute:
              1   pending meta value (used only at construction time)
             -1   normal
              0   skipped
            <-1   meta value. */
    vector<int> attributeTypes; //P types of attributes (-1 normal, 0 skip, <-1 = meta ID)
    int headerLines;
    int nrRowsHint;

    typedef struct {
        char *identifier;
        int matchRoot;
        int varType;
    } TIdentifierDeclaration;

    static const TIdentifierDeclaration typeIdentifiers[] ;

    bool readLine(FILE *f, int &fileLine, vector<string> &atoms,
        bool const skipEmptyLines=true, bool const skipEmptyAtomLines=true) const;

    void readTxtHeader(TDomain::TAttributeDescriptions &);
    void readTabHeader(TDomain::TAttributeDescriptions &);
    void prepareBasket();

    void skipHeader(FILE *f, int &fileLine) const;
    int detectAttributeType(TVariable::TAttributeDescription &desc,
        bool const noCodedDiscrete);
    void scanAttributeValues(TDomain::TAttributeDescriptions &desc);
    bool isUndefined(string const &) const;

    void setFromFileName(string const &filename);
    bool findFile_path(string const &filename, char const *paths);

public:
    bool findFile(string name=string());
    PyObject *read(bool const readerOnly);

    PICKLING_ARGS(NO_PICKLING)
    static TOrange *__new__(PyTypeObject *type, PyObject *args, PyObject *kw) PYDOC("(filename, auto_detect[, create_new_on, undefineds, no_coded_discrete, no_class, sep, quote, comment, escapes, comment_only_at_beginning)");
    PyObject *py_readDomain() PYARGS(METH_NOARGS, "()");
    PyObject *py_read() PYARGS(METH_NOARGS, "()");
};

#endif

