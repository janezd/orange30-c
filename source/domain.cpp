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
#include "domain.px"


int TDomain::domainVersion=0;

/// Constructs a domain without any variables
TDomain::TDomain()
: classVar(),
  attributes(new TVarList()),
  variables(new TVarList()),
  anonymous(false),
  version(++domainVersion)
{
    // better here, we want knownDomains initialized before
    lastDomain = knownDomains.end(); 
}


/*! Constructs a domain with the given variables. The list
    is cloned. If the list is not empty, the last variable
    is used as the class. */
TDomain::TDomain(PVarList const &vl)
: classVar(vl->size() ? vl->back() : PVariable()),
  attributes(vl->clone()),
  variables(vl->clone()),
  anonymous(false),
  version(++domainVersion)
{ 
    lastDomain = knownDomains.end(); 
    checkPrimitive(*vl);
    if (attributes->size()) {
        attributes->erase(attributes->end()-1);
    }
}


/*! Constructs the list with the given variables and the class attribute,
    The lists are cloned. */
TDomain::TDomain(PVariable const &va, PVarList const &vl)
: classVar(va),
  attributes(vl->clone()),
  variables(vl->clone()),
  anonymous(false),
  version(++domainVersion)
{ 
    lastDomain = knownDomains.end(); 
    checkPrimitive(*vl, va);
    if (va) {
        variables->push_back(va); 
    }
}


/*! Constructs a domain with the given variables.
    If the list is not empty, the last variable
    is used as the class. */
TDomain::TDomain(TVarList const &vl)
: classVar(vl.size() ? vl.back() : PVariable()),
  attributes(vl.clone()),
  variables(vl.clone()),
  anonymous(false),
  version(++domainVersion)
{ 
    lastDomain = knownDomains.end(); 
    checkPrimitive(vl);
    if (variables->size()) {
        variables->erase(variables->end()-1);
    }
}


/*! Constructs the list with the given variables and the class attribute,
    The lists are cloned. */
TDomain::TDomain(PVariable const &va, TVarList const &vl)
: classVar(va),
  attributes(vl.clone()),
  variables(vl.clone()),
  anonymous(false),
  version(++domainVersion)
{ 
    lastDomain = knownDomains.end(); 
    checkPrimitive(vl, va);
    if (va) {
        variables->push_back(va); 
    }
}


/*! Copy constructor. */
TDomain::TDomain(const TDomain &old)
: TOrange(old),
  classVar(old.classVar),
  attributes(old.attributes->clone()),
  variables(old.variables->clone()),
  anonymous(false),
  metas(old.metas),
  version(++domainVersion),
  knownDomains()  // don't copy, unless you want to mess with the notifiers..
{
    lastDomain = knownDomains.end();
}


/*! Destructor, which also notifies other domains that the domain has
    changed. */
TDomain::~TDomain()
{ 
    domainChangedDispatcher(); 
}


/*! Checks whether the discrete variable has the same order of values
    as prescribed in the description. If the variable is continuous or
    it has fewer values (but still matches the beginning of the description)
    the function returns \c true. 

    \todo
    Why is this a global function? Doesn't it belong to the descriptor?
    Same for #augmentVariableValues. */
inline bool checkValueOrder(
    PVariable const &var,
    const TVariable::TAttributeDescription &desc)
{
    if (desc.varType != TVariable::Discrete) {
        return true;
    }
    TDiscreteVariable *const dvar =
        dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
    return dvar && dvar->checkValuesOrder(desc.fixedOrderValues);
}


/*! Appends the missing values from the description to the descriptor. */
void augmentVariableValues(PVariable const &var,
                           const TVariable::TAttributeDescription &desc)
{
    if (desc.varType != TVariable::Discrete)
        return;
    
    TDiscreteVariable *const evar =
        dynamic_cast<TDiscreteVariable *>(var.borrowPtr());
    const_ITERATE(vector<string>, fvi, desc.fixedOrderValues) {
        evar->addValue(*fvi);
    }
    vector<string> sorted;
    set<string> values;
    map<string, int>::const_iterator dvi(desc.values.begin());
    map<string, int>::const_iterator const dve(desc.values.end());
    for(; dvi != dve; dvi++) {
        values.insert(values.end(), dvi->first);
    }
    TDiscreteVariable::presortValues(values, sorted);
    const_ITERATE(vector<string>, ssi, sorted) {
        evar->addValue(*ssi);
    }
}


/*! Checks whether the attributes, the class attribute and the meta
    attributes match the given domain description. Each variable's
    name and type must match the described and the values of
    discrete attributes must be in the right order when the order is
    prescribed. Any missing values are added.

    The function returns \c true on success.
*/
bool TDomain::makeCompatible(const TAttributeDescriptions *attrdescs,
                             bool const hasClass,
                             const TAttributeDescriptions *metadescs,
                             TMetaId *metaIDs)
{
    // check the number of attributes and meta attributes,
    // and the presence of class attribute
    if (    (variables->size() != attrdescs->size())
        ||  (bool(classVar) != hasClass)
        || (metadescs ? (metadescs->size() != metas.size())
                      : (metas.size() != 0) )
        ) {
            return false;
    }

    // check the names and types of attributes
    TVarList::const_iterator vi(variables->begin());
    TAttributeDescriptions::const_iterator ai(attrdescs->begin());
    TAttributeDescriptions::const_iterator const ae(attrdescs->end());
    for(; ai != ae; ai++, vi++) {
        if (    (ai->name != (*vi)->getName())
            || ((ai->varType>0) && (ai->varType != (*vi)->varType)
            // || (((**ai).varType==PYTHONVAR) &&
                // !pythonDeclarationMatches((**ai).typeDeclaration, *vi)
            )
            || !checkValueOrder(*vi, *ai)
            ) {
                return false;
        }
    }

    // check the meta attributes if they exist
    TAttributeDescriptions::const_iterator mi, me;
    if (metadescs) {
        for(mi = metadescs->begin(), me = metadescs->end(); mi != me; mi++) {
            PVariable var = getMetaVar(mi->name, false);
            if (    !var
                || ((mi->varType > 0) && (mi->varType != var->varType))
                //  || (((**mi).varType==PYTHONVAR) &&
                    //  !pythonDeclarationMatches((**mi).typeDeclaration, var)
                || !checkValueOrder(var, *mi)
                ) {
                    return false;
            }
            if (metaIDs) {
                *(metaIDs++) = getMetaNum(mi->name, false);
            }
        }
    }

    ai = attrdescs->begin(), vi = variables->begin();
    for(; ai != ae; ai++, vi++) {
        augmentVariableValues(*vi, *ai);
    }
    for(mi = metadescs->begin(); mi != me; mi++) {
        if (mi->varType == TVariable::Discrete) {
            augmentVariableValues(getMetaVar(mi->name), *mi);
        }
    }
    return true;
}


/*! Constructs a domain from the given attribute descriptions
    \param attributes Descriptions of attributes
    \param hasClass Indicates whether the last description refers to a class
    \param metas Descriptions for registered meta attributes
    \param createNewOn Defines the status at which a new variable is created
                       instead of reusing an existing (see TVariable::MakeStatus)
    \param status Creation stati for the variables in the domain
    \param metaStatus Creation stati for meta attributes
    */
PDomain TDomain::prepareDomain(TAttributeDescriptions const *const attributes,
                               bool const hasClass,
                               TAttributeDescriptions const *const metas,
                               int const createNewOn,
                               PIntList &status,
                               PIntIntList &metaStatus)
{ 
    status = PIntList(new TIntList);
    metaStatus = PIntIntList(new TIntIntList);
    PVarList attrList = PVarList(new TVarList);
    int tStatus;
    const_PITERATE(TAttributeDescriptions, ai, attributes) {
        attrList->push_back(TVariable::make(*ai, tStatus, createNewOn));
        status->push_back(tStatus);
    }
    PVariable classVar;
    if (hasClass) {
        classVar = attrList->back();
        attrList->erase(attrList->end()-1);
    }
    PDomain newDomain = PDomain(new TDomain(classVar, attrList));
    if (metas) {
        const_PITERATE(TAttributeDescriptions, mi, metas) {
            PVariable var = TVariable::make(*mi, tStatus, createNewOn);
            TMetaId id = var->defaultMetaId;
            if (!id) {
                id = getMetaID();
            }
            newDomain->metas.push_back(TMetaDescriptor(id, var));
            metaStatus->push_back(make_pair(id, tStatus));
        }
    }
    return newDomain;
}

/*! A function used by constructor that raises an exception if any variable
    in the list or the class variable (if given) is not primitive, that is,
    neither TContinuousVariable nor TDiscreteVariable. */
void TDomain::checkPrimitive(
    TVarList const &attributes, PVariable const &classVar)
{
    const_ITERATE(TVarList, vi, attributes) {
        if (!(*vi)->isPrimitive())
            raiseError(PyExc_TypeError,
            "Variable '%s' is neither discrete nor continuous",
            (*vi)->cname());
    }
    if (classVar && !classVar->isPrimitive()) {
        raiseError(PyExc_TypeError,
            "Class attribute '%s' is neither discrete nor continuous",
            classVar->cname());
    }
}

/*! Traverses the references to descriptors of meta attributes (#metas)
    that are not directly exposed to Python and covered by
    #TOrange::traverse_references. */
int TDomain::traverse_references(visitproc visit, void *arg)
{
    for(TMetaVector::const_iterator mi(metas.begin()), me(metas.end());
        mi!=me; mi++) {
            Py_VISIT(mi->variable.borrowPyObject());
    }
    return TOrange::traverse_references(visit, arg);
}


/*! Clears the references to descriptors of meta attributes (#metas)
    that are not directly exposed to Python and covered by
    #TOrange::traverse_references. */
int TDomain::clear_references()
{ 
    metas.clear();
    domainHasChanged();
    return TOrange::clear_references();
}


/*! A function that is called when the domain is changed, i.e. a variable is
    added, changed or removed. The function increases the version number,
    clears the #knownDomains vector, notifies the domains that know this domain
    and clears #knownByDomains.
*/
void TDomain::domainHasChanged()
{ 
    version = ++domainVersion;
    domainChangedDispatcher();
    knownDomains.clear();
    knownByDomains.clear();
    lastDomain = knownDomains.end();
}


/*! Returns the index of the variable or meta attribute corresponding to the
    descriptor. If it is not found, the function throws an exception or
    returns \c ILLEGAL_INT.
    \param var Variable descriptor
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
TAttrIdx TDomain::getVarNum(PVariable const &var, bool const throwExc) const
{
    TVarList::const_iterator const ve(variables->end());
    for(TVarList::const_iterator vb(variables->begin()), vi(vb); vi!=ve; vi++) {
        if (*vi == var) {
            return vi-vb;
        }
    }
    TMetaId pos = getMetaNum(var, false);
    if ((pos == ILLEGAL_INT) && throwExc) {
        raiseError(PyExc_KeyError, "attribute '%s' not found", var->cname());
    }
    return pos;
}


/*! Returns the index of the variable or meta attribute with the given name.
    If it is not found, the function throws an exception or returns
    \c ILLEGAL_INT.
    \param name Variable name
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
TAttrIdx TDomain::getVarNum(string const &name, bool const throwExc) const
{ 
    for(TVarList::const_iterator vb(variables->begin()), vi(vb), ve(variables->end());
        vi!=ve; vi++) {
            if ((*vi)->getName() == name) {
                return vi-vb;
            }
    }
    TMetaId pos = getMetaNum(name, false);
    if ((pos == ILLEGAL_INT) && throwExc) {
        raiseError(PyExc_KeyError, "attribute '%s' not found", name.c_str());
    }
    return pos;
}


/*! Returns the index of the variable or meta attribute indicated by the given
    Python object. The object can be a Python \c int (in which case the function
    checks the index), a descriptor (\c Variable) or a string. If the variable
    is not found, the function raises an exception or returns \c ILLEGAL_INT.
    \param arg Variable descriptor
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
TAttrIdx TDomain::getVarNum(PyObject *arg, bool const throwExc) const
{
    if (PyLong_Check(arg)) {
        TAttrIdx const idx = TAttrIdx(PyLong_AsLong(arg));
        if (idx >= variables->size()) {
            if (throwExc) {
	            raiseError(PyExc_IndexError,
                    "index out of range");
            }
            else {
                return ILLEGAL_INT;
            }
        }
        if ((idx == -1) && !classVar) {
            if (throwExc) {
                raiseError(PyExc_IndexError,
                    "index out of range (there is no class variable)");
            }
            else {
                return ILLEGAL_INT;
            }
        }
        return idx;
    }
    if (OrVariable_Check(arg)) {
        return getVarNum(PVariable(arg), throwExc);
    }
    else if (PyUnicode_Check(arg)) {
        return getVarNum(PyUnicode_As_string(arg), throwExc);
    }
    if (throwExc) {
        raiseError(PyExc_TypeError,
            "invalid index type ('%s')", arg->ob_type->tp_name);
    }
    return ILLEGAL_INT;
}


/*! Returns a descriptor for the variable with the given index. Index can also
    be -1, indicating the class attribute, or negative for meta attributes. In
    case of failure, the function throws an exception or returns \c NULL.
    \param idx Index
    \param throwExc Indicates whether the function should throw exception or
                    return \c NULL
*/
PVariable TDomain::getVar(TAttrIdx const idx, bool const throwExc) const
{ 
    if (idx >= 0) {
        if (idx >= int(variables->size())) {
            if (throwExc) {
                raiseError(PyExc_IndexError,
                    "index %i out of range", idx);
            }
            else {
                return PVariable();
            }
        }
        return variables->at(idx);
    }
    else if (idx == -1) {
        return classVar;
    }
    else {
        const_ITERATE(TMetaVector, mi, metas) {
            if (mi->id == idx)
                return mi->variable;
        }
        if (throwExc) {
            raiseError(PyExc_KeyError,
                "meta attribute with index %i not in domain", idx);
        }
        else {
            return PVariable();
        }
    }
    return PVariable();
}


/*! Returns a descriptor for the variable with the given name. In
    case of failure, the function throws an exception or returns \c NULL.
    \param name Variable name
    \param includeMetas Indicates whether also search the meta attributes
    \param throwExc Indicates whether the function should throw exception or
                    return \c NULL
*/
PVariable TDomain::getVar(const string &name, bool includeMetas, bool throwExc) const
{ 
    const_PITERATE(TVarList, vi, variables) {
        if ((*vi)->getName() == name) {
            return *vi;
        }
    }
    if (includeMetas) {
        const_ITERATE(TMetaVector, mi, metas) {
            if (mi->variable->getName() == name)
                return mi->variable;
        }
    }
    if (throwExc) {
        raiseError(PyExc_KeyError, "attribute '%s' not found", name.c_str());
    }
    return PVariable();
}


/*! Returns a descriptor for the variable indicated by the given Python object.
    The object can be a Python \c int (in which case the function
    checks the index), a descriptor (\c Variable) or a string. 
    In case of failure, the function throws an exception or returns \c NULL.
    \param arg A Python object describing the variable
    \param includeMetas Indicates whether also search the meta attributes
    \param throwExc Indicates whether the function should throw exception or
                    return \c NULL
    \param checkExistence Indicates whether to check that the variable belongs
                    to the domain if the Python object is already a descriptor.
*/
PVariable TDomain::getVar(PyObject *arg, bool const includeMetas, 
                          bool const throwExc, bool const checkExistence) const
{
    if (PyLong_Check(arg)) {
        TAttrIdx const idx = TAttrIdx(PyLong_AsLong(arg));
        if ((idx < -1) && !includeMetas) {
            if (throwExc) {
                raiseError(PyExc_KeyError, "invalid attribute index (%i)", idx);
            }
            else {
                return PVariable();
            }
        }
        return getVar(idx, throwExc);
    }
    if (OrVariable_Check(arg)) {
        PVariable var(arg);
        // The second condition will raise the exception if needed
        if (!checkExistence
            || (   (includeMetas && getMetaNum(var, false) != ILLEGAL_INT)
                || (getVarNum(var, throwExc) != ILLEGAL_INT))) {
            return var;
        }
        else {
            return PVariable();
        }
    }
    else if (PyUnicode_Check(arg)) {
        return getVar(PyUnicode_As_string(arg), includeMetas, throwExc);
    }
    if (throwExc) {
        raiseError(PyExc_TypeError,
            "invalid index type ('%s')", arg->ob_type->tp_name);
    }
    return PVariable();
}


/*! Returns the id of meta attribute with the given name. If it is not found,
    the function throws an exception or returns \c ILLEGAL_INT.
    \param name Variable name
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
TMetaId TDomain::getMetaNum(string const &name, bool const throwExc) const
{ 
    const_ITERATE(TMetaVector, mi, metas) {
        if (mi->variable->getName() == name) {
            return mi->id;
        }
    }
    if (throwExc) {
        raiseError(PyExc_KeyError, "meta attribute '%s' not found", name.c_str());
    }
    return ILLEGAL_INT;
}


/*! Returns the id of meta attribute corresponding to the descriptor.
    If it is not found, the function throws an exception or returns \c ILLEGAL_INT.
    \param var Variable descriptor
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
TMetaId TDomain::getMetaNum(PVariable const &var, bool const throwExc) const
{
    const_ITERATE(TMetaVector, mi, metas) {
        if (mi->variable == var) {
            return mi->id;
        }
    }
    if (throwExc) {
        raiseError(PyExc_KeyError, "meta attribute '%s' not found", var->cname());
    }
    return ILLEGAL_INT;
}

/*! Returns the id meta attribute indicated by the given Python object. The
    object can be a Python \c int (in which case the function checks it if
    \c checkId is \c true), a descriptor (\c Variable) or a string. If the attribute
    is not found, the function raises an exception or returns \c ILLEGAL_INT.
    \param arg Variable descriptor
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
    \param checkId Indicates whether to check the id if the Python object is
                    an \c int
*/
TMetaId TDomain::getMetaNum(PyObject *arg,
                            bool const throwExc,
                            bool const checkId) const
{
    if (PyLong_Check(arg)) {
        const int idx = PyLong_AsLong(arg);
        if (idx >= -1) {
            if (throwExc) {
                raiseError(PyExc_KeyError,
                    "invalid meta attribute index (%i)", idx);
            }
            else {
                return ILLEGAL_INT;
            }
        }
        if (checkId && !getMetaVar(idx, throwExc)) {
            return ILLEGAL_INT; // exception raised by getMetaVar, if needed
        }
        return idx;
    }

    if (OrVariable_Check(arg)) {
        return getMetaNum(PVariable(arg), throwExc);
    }
    else if (PyUnicode_Check(arg)) {
        return getMetaNum(PyUnicode_As_string(arg), throwExc);
    }
    if (throwExc) {
        raiseError(PyExc_TypeError,
            "invalid index type ('%s')", arg->ob_type->tp_name);
    }
    return ILLEGAL_INT;
}


/*! Returns descriptor of the meta attribute with the given id. If it
    is not found, the function raises an exception or returns \c ILLEGAL_INT.
    \param id Attribute id
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
PVariable TDomain::getMetaVar(TAttrIdx const id, bool const throwExc) const
{
    const_ITERATE(TMetaVector, mi, metas) {
        if (mi->id == id) {
            return mi->variable;
        }
    }
    if (throwExc) {
        raiseError(PyExc_KeyError,
            "meta attribute with index %i not found", id);
    }
    return PVariable();
}


/*! Returns descriptor of the meta attribute with the given name. If it
    is not found, the function raises an exception or returns \c ILLEGAL_INT.
    \param name Attribute name
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
*/
PVariable TDomain::getMetaVar(string const &name, bool const throwExc) const
{
    const_ITERATE(TMetaVector, mi, metas) {
        if (mi->variable->getName() == name) {
            return mi->variable;
        }
    }
    if (throwExc) {
        raiseError(PyExc_KeyError,
            "meta attribute '%s' not found", name.c_str());
    }
    return PVariable();
}


/*! Returns the id meta attribute indicated by the given Python object. The
    object can be a Python \c int, a descriptor (\c Variable) or a string.
    If the attribute is not found, the function raises an exception or returns
    \c ILLEGAL_INT.
    \param arg Variable descriptor
    \param throwExc Indicates whether the function should throw exception or
                    return \c ILLEGAL_INT
    \param checkExistence Indicates whether to check that the variable is in the
                    domain if the Python object is already a \c Variable.
*/
PVariable TDomain::getMetaVar(
    PyObject *arg, bool const throwExc, bool const checkExistence) const
{ 
    if (PyLong_Check(arg)) {
        const TMetaId idx = PyLong_AsLong(arg);
        if (idx >= -1) {
            if (throwExc) {
                raiseError(PyExc_KeyError,
                    "invalid meta attribute index (%i)", idx);
            }
            else {
                return PVariable();
            }
        }
        return getMetaVar(idx, throwExc);
    }
    if (OrVariable_Check(arg)) {
        PVariable var(arg);
        return !checkExistence || (getMetaNum(var, throwExc) != ILLEGAL_INT) ?
            var : PVariable();
    }
    else if (PyUnicode_Check(arg)) {
        return getMetaVar(PyUnicode_As_string(arg), throwExc);
    }
    if (throwExc) {
        raiseError(PyExc_TypeError,
            "invalid index type ('%s')", arg->ob_type->tp_name);
    }
    return PVariable();
}


/*!  Converts the example \c src, which may be from another domain to \c dest
     which is from this domain. If domains are the same, the values are copied.
     Otherwise a corresponding TDomainMapping is found or constructed if
     necessary. Conversion copies the values as defined in TDomainMapping,
     which may include computing the value from values of other attributes in
     the source domain.
     \param dest The destination example
     \param src The source example
     \param filterMetas If \c true, all unused meta attributes from the source
                        are copied to the destination
*/
void TDomain::convert(TExample *const dest, TExample const *const src,
                      bool const filterMetas)
{
    if (dest->referenceType == TExample::Indirect) {
        raiseError(PyExc_SystemError,
            "cannot convert to examples with indirect references to tables");
    }

    if (dest == src) {
        if (filterMetas && dest->supportsMeta()) {
            dest->removeMetas();
        }
        return;
    }

    TDomain *const srcDomain = src->domain.borrowPtr();
    int srcMetaHandle = src->getMetaHandle();
    if (srcDomain == this) {
        memcpy(const_cast<TValue *>(dest->values), src->values,
            dest->values_end-dest->values);
        if (dest->supportsMeta()) {
            int &destMetaHandle = dest->getMetaHandle();
            if (destMetaHandle) {
                dest->removeMetas();
            }
            if (!filterMetas) {
                MetaChain::copyChain(destMetaHandle, srcMetaHandle);
            }
        }
        else {
            if (!filterMetas && srcMetaHandle) {
                dest->getMetaHandle(); // just to raise the exception
            }
        }
        return;
    }

    if (lastDomain != knownDomains.end()) // if not, there are no known domains (otherwise lastDomain would point to one)
        if (srcDomain != lastDomain->domain)
            for(lastDomain = knownDomains.begin();
                (lastDomain != knownDomains.end()) && (lastDomain->domain != srcDomain);
                lastDomain++);

    // no domains or no src.domain:
    // - construct mapping if none of the two domains is anonymous
    // - instead just check that they are compatible and construct an empty mapping
    if (lastDomain == knownDomains.end()) {
        knownDomains.push_back(TDomainMapping(srcDomain));
        if (anonymous || srcDomain->anonymous) {
            TVarList::const_iterator v1i(variables->begin());
            TVarList::const_iterator v2i(srcDomain->variables->begin());
            TVarList::const_iterator const v1e(variables->end());
            TVarList::const_iterator const v2e(srcDomain->variables->end());
            for(; (v1i != v1e) && (v2i != v2e) && ((*v1i)->varType == (*v2i)->varType);
                v1i++, v2i++);
            if ((v1i != v1e) || (v2i != v2e)) {
                raiseError(PyExc_ValueError,
                    "cannot convert to/from anonymous (domain) with "
                    "mismatching number or types of variables");
            }

        }
        else {
            const_cast<TDomain *>(srcDomain)->knownByDomains.push_back(this);
            lastDomain = knownDomains.end();
            lastDomain--;

            const_PITERATE(TVarList, vi, variables) {
                const int cvi = srcDomain->getVarNum(*vi, false);
                lastDomain->positions.push_back(cvi);
                if ((cvi != ILLEGAL_INT) && (cvi<0)) {
                    lastDomain->metasNotToCopy.insert(cvi);
                }
            }

            ITERATE(TMetaVector, mvi, metas) {
                const int cvi = srcDomain->getVarNum(mvi->variable, false);
                lastDomain->metaPositions.push_back(make_pair(TMetaId((*mvi).id), cvi));
                if ((cvi != ILLEGAL_INT) && (cvi<0)) {
                    lastDomain->metasNotToCopy.insert(cvi);
                }
            }

            /* I don't get this: why are some metas inserted in above two blocks if
               if all src's metas are unwanted anyway and are thus inserted here below?! */
            const_ITERATE(TMetaVector, mvio, srcDomain->metas) {
                lastDomain->metasNotToCopy.insert(mvio->id);
            }
        }
    }

    if (anonymous || srcDomain->anonymous) {
        copy(src->begin(), src->end(), dest->begin());
    }
    else {
        // Now, lastDomain points to an appropriate mapping
        vector<int>::iterator pi(lastDomain->positions.begin());
        TVarList::iterator vi(variables->begin());

        TExample::iterator deval(dest->begin());
        for(int Nv = dest->domain->variables->size(); Nv--; pi++, vi++)
            *deval++ = (*pi == ILLEGAL_INT) ? (*vi)->computeValue(src) : (*src)[*pi];

        TMetaVector::iterator mvi(metas.begin());
        vector<pair<TMetaId, TMetaId> >::const_iterator
            vpii(lastDomain->metaPositions.begin()),
            vpie(lastDomain->metaPositions.end());
        for(; vpii!=vpie; vpii++, mvi++) {
            if (!mvi->optional) {
                if (vpii->second == ILLEGAL_INT) {
                    dest->setMeta(vpii->first, mvi->variable->computeValue(src));
                }
                else if ((vpii->second >= 0) || mvi->variable->isPrimitive()) {
                    dest->setMeta(vpii->first, (*src)[vpii->second]);
                }
                else {
                    TMetaValue const &mv = src->getMetaIfExists(vpii->second);
                    if (mv.id) {
                        dest->setMeta(mv);
                    }
                    else {
                        dest->setMeta(vpii->second, (PyObject *)NULL);
                    }
                }
            }
            /* Some code repetition from above, but we'd like to make the conversion code fast:
                we do basically the same as above except that we check whether the value is defined
                before setting it */
            else {
                if (vpii->second != ILLEGAL_INT) {
                    if ((vpii->second >= 0) || mvi->variable->isPrimitive()) {
                        TValue const &val = (*src)[vpii->second];
                        if (!isnan(val)) {
                            dest->setMeta(vpii->first, val);
                        }
                    }
                    else {
                        PyObject **obj = 
                            MetaChain::getObjectPtr(srcMetaHandle, vpii->second);
                        if (obj) {
                            dest->setMeta(vpii->first, *obj);
                        }
                    }
                }
                if (mvi->variable->getValueFrom) {
                    TValue val = mvi->variable->computeValue(src);
                    if (!isnan(val)) {
                        dest->setMeta(vpii->first, val);
                    }
                }
            }
        }
    }
    
    if (!filterMetas) {
        const set<int>::iterator mend = lastDomain->metasNotToCopy.end();
        while(srcMetaHandle) {
            TMetaValue mr = MetaChain::get(srcMetaHandle);
            if (lastDomain->metasNotToCopy.find(mr.id) == mend) {
                dest->setMeta(mr);
            }
            srcMetaHandle = MetaChain::advance(srcMetaHandle);
        }
    }
}


/*! Notifies the domains from #knownDomains and #knownByDomains that this
    domain has changed.
    \todo Why knownDomains?!
*/
void TDomain::domainChangedDispatcher()
{ 
    ITERATE(list<TDomainMapping>, di, knownDomains) {
        (*di).domain->domainChangedNoticeHandler(this);
    }
    ITERATE(list<TDomain *>, ki, knownByDomains) {
        (*ki)->domainChangedNoticeHandler(this);
    }
}

/*! Gets the notice from other domains that they have changed. The function
    removes the domain from #knownDomains and resets #lastDomain if necessary. */
void TDomain::domainChangedNoticeHandler(TDomain *const dom)
{
    bool rld = (lastDomain == knownDomains.end()) || (dom == lastDomain->domain);
    list<TDomainMapping>::iterator li(knownDomains.begin()), ln, le(knownDomains.end()); 
    while(li!=le) {
        if (dom == (ln=li++)->domain) {
            knownDomains.erase(ln);
        }
    }
    if (rld) {
        lastDomain = knownDomains.end();
        if (knownDomains.size()) {
            lastDomain--;
        }
    }
    knownByDomains.remove(dom);
}


/*! Returns \c true if the domain has any discrete variables. The class is
    checked if \c checkClass is \c true. Meta attributes are ignored. */
PVariable TDomain::hasDiscreteAttributes(bool const checkClass) const
{
    PVarList const &toCheck = checkClass ? variables : attributes;
    const_PITERATE(TVarList, vi, toCheck) {
        if ((*vi)->varType == TVariable::Discrete)
            return *vi;
    }
    return PVariable();
}


/*! Returns \c true if the domain has any continuous variables. The class is
    checked if \c checkClass is \c true. Meta attributes are ignored. */
PVariable TDomain::hasContinuousAttributes(bool const checkClass) const
{
    PVarList const &toCheck = checkClass ? variables : attributes;
    const_PITERATE(TVarList, vi, toCheck) {
        if ((*vi)->varType == TVariable::Continuous)
            return *vi;
    }
    return PVariable();
}


/*! Adds the domain to CRC adding the name, varType and values for all
    variables. Meta attributes are ignored. */
void TDomain::addToCRC(unsigned int &crc) const
{
    const_PITERATE(TVarList, vi, variables) {
        add_CRC((*vi)->cname(), crc);
        add_CRC((const unsigned char)(*vi)->varType, crc);
        if ((*vi)->varType == TVariable::Discrete) {
            PITERATE(TStringList, vli, PDiscreteVariable::cast(*vi)->values) {
                add_CRC(vli->c_str(), crc);
            }
        }
    }
}


/*! Computes a CRC32 for the domain; used to compute a hash value for Python. */
unsigned int TDomain::sumValues() const
{ 
    unsigned int crc;
    INIT_CRC(crc);
    addToCRC(crc);
    FINISH_CRC(crc);
    return crc & 0x7fffffff;
}


/// @cond Python

TOrange *TDomain::__new__(PyTypeObject *type, PyObject *args, PyObject *kw)
/*
Constructor creates a new domain with the given attributes, class variable
and meta attributes. If the optional source argument is given, attributes
and class may also be given as names.
*/
{
    PyObject *arg_attributes;
    PyObject *arg_class = NULL;
    PyObject *arg_source = NULL;
    PyObject *arg_metas = NULL; // for unpickling purposes only!

    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|OOO!:Domain",
        Domain_keywords, &arg_attributes,
        &arg_class, &arg_source, &PyList_Type, &arg_metas)) {
            return NULL;
    }
    if (OrDomain_Check(arg_attributes)) {
        PDomain old_domain(arg_attributes);
        if (arg_class) {
            if (PyUnicode_Check(arg_class) || OrVariable_Check(arg_class)) {
                PVariable classVar =
                    varFromArg_byDomain(arg_class, old_domain, false);
                if (!classVar) {
                    return NULL;
                }
                PVarList attributes = old_domain->variables;
                TAttrIdx vnumint = old_domain->getVarNum(classVar, false);
                if (vnumint >= 0) {
                    attributes->erase(attributes->begin()+vnumint);
                }
                return new(type) TDomain(classVar, attributes);
            }
            else if (PyLong_Check(arg_class) || (arg_class==Py_None)) {
                if (PyObject_IsTrue(arg_class)) {
                    return old_domain->clone();
                }
                else {
                    return new(type) TDomain(PVariable(), old_domain->variables);
                }
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "Domain: invalid arguments for constructor");
                return NULL;
            }
        }
        return old_domain->clone();
    }

    /* Now, arg_class can be either
    - NULL
    - source (i.e. Domain or list of variables)
    - boolean that tells whether we have a class
    - class variable
    If arg1 is present but is not source, arg2 can be source
    */

    if (arg_class && 
        (OrDomain_Check(arg_class) || OrVarList_Check(arg_class) || PyList_Check(arg_class))) {
            if (arg_source) {
                PyErr_Format(PyExc_TypeError,
                    "Domain: invalid arguments, source is given twice");
                return NULL;
            }
            arg_source = arg_class;
            arg_class = NULL;
    }

    PVarList source;
    PVariable classVar;
    bool hasClass = true;
    if (arg_source && (arg_source != Py_None)) {
        if (OrDomain_Check(arg_source)) {
            source = PDomain(arg_source)->variables;
        }
        else if (OrVarList_Check(arg_source)) {
            source = PVarList(arg_source);
        }
        else if (PyList_Check(arg_source)) {
            PyObject *pyvarlist = TVarList::newFromArgument(&OrVarList_Type, arg_source);
            source = PVarList(pyvarlist);
            Py_DECREF(pyvarlist);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                "'Domain' or list of attributes expected as 'source'");
            return NULL;
        }
    }
    if (arg_class) {
        if (OrVariable_Check(arg_class)) {
            classVar = PVariable(arg_class);
        }
        else if (PyUnicode_Check(arg_class)) {
            classVar = TDomain::varFromArg_byVarList(arg_class, source);
            if (!classVar)
                return NULL;
        }
        else {
            hasClass = (PyObject_IsTrue(arg_class) != 0);
        }
    }

    PVarList variables;
    if (!TDomain::varListFromVarList(arg_attributes, source, variables, true, false)) {
        return NULL;
    }
    if (hasClass && !classVar && variables->size()) {
        classVar = variables->back();
        variables->erase(variables->end()-1);
    }

    PDomain newDomain(new(type) TDomain(classVar, variables));
    // Unpickling: metas
    if (arg_metas) {
        Py_ssize_t sze = PyList_Size(arg_metas);
        newDomain->metas.reserve(sze);
        TMetaId id;
        PVariable var;
        int opt;
        Py_ssize_t i;
        for(i = 0; i < sze; i++) {
            if (!PyArg_ParseTuple(PyList_GET_ITEM(arg_metas, i),
                "iO&i:meta_unpickler", &id, &PVariable::argconverter, &var, &opt)) {
                return NULL;
            }
            newDomain->metas.push_back(TMetaDescriptor(id, var, opt));
        }
    }
    return newDomain.getPtr();
}


PyObject *TDomain::__getnewargs__() const
{
    PyObject *metalist = PyList_New(metas.size());
    Py_ssize_t i = 0;
    const_ITERATE(TMetaVector, mi, metas) {
        PyList_SetItem(metalist, i++, Py_BuildValue("(iNi)", mi->id,
            mi->variable.toPython(), mi->optional));
    }
    return Py_BuildValue("(NNON)",
        attributes.getPyObject(), classVar.toPython(), Py_None, metalist);
}


Py_ssize_t TDomain::__len__() const
{
    return variables->size();
}


int TDomain::__contains__(PyObject *arg) const
{
    PVariable var = TDomain::varFromArg_byDomain(
        arg, PDomain::fromBorrowedPtr(const_cast<TDomain *>(this)), true, false, false);
    return var ? 1 : 0;
}


PyObject *TDomain::__item__(Py_ssize_t index) const
{ 
    if ((index < 0) || (index >= variables->size())) {
        return PyErr_Format(PyExc_IndexError, "index %i is out of range", index);
    }
    return variables->at(index).getPyObject();
}


PyObject *TDomain::__subscript__(PyObject *index) const
{ 
    if (PySlice_Check(index)) {
        PySliceObject *slice = (PySliceObject *)(index);
        if (   (PyLong_Check(slice->start) && (PyLong_AsLong(slice->start) < 0))
            || (PyLong_Check(slice->stop) && (PyLong_AsLong(slice->stop) < 0))) {
                return PyErr_Format(PyExc_IndexError,
                    "Domain slices cannot have negative indices (meta attributes)");
        }
        Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
        if (PySlice_GetIndicesEx((PyObject *)slice, variables->size(),
            &start, &stop, &step, &slicelength) < 0) {
                return NULL;
        }
#else
        if (PySlice_GetIndicesEx(slice, variables->size(),
            &start, &stop, &step, &slicelength) < 0) {
                return NULL;
        }
#endif
        PyObject *res = PyList_New(slicelength);
        for(int i = 0;
            step > 0 ? start < stop : start > stop;
            start += step, i++) {
                PyList_SetItem(res, i, variables->at(start).getPyObject());
        }
        return res;
    }
    PVariable var = varFromArg_byDomain(index,
        PDomain::fromBorrowedPtr(const_cast<TDomain *>(this)), true);
    return var.getPyObject();
}

const string noname = string("<noname>");

const string &getName(TVariable *var)
{
    return var->getName().size() ? var->getName() : noname;
}

PyObject *TDomain::__repr__() const
{
    string res = "<";
    res += THIS_AS_PyObject->ob_type->tp_name;
    res += " [";
    int added = 0;
    PITERATE(TVarList, vi, attributes) {
        if (added==20) {
            res += ", ...";
            break;
        }
        if (added++) {
            res += ", ";
        }
        res += (*vi)->getName();
    }
    if (classVar) {
        if (!added) {
            res += "(no attrs)";
        }
        res += " -> ";
        if (classVar) {
            res += classVar->getName();
        }
    }
    else {
        if (added) {  // if there is no class and no attributes, we output []
            res += " -> (no class)";
        }
    }
    res += "]";
    if (metas.size()) {
        if (added) {
            res += " ";
        }
        res += "{";
        added = 0;
        char pls[256];
        const_ITERATE(TMetaVector, mi, metas) {
            snprintf(pls, 256, "%s%i:%s",
                added++ ? ", " : "", int(mi->id), mi->variable->cname());
            res += pls;
        }
        res += "}";
    }
    res += ">";
    return PyUnicode_FromString(res.c_str());
}


PyObject *TDomain::__str__() const
{ 
    return __repr__();
}



TDataColumns::TDataColumns(PDomain const &dom)
    : domain(dom)
{}

bool TDataColumns::matchesRenamedVar(string const &pyname, string const &varname)
{
    string::const_iterator pi(pyname.begin()), vi(varname.begin());
    const string::const_iterator pe(pyname.end()), ve(varname.end());
    if ((pi == pe) || (vi == ve)) {
        return false;
    }
    if ((*vi >= '0') && (*vi <= '9')) {
        if (*pi++ != '_') {
            return false;
        }
    }
    for(; (pi != pe) && (vi != ve); pi++, vi++) {
        if (! ((*vi >= '0') && (*vi <= '9') ||
                (*vi >= 'A') && (*vi <= 'Z') ||
                (*vi >= 'a') && (*vi <= 'z'))) {
                    if (*pi != '_') {
                        return false;
                    }
        }
        else {
            if (*pi != *vi) {
                return false;
            }
        }
    }
    return (pi == pe) && (vi == ve);
}

PyObject *TDataColumns::__getattr__(PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        return PyErr_Format(PyExc_TypeError,
            "variable name must be string, not '%s'", name->ob_type->tp_name);
    }
    string sname = PyUnicode_As_string(name);
    PVariable var = domain->getVar(name, true, false);
    if (var) {
        return var.getPyObject();
    }
    PITERATE(TVarList, vi, domain->variables) {
        if (matchesRenamedVar(sname, (*vi)->getName())) {
            return (*vi).getPyObject();
        }
    }
    ITERATE(TMetaVector, mi, domain->metas) {
        if (matchesRenamedVar(sname, mi->variable->getName())) {
            return mi->variable.getPyObject();
        }
    }
    return PyErr_Format(PyExc_AttributeError,
        "Domain has no variable '%s'", sname.c_str());
}

PyObject *TDomain::__get__columns(PyObject *self)
{
    return PDataColumns(new TDataColumns(PDomain(self))).getPyObject();
}


    // Given a parameter from Python and a domain, it returns a variable.
// Python's parameter can be a string name, an index or Variable
PVariable TDomain::varFromArg_byDomain(PyObject *obj,
                                       PDomain const &domain,
                                       bool const checkForIncludance,
                                       bool const noIndex,
                                       bool const throwExc)
{ 
    if (domain) {
        if (PyUnicode_Check(obj)) {
            string const name = PyUnicode_As_string(obj);
            PVariable res = domain->getVar(name, true, false);
            if (!res && throwExc) {
                raiseError(PyExc_IndexError,
                    "domain has no variable '%s'", name.c_str());
            }
            return res;
        }
        if (!noIndex && PyLong_Check(obj)) {
            TAttrIdx idx = PyLong_AsLong(obj);
            if (idx == -1) {
                if (!domain->classVar) {
                    if (throwExc) {
                        raiseError(PyExc_IndexError, "domain has no class");
                    }
                    return PVariable();
                }
                return domain->classVar;
            }
            if (idx < 0) {
                PVariable res = domain->getMetaVar(idx, false);
                if (!res && throwExc) {
                    raiseError(PyExc_IndexError,
                        "data has no meta attribute %i", idx);
                }
                return res;
            }
            if (idx >= domain->variables->size()) {
                if (throwExc) {
                    raiseError(PyExc_IndexError,
                        "index %i out of range", idx);
                }
                return PVariable();
            }
            return domain->getVar(idx);
        }
    }
    if (OrVariable_Check(obj)) {
        PVariable var(obj);
        if (checkForIncludance)
            if (!domain || (domain->getVarNum(var, false)==ILLEGAL_INT)) {
                if (throwExc) {
                    raiseError(PyExc_IndexError,
                        "domain has no variable '%s'", var->cname());
                }
                return PVariable();
            }
        return var;
    }
    raiseError(PyExc_TypeError,
        "cannot convert '%s' to variable", obj->ob_type->tp_name);
    return PVariable();
}


// Given a parameter from Python and a list of variables, it returns a variable.
// Python's parameter can be a string name, an index or Variable
PVariable TDomain::varFromArg_byVarList(PyObject *obj,
                                        PVarList const &varlist,
                                        bool const checkForIncludance,
                                        bool const noIndex)
{ 
    if (varlist) {
        if (PyUnicode_Check(obj)) {
            string const s = PyUnicode_As_string(obj);
            TVarList::const_iterator fi(varlist->begin()), fe(varlist->end());
            for(; (fi!=fe) && ((*fi)->getName() != s); fi++);
            if (fi==fe) {
                raiseError(PyExc_IndexError,
                    "domain has no variable '%s'", s.c_str());
            }
            else {
                return *fi;
            }
        }
    }
    if (OrVariable_Check(obj)) {
        PVariable var(obj);
        if (checkForIncludance) {
            TVarList::const_iterator fi(varlist->begin()), fe(varlist->end());
            for(; (fi!=fe) && (*fi != var); fi++);
            if (fi==fe) {
                raiseError(PyExc_IndexError,
                    "domain has no variable '%s'", var->cname());
            }
        }
        return var;
    }
    if (!noIndex && PyLong_Check(obj)) {
        int idx = (int)PyLong_AsLong(obj);
        if (idx < 0) {
            idx += varlist->size();
        }
        if ((idx < 0) || (idx >= varlist->size())) {
            raiseError(PyExc_IndexError,
                "index %i out of range", idx);
        }
        return varlist->at(idx);
    }
    raiseError(PyExc_TypeError,
        "cannot convert '%s' to variable", obj->ob_type->tp_name);
    return PVariable();
}


bool TDomain::varListFromVarList(PyObject *arg_subset,
                                 PVarList const &source,
                                 PVarList &res,
                                 bool const allowSingle,
                                 bool const checkForIncludance)
{
    try {
        if (OrVarList_Check(arg_subset)) {
            PVarList variables(arg_subset);
            if (checkForIncludance) {
                const_PITERATE(TVarList, vi, variables) {
                    TVarList::const_iterator fi(source->begin()), fe(source->end());
                    for(; (fi!=fe) && (*fi != *vi); fi++);
                    if (fi==fe) {
                        PyErr_Format(PyExc_IndexError,
                            "data has no variable '%s'", (*vi)->cname());
                        res = PVarList();
                        return false;
                    }
                }
            }
            res = CLONE(PVarList, variables);
            return true;
        }
        if (PySlice_Check(arg_subset)) {
            Py_ssize_t start, stop, step, slicelength;
#if PY_VERSION_HEX >= 0x03020000
            if (PySlice_GetIndicesEx(arg_subset, source->size(),
#else
            if (PySlice_GetIndicesEx((PySliceObject *)index, size(),
#endif
                     &start, &stop, &step, &slicelength) < 0) {
                return NULL;
            }
            res = PVarList(new TVarList());
            int si, i;
            for(si = start, i = slicelength; i--; si += step) {
                res->push_back(source->at(si));
            }
            return res;
        }
        if (PySequence_Check(arg_subset) && !PyUnicode_Check(arg_subset)) {
            PyObject *iter = PyObject_GetIter(arg_subset);
            if (!iter) {
                res = PVarList();
                return false;
            }
            GUARD(iter);
            PyObject *item;
            res = PVarList(new TVarList());
            while((item = PyIter_Next(iter)) != NULL) {
                PyObject *it2 = item; // ensure it dies in every iteration
                GUARD(it2);
                PVariable variable = 
                    varFromArg_byVarList(item, source, checkForIncludance);
                if (!variable) {
                    res = PVarList();
                    return false;
                }
                res->push_back(variable);
            }
            return true;
        }
        if (allowSingle) {
            PVariable variable =
                varFromArg_byVarList(arg_subset, source, checkForIncludance);
            if (!variable) {
                res = PVarList();
                return false;
            }
            res = PVarList(new TVarList());
            res->push_back(variable);
            return true;
        }
    } PyCATCH_r(false);
    PyErr_Format(PyExc_TypeError, "invalid parameters for list of variables");
    return false;
}


TMetaDescriptor *TDomain::metaDescriptorFromArg(PyObject *rar)
{
    TMetaDescriptor *desc = NULL;
    if (PyUnicode_Check(rar)) {
        desc = metas[string(PyUnicode_As_string(rar))];
    }
    else if (OrVariable_Check(rar)) {
        desc = metas[PVariable(rar)->getName()];
    }
    else if (PyLong_Check(rar)) {
        desc = metas[(int)PyLong_AsLong(rar)];
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "invalid meta descriptor (%s)", rar->ob_type->tp_name);
        return NULL;
    }
    if (!desc) {
        PyErr_Format(PyExc_IndexError, "meta attribute does not exist");
        return NULL;
    }
    return desc;
}


PyObject *TDomain::meta_id(PyObject *rar) const
{ 
    const TMetaDescriptor *desc = metaDescriptorFromArg(rar);
    return desc ? PyLong_FromLong(desc->id) : NULL;
}


PyObject *TDomain::isOptionalMeta(PyObject *rar) const
{
    const TMetaDescriptor *desc = metaDescriptorFromArg(rar);
    return desc ? PyBool_FromBool(desc->optional != 0) : NULL;
}


PyObject *TDomain::has_meta(PyObject *rar) const
{
    TMetaDescriptor const *desc = NULL;
    if (PyUnicode_Check(rar)) {
        desc = metas[PyUnicode_As_string(rar)];
    }
    else if (OrVariable_Check(rar)) {
        desc = metas[PVariable(rar)->getName()];
    }
    else if (PyLong_Check(rar)) {
        desc = metas[(int)PyLong_AsLong(rar)];
    }
    else {
      return PyErr_Format(PyExc_TypeError,
          "invalid meta descriptor (%s)", rar->ob_type->tp_name);
    }
    return PyBool_FromBool(desc != NULL);
}



PyObject *TDomain::get_meta(PyObject *rar) const
{ 
    TMetaDescriptor const *desc = metaDescriptorFromArg(rar);
    return desc ? desc->variable.toPython() : NULL;
}


PyObject *TDomain::getmetasLow() const
{
    PyObject *dict = PyDict_New();
    const_ITERATE(TMetaVector, mi, metas) {
        PyDict_SetItem(dict, PyLong_FromLong(mi->id), mi->variable.getPyObject());
    }
    return dict;
}


PyObject *TDomain::getmetasLow(const int optional) const
{
    PyObject *dict = PyDict_New();
    const_ITERATE(TMetaVector, mi, metas) {
        if (optional == (*mi).optional) {
            PyDict_SetItem(dict, PyLong_FromLong(mi->id), mi->variable.getPyObject());
        }
    }
    return dict;
}


PyObject *TDomain::get_metas(PyObject *args, PyObject *kw) const
{ 
    if (PyTuple_Size(args) && (PyTuple_GET_ITEM(args, 0) != Py_None)) {
        int opt;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i:get_metas",
            Domain_get_metas_keywords, &opt)) {
                return NULL;
        }
        return getmetasLow(opt);
    }

    return getmetasLow();
}


PyObject *TDomain::add_meta(PyObject *args, PyObject *kw)
{
    int id;
    PVariable var;
    int opt = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "iO&|i:add_meta",
        Domain_add_meta_keywords,
        &id, &PVariable::argconverter, &var, &opt)) {
        return NULL;
    }
    metas.push_back(TMetaDescriptor(id, var, opt));
    domainHasChanged();
    Py_RETURN_NONE;
}


bool TDomain::convertMetasFromPython(PyObject *dict, TMetaVector &metas)
{
    Py_ssize_t pos = 0;
    PyObject *pykey, *pyvalue;
    while (PyDict_Next(dict, &pos, &pykey, &pyvalue)) {
        if (!PyLong_Check(pykey) || ((int)PyLong_AsLong(pykey)>=0)) {
            PyErr_Format(PyExc_TypeError,
                "parsing meta attributes: dictionary keys should be meta-ids");
            return false;
        }
        if (!OrVariable_Check(pyvalue)) {
            PyErr_Format(PyExc_TypeError,
                "parsing meta attributes: dictionary value at position '%i' should be 'Variable', not '%s'",
                pos-1, pyvalue->ob_type->tp_name);
            return false;
        }
        metas.push_back(TMetaDescriptor((TMetaId)PyLong_AsLong(pykey), PVariable(pyvalue)));
    }
    return true;
}


PyObject *TDomain::addmetasLow(PyObject *dict, const int opt)
{
    TMetaVector tmetas;
    if (!convertMetasFromPython(dict, tmetas)) {
        return NULL;
    }
    ITERATE(TMetaVector, mi, tmetas) {
        mi->optional = opt;
        metas.push_back(*mi);
    }
    domainHasChanged();
    Py_RETURN_NONE;
}


PyObject *TDomain::add_metas(PyObject *args, PyObject *kw)
{
    PyObject *pymetadict;
    int opt = 0;
    if (!PyArg_ParseTuple(args, "O!|i:addmetas",
        &PyDict_Type, &pymetadict, &opt)) {
            return NULL;
    }
    return addmetasLow(pymetadict, opt);
}


bool TDomain::removeMeta(PyObject *rar, TMetaVector &metas)
{
    TMetaVector::iterator mvi(metas.begin()), mve(metas.end());

    if (PyLong_Check(rar)) {
        int id = (int)PyLong_AsLong(rar);
        while((mvi!=mve) && ((*mvi).id!=id)) {
            mvi++;
        }
    }
    else if (OrVariable_Check(rar)) {
        while((mvi!=mve) && (mvi->variable != PVariable(rar))) {
            mvi++;
        }
    }
    else if (PyUnicode_Check(rar)) {
        string metaname = PyUnicode_As_string(rar);
        while((mvi!=mve) && (mvi->variable->getName() != metaname)) {
            mvi++;
        }
    }
    else {
        mvi=mve;
    }
    if (mvi==mve) {
        PyErr_Format(PyExc_IndexError, "meta value not found");
        return false;
    }
    metas.erase(mvi);
    return true;
}


PyObject *TDomain::remove_meta(PyObject *rar)
{
    if (PyDict_Check(rar)) {
        Py_ssize_t pos=0;
        PyObject *key, *value;
        TMetaVector newMetas = metas;
        TMetaVector::iterator mve = metas.end();
        while (PyDict_Next(rar, &pos, &key, &value)) {
            if (!PyLong_Check(key) || !OrVariable_Check(value)) {
                return PyErr_Format(PyExc_AttributeError, "invalid arguments");
            }
            TMetaId idx = PyLong_AsLong(key);
            TMetaVector::iterator mvi(newMetas.begin());
            PVariable var = PVariable(value);
            for(; (mvi!=mve) && ( (mvi->id!=idx) || mvi->variable != var); mvi++);
            if (mvi==mve) {
                return PyErr_Format(PyExc_AttributeError, "meta attribute not found");
            }
            newMetas.erase(mvi);
        }
        metas = newMetas;
        domainHasChanged();
    }
    else if (PyList_Check(rar)) {
        TMetaVector newMetas = metas;
        for(Py_ssize_t pos=0, noel=PyList_Size(rar); pos!=noel; pos++) {
            if (!TDomain::removeMeta(PyList_GetItem(rar, pos), newMetas)) {
                return NULL;
            }
        }
        metas=newMetas;
        domainHasChanged();
    }
    else if (!TDomain::removeMeta(rar, metas)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


PyObject *TDomain::py_hasDiscreteAttributes(PyObject *args, PyObject *kw) const
{
    int includeClass = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|i:has_discrete_attributes",
        Domain_hasDiscreteAttributes_keywords, &includeClass)) {
            return NULL;
    }
    PVariable discreteAttr = hasDiscreteAttributes(includeClass != 0);
    return PyBool_FromBool(discreteAttr);
}


PyObject *TDomain::py_hasContinuousAttributes(PyObject *args, PyObject *kw) const
{
    int includeClass = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|i:has_continuous_attributes",
        Domain_hasContinuousAttributes_keywords, &includeClass)) {
            return NULL;
    }
    PVariable continuousAttr = hasContinuousAttributes(includeClass != 0);
    return PyBool_FromBool(continuousAttr);
}


PyObject *TDomain::index(PyObject *arg) const
{
    PVariable variable = varFromArg_byDomain(arg,
        PDomain::fromBorrowedPtr(const_cast<TDomain *>(this)), true);
    return variable ? PyLong_FromLong(getVarNum(variable)) : NULL;
}


PyObject *TDomain::checksum() const
{ 
    return PyLong_FromLong(sumValues()); 
}


PyObject *TDomain::__call__(PyObject *args, PyObject *kw)
{
    PyObject *pyexample;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O:Domain",
        Domain_call_keywords, &pyexample)) {
            return NULL;
    }
    PDomain me = PDomain::fromBorrowedPtr(this);
    PExample ex = TExample::fromDomainAndPyObject(me, pyexample, false);
    if (ex->domain == this) {
        return ex.getPyObject();
    }
    PExample newEx = TExample::constructFree(me);
    convert(newEx.borrowPtr(), ex.borrowPtr(), false);
    return newEx.getPyObject();
}


PyObject *newmetaid(PyObject *, PyObject *args, PyObject  *kw)
{ 
    try {
        PVariable var;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|O&:newmetaid",
            newmetaid_keywords, &PVariable::argconverter_n, &var)) {
                return NULL;
        }
        return PyLong_FromLong(getMetaID(var));
    }
    PyCATCH
}

/// @endcond

TMetaDescriptor::TMetaDescriptor()
: id(ILLEGAL_INT),
  optional(0)
{}


TMetaDescriptor::TMetaDescriptor(TMetaId const ai, PVariable const &avar, int const opt)
: id(ai),
  variable(avar),
  optional(opt)
{
    if (!avar->defaultMetaId) {
        avar->defaultMetaId = id;
    }
}


TMetaDescriptor::TMetaDescriptor(const TMetaDescriptor &old)
: id(old.id),
  variable(old.variable),
  optional(old.optional)
{}


TMetaDescriptor &TMetaDescriptor::operator =(const TMetaDescriptor &old)
{
    if (this != &old) {
        id = old.id;
        variable = old.variable;
        optional = old.optional;
    }
    return *this;
}


TMetaDescriptor *TMetaVector::operator[](PVariable const &var)
{
    this_ITERATE(mi) {
        if (mi->variable == var) {
            return &*mi;
        }
    }
    return NULL; 
}


TMetaDescriptor const *TMetaVector::operator[](PVariable const &var) const
{
    const_this_ITERATE(mi) {
        if (mi->variable == var) {
            return &*mi;
        }
    }
    return NULL; 
}


TMetaDescriptor *TMetaVector::operator[](string const &sna) 
{
    this_ITERATE(mi) {
        if (mi->variable->getName() == sna) {
            return &*mi;
        }
    }
    return NULL; 
}


TMetaDescriptor const *TMetaVector::operator[](string const &sna) const
{
    const_this_ITERATE(mi) {
        if (mi->variable->getName() == sna) {
            return &*mi;
        }
    }
    return NULL; 
}


TMetaDescriptor *TMetaVector::operator[](TMetaId const ai) 
{ 
    this_ITERATE(mi) {
        if (mi->id == ai) {
            return &*mi;
        }
    }
    return NULL;
}


TMetaDescriptor const *TMetaVector::operator[](TMetaId const ai) const
{ 
    const_this_ITERATE(mi) {
        if (mi->id==ai) {
            return &*mi;
        }
    }
    return NULL; 
}


