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
#include "filter.px"

TFilter::TFilter(bool const anegate) 
: negate(anegate)
{}


TFilter::TFilter(PDomain const &dom, bool const anegate) 
: negate(anegate),
  domain(dom)
{}


bool TFilter::operator()(PExample const &ex)
{
    return (*this)(ex.borrowPtr());
}


PExampleTable TFilter::operator()(PExampleTable const &data)
{
    PExampleTable res = TExampleTable::constructEmptyReference(data);
    TExampleTable &ref = *res;
    for(TExampleIterator ei(data); ei; ++ei) {
        if ((*this)(*ei)) {
            ref.fast_push_back(*ei);
        }
    }
    return res;
}


PFilter TFilter::deepCopy() const
{
  raiseError(PyExc_NotImplementedError, "deep copy not implemented");
  return PFilter();
}


PyObject *TFilter::__call__(PyObject *args, PyObject *kw)
{
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O:Filter",
        Filter_call_keywords, &obj)) {
        return NULL;
    }
    if (OrExample_Check(obj)) {
        TExample *example;
        if (domain) {
            PExample pexample = TExample::fromDomainAndPyObject(domain, obj, false);
            example = pexample.borrowPtr();
        }
        else {
            example = (TExample *)&((OrOrange *)obj)->orange;
        }
        return PyBool_FromBool((*this)(example));
    }
    else if (OrExampleTable_Check(obj)) {
        return (*this)(PExampleTable(obj)).getPyObject();
    }
    return PyErr_Format(PyExc_TypeError,
        "Filter expects Example or ExampleTable, not '%s'", obj->ob_type->tp_name);
}


PyObject *TFilter::__and__(PyObject *self, PyObject *other)
{
    try {
        if (!OrFilter_Check(other)) {
            return PyErr_Format(PyExc_TypeError,
                "unsupported operand types for 'and': '%s' and '%s'",
                self->ob_type->tp_name, other->ob_type->tp_name);
        }
        PFilter_conjunction filter(new TFilter_conjunction());
        filter->filters->push_back(PFilter(self));
        filter->filters->push_back(PFilter(other));
        return filter.getPyObject();
    }
    PyCATCH;
}

PyObject *TFilter::__or__(PyObject *self, PyObject *other)
{
    try {
        if (!OrFilter_Check(other)) {
            return PyErr_Format(PyExc_TypeError,
                "unsupported operand types for 'and': '%s' and '%s'",
                self->ob_type->tp_name, other->ob_type->tp_name);
        }
        PFilter_disjunction filter(new TFilter_disjunction());
        filter->filters->push_back(PFilter(self));
        filter->filters->push_back(PFilter(other));
        return filter.getPyObject();
    }
    PyCATCH;
}



TFilter_random::TFilter_random(
    double const ap, bool const aneg, PRandomGenerator const &rgen)
: TFilter(aneg),
  prob(ap),
  randomGenerator(rgen ? rgen : PRandomGenerator(new TRandomGenerator()))
{}


// Chooses an example (returns true) if rand()<maxrand; example is ignored
bool TFilter_random::operator()(TExample const *const)
{
    if (!randomGenerator) {
        randomGenerator = PRandomGenerator(new TRandomGenerator());
    }
    return (randomGenerator->randdouble() < prob) != negate;
}


TFilter_hasSpecial::TFilter_hasSpecial(bool const aneg)
: TFilter(aneg)
{}


TFilter_hasSpecial::TFilter_hasSpecial(PDomain const &dom, bool aneg)
: TFilter(dom, aneg)
{}


inline bool example_hasMissing(TExample const *const example)
{
    const_PITERATE(TExample, ei, example) {
        if (isnan(*ei)) {
            return true;
        }
    }
    return false;
}

bool TFilter_hasSpecial::operator()(TExample const *const exam)
{ 
    if (domain) {
        PExample converted = exam->convertedTo(domain, false);
        return example_hasMissing(converted.getPtr()) != negate;
    }
    else {
        return example_hasMissing(exam) != negate;
    }
}


TFilter_isDefined::TFilter_isDefined(bool const aneg)
: TFilter(aneg)
{}


TFilter_isDefined::TFilter_isDefined(PDomain const &dom, bool aneg)
: TFilter(dom, aneg),
check(new TAttributedBoolList(dom->variables, true))
{}


bool TFilter_isDefined::isDefined_noConversion(TExample const *const example) const
{
    if (!check || !check->size()) {
        return example_hasMissing(example) == negate;
    }
    TAttributedBoolList::const_iterator ci(check->begin()), ce(check->end());
    TExample::const_iterator ei(example->begin()), ee(example->end());
    for(; (ci!=ce) && (ei!=ee); ci++, ei++) {
        if (*ci && isnan(*ei)) {
            return negate;
        }
    }
    return !negate;
}

bool TFilter_isDefined::operator()(TExample const *const exam)
{
    if (domain) {
        PExample converted = exam->convertedTo(domain, false);
        return isDefined_noConversion(converted.getPtr());
    }
    else {
        return isDefined_noConversion(exam);
    }
}

int TFilter_isDefined::__set__domain(PyObject *self, PyObject *val)
{
    PFilter_isDefined me(self);
    if (val == Py_None) {
        me->domain = PDomain();
        me->check = PAttributedBoolList();
        return 0;
    }
    if (!OrDomain_Check(val)) {
        PyErr_Format(PyExc_TypeError,
            "domain must by an instance of Domain, not '%s'",
            val->ob_type->tp_name);
        return -1;
    }
    me->domain = PDomain(val);
    me->check = PAttributedBoolList(
        new TAttributedBoolList(me->domain->variables, true));
    return 0;
}


TFilter_hasMeta::TFilter_hasMeta(TMetaId const anid, bool aneg)
: TFilter(aneg),
  id(anid)
{}


bool TFilter_hasMeta::operator()(TExample const *const exam)
{
    return (id && exam->hasMeta(id)) != negate;
}


TFilter_hasClassValue::TFilter_hasClassValue(bool aneg)
: TFilter(aneg)
{}


TFilter_hasClassValue::TFilter_hasClassValue(PDomain const &dom, bool aneg)
: TFilter(dom, aneg)
{}


bool TFilter_hasClassValue::operator()(TExample const *const exam)
{ 
    if (domain) {
        PExample converted = exam->convertedTo(domain, false);
        return (isnan(converted->getClass())!=0) == negate;
    }
    else {
        return (isnan(exam->getClass())!=0) == negate;
    }
}


TFilter_sameValue::TFilter_sameValue(TValue const aval, TAttrIdx const apos,
                                     bool const aneg, PDomain const &dom)
: TFilter(dom, aneg), 
  position(apos),
  value(aval)
{}


bool TFilter_sameValue::operator()(TExample const *const example)
{ 
    if (domain && (domain != example->domain)) {
        if (position >= domain->variables->size()) {
            raiseError(PyExc_IndexError, "index %i out of range", position);
        }
        // this is slow and inefficient, but it's the only legal way of doing it
        PExample converted = example->convertedTo(domain, false);
        TValue const val = position != -1 ? 
            (*converted)[position] : converted->getClass();
        return (fabs(val - value) < 1e-6) != negate;
    }
    else {
        if (position >= example->domain->variables->size()) {
            raiseError(PyExc_IndexError, "index %i out of range", position);
        }
        TValue const val = position != -1 ?
            (*example)[position] : example->getClass();
        return (fabs(val - value) < 1e-6) != negate;
    }
}


// This constructor cannot set variable: virtual method setVariable checks
// the variable type, but cannot be called from the parent class constructor
// (virtual methods don't work there yet, remember?)
TValueFilter::TValueFilter(int const accs)
: variable(PVariable()),
  lastPosition(ILLEGAL_INT),
  lastDomainVersion(-1),
  acceptMissing(accs)
{}


PValueFilter TValueFilter::deepCopy() const
{
    raiseError(PyExc_SystemError, "Deep copy not implemented.");
    return PValueFilter();
}


TValueFilter::Operator TValueFilter::operatorFromPy(int op)
{
    switch (op) {
        case Py_LT: return Less;
        case Py_GT: return Greater;
        case Py_GE: return GreaterEqual;
        case Py_LE: return LessEqual;
        case Py_EQ: return Equal;
        case Py_NE: return NotEqual;
        default:
            PyErr_Format(PyExc_TypeError, "invalid comparison operator");
            return None;
    }
}


PyObject *TValueFilter::__call__(PyObject *args, PyObject *kw)
{
    PExample example;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&:Filter",
        ValueFilter_call_keywords, &PExample::argconverter, &example)) {
        return NULL;
    }
    return PyBool_FromBool((*this)(example.borrowPtr())!=0);
}


PyObject *TValueFilter::__get__variable(OrValueFilter *self)
{
    TValueFilter &me = dynamic_cast<TValueFilter &>(self->orange);
    return me.getVariable().getPyObject();
}


int TValueFilter::__set__variable(OrValueFilter *self, PyObject *value)
{
    TValueFilter &me = dynamic_cast<TValueFilter &>(self->orange);
    if (!OrVariable_Check(value)) {
        PyErr_Format(PyExc_TypeError,
            "ValueFilter.variable must be a Variable, not '%s'",
            value->ob_type->tp_name);
        return -1;
    }
    try {
        me.setVariable(PVariable(value));
    }
    PyCATCH_1;
    return 0;
}


PyObject *TValueFilter::__getnewargs__() const
{
    return PyTuple_Pack(1, variable.borrowPyObject());
}


TValueFilter_continuous::TValueFilter_continuous(
    PVariable const &var,
    TValue const amin, TValue const amax,
    bool const outs, int const accs)
: TValueFilter(accs),
  min(amin),
  max(amax),
  outside(outs),
  oper(None)
{
    setVariable(var);
}


TValueFilter_continuous::TValueFilter_continuous(
    PVariable const &var, int const op, TValue const amin, TValue const amax,
    int const accs)
: TValueFilter(accs),
  min(amin),
  max(amax),
  oper(op)
{
    setVariable(var);
}


#define EQUAL(x,y)  (fabs(x-y) <= y*1e-10) ? 1 : 0
#define LESS_EQUAL(x,y) (x-y <= y*1e-10) ? 1 : 0
#define TO_BOOL(x) (x) ? 1 : 0;

int TValueFilter_continuous::operator()(TExample const *const example) const
{ 
    TValue const val = (*example)[getVarPos(example->domain)];
    if (isnan(val)) {
        return acceptMissing;
    }
    switch (oper) {
        case None:         return TO_BOOL(((val>=min) && (val<=max)) != outside);
        case Equal:        return EQUAL(val, min);
        case NotEqual:     return 1 - EQUAL(val, min);
        case Less:         return TO_BOOL(val < min);
        case LessEqual:    return LESS_EQUAL(val, min);
        case Greater:      return TO_BOOL(min < val);
        case GreaterEqual: return LESS_EQUAL(min, val);
        case Between:      return (LESS_EQUAL(min, val)) * (LESS_EQUAL(val, max));
        case Outside:      return TO_BOOL((val < min) || (val > max));
        default:  return -1;
    }
}


PValueFilter TValueFilter_continuous::deepCopy() const
{
    TValueFilter_continuous *filter =
        new TValueFilter_continuous(getVariable(), oper, min, max, acceptMissing);
    filter->outside = outside;
    return PValueFilter(filter);
}


TOrange *TValueFilter_continuous::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    int oper = -1;
    TValue min = ILLEGAL_FLOAT, max = ILLEGAL_FLOAT;
    // watch out: cannot use different keywords (such as ref) with
    // call-constructable classes since GenericNewWithCall will filter them out!
    if (args && PyTuple_Size(args) == 3) {
        if (!PyArg_ParseTuple(args, "O&id",
            &PVariable::argconverter, &var, &oper, &min)) {
                return NULL;
        }
    }
    else {
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|idd",
            ValueFilter_continuous_keywords,
            &PVariable::argconverter, &var, &oper, &min, &max)) {
                return NULL;
        }
        if (oper == -1) {
            if ((min != ILLEGAL_FLOAT) && (max != ILLEGAL_FLOAT)) {
                oper = Between;
            }
            else if (min != ILLEGAL_FLOAT) {
                oper = GreaterEqual;
            }
            else if (max != ILLEGAL_FLOAT) {
                oper = LessEqual;
            }
            // else it doesn't matter since we'll raise exception below
        }
    }
    if ((oper == Between) || (oper == Outside)) {
        if (min == ILLEGAL_FLOAT) {
            PyErr_Format(PyExc_ValueError, "lower bound must be given");
            return NULL;
        }
        if (max == ILLEGAL_FLOAT) {
            PyErr_Format(PyExc_ValueError, "upper bound must be given");
            return NULL;
        }
    }
    else {
        if (min == ILLEGAL_FLOAT) {
            min = max;
        }
        if (min == ILLEGAL_FLOAT) {
            PyErr_Format(PyExc_ValueError, "reference value must be given");
            return NULL;
        }
    }
    return new TValueFilter_continuous(var, oper, min, max);
}


TValueFilter_discrete::TValueFilter_discrete(
    PVariable const &var, PValueList const &bl, int const accs, bool const neg)
: TValueFilter(accs),
  values(bl ? bl : PValueList(new TValueList())),
  negate(neg)
{
    setVariable(var);
}



int TValueFilter_discrete::operator()(TExample const *const example) const
{ 
    TValue const val = (*example)[getVarPos(example->domain)];
    if (isnan(val)) {
        return negate && (acceptMissing != -1) ? 1-acceptMissing : acceptMissing;
    }
    const_PITERATE(TValueList, vi, values) {
        if (*vi == val) {
            return negate ? 0 : 1;
        }
    }
    return negate ? 1 : 0;
}


PValueFilter TValueFilter_discrete::deepCopy() const
{
    return PValueFilter(new TValueFilter_discrete(
        getVariable(), PValueList(new TValueList(*values)), acceptMissing, negate));
}


TOrange *TValueFilter_discrete::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    PValueList vallist;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|O&",
        ValueFilter_discrete_keywords,
        &PVariable::argconverter, &var, &PValueList::argconverter_n, &vallist)) {
            return NULL;
    }
    return new TValueFilter_discrete(var, vallist);
}


TValueFilter_string::TValueFilter_string(
    PVariable const &var, int const op,
    string const &amin, string const &amax,
    int const accs, bool const csens)
: TValueFilter(accs),
  min(amin),
  max(amax),
  oper(op),
  caseSensitive(csens)
{
    setVariable(var);
}


string strToLower(string nm)
{ 
    string res(nm.size(), 32);
    transform(nm.begin(), nm.end(), res.begin(), tolower);
    return res;
}

#ifndef _MSC_VER
int strcmpi(const char *s1, const char *s2) // case insensitive comparison
{
    char c1, c2;
    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    }
    while (c1 && c2 && (c1==c2));
    if (c1 < c2) {
        return -1;
    }
    else if (c1 > c2) {
        return 1;
    }
    else {
        return 0;
    }
}
#endif

#define rcmp (*(caseSensitive ? &strcmp : &strcmpi))

int strncmpi(const char *s1, const char *s2, size_t n) // case insensitive comparison
{
    if (!n) {
        return 0;
    }
    char c1, c2;
    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    }
    while (--n && c1 && c2 && (c1==c2));
    if (c1 < c2) {
        return -1;
    }
    else if (c1 > c2) {
        return 1;
    }
    else {
        return 0;
    }
}

int findi(const char *haystack, const char *needle)
{
    while(*haystack) {
        const char *ha = haystack++, *ne = needle;
        for(; *ha && *ne && tolower(*ha) == tolower(*ne); ha++, ne++);
        if (!*ne) {
            return 1;
        }
    }
    return 0;
}


int TValueFilter_string::operator()(TExample const *const example) const
{ 
    TMetaValue val = example->getMeta(getVarPos(example->domain));
    if (val.isPrimitive || val.object && !PyUnicode_Check(val.object)) {
        raiseError(PyExc_TypeError, "string value expected");
    }
    if (!val.object) {
        return acceptMissing;
    }
	PyObject *bytes = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(val.object),
                                           PyUnicode_GET_SIZE(val.object), "ignore");
    GUARD(bytes);
    char const *value = PyBytes_AS_STRING(bytes);
    char const *ref = min.c_str();
    switch(oper) {
        case Equal:        
            return TO_BOOL(!rcmp(value, ref));

        case NotEqual:     
            return TO_BOOL(rcmp(value, ref));

        case Less:         
            return TO_BOOL(rcmp(value, ref) < 0);

        case LessEqual:    
            return TO_BOOL(rcmp(value, ref) <= 0);

        case Greater:      
            return TO_BOOL(rcmp(value, ref) > 0);

        case GreaterEqual: 
            return TO_BOOL(rcmp(value, ref) >= 0);

        case Between:      
            return TO_BOOL((rcmp(value, ref) >= 0) &&
                           (rcmp(value, max.c_str()) <= 0));

        case Outside:      
            return TO_BOOL((rcmp(value, ref) < 0) ||
                           (rcmp(value, max.c_str()) > 0));

        case Contains:     
            return caseSensitive ? (strstr(value, ref) ? 1 : 0)
                                 : findi(value, ref);

        case NotContains:
            return caseSensitive ? (strstr(value, ref) ? 0 : 1)
                                 : 1 - findi(value, ref);

        case BeginsWith:   
            return TO_BOOL(!(*(caseSensitive ? &strncmp : strncmpi))(
                               value, ref, min.size()));

        case EndsWith: {
            const int vsize = strlen(value), rsize = min.size();
            if (rsize > vsize) {
                return 0;
            }
            return TO_BOOL(rcmp(value+(vsize-rsize), ref)==0);
        }

        default:
          return -1;
    }
}


TOrange *TValueFilter_string::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    int oper = -1;
    char *min = NULL, *max = NULL;
    // watch out: cannot use different keywords (such as ref) with
    // call-constructable classes since GenericNewWithCall will filter them out!
    if (args && PyTuple_Size(args) == 3) {
            if (!PyArg_ParseTuple(args, "O&is",
                &PVariable::argconverter, &var, &oper, &min)) {
                    return NULL;
            }
    }
    else {
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|iss",
            ValueFilter_string_keywords,
            &PVariable::argconverter, &var, &oper, &min, &max)) {
                return NULL;
        }
        if (oper == -1) {
            if (min && max) {
                oper = Between;
            }
            else if (min) {
                oper = GreaterEqual;
            }
            else if (max) {
                oper = LessEqual;
            }
            // else it doesn't matter since we'll raise exception below
        }
    }
    if ((oper == Between) || (oper == Outside)) {
        if (!min) {
            PyErr_Format(PyExc_ValueError, "lower bound must be given");
            return NULL;
        }
        if (!max) {
            PyErr_Format(PyExc_ValueError, "upper bound must be given");
            return NULL;
        }
    }
    else {
        if (!min) {
            min = max;
        }
        if (!min) {
            PyErr_Format(PyExc_ValueError, "reference value must be given");
            return NULL;
        }
    }
    return new TValueFilter_string(var, oper, min, max ? string(max) : string());
}


TValueFilter_stringList::TValueFilter_stringList(
    PVariable const &var, PStringList const &bl,
    int const accs, bool const neg, bool const csens)
: TValueFilter(accs),
  values(bl),
  negate(neg),
  caseSensitive(csens)
{
    setVariable(var);
}


int TValueFilter_stringList::operator()(TExample const *const example) const
{ 
    TMetaValue val = example->getMeta(getVarPos(example->domain));
    if (val.isPrimitive || val.object && !PyUnicode_Check(val.object)) {
        raiseError(PyExc_TypeError, "string value expected");
    }
    if (!val.object) {
        if (acceptMissing == -1) {
            return -1;
        }
        return negate ? 1-acceptMissing : acceptMissing;
    }
	PyObject *bytes = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(val.object),
                                           PyUnicode_GET_SIZE(val.object), "ignore");
    GUARD(bytes);
    char const *value = PyBytes_AS_STRING(bytes);
    PITERATE(TStringList, vi, values) {
        if (!rcmp(value, vi->c_str())) {
            return negate ? 0 : 1;
        }
    }
    return negate ? 1 : 0;
}


TOrange *TValueFilter_stringList::__new__(
    PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PVariable var;
    PStringList vallist;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O&|O&",
        ValueFilter_stringList_keywords,
        &PVariable::argconverter, &var, &PStringList::argconverter_n, &vallist)) {
            return NULL;
    }
    return new TValueFilter_stringList(var, vallist);
}


TFilter_values::TFilter_values(
    bool const conjunc, bool const aneg, PDomain const &dom)
: TFilter(dom, aneg),
  conditions(PValueFilterList(new TValueFilterList())),
  conjunction(conjunc)
{}


TFilter_values::TFilter_values(
    PValueFilterList const &v, bool const conjunc,
    bool const aneg, PDomain const &dom)
: TFilter(dom, aneg),
  conditions(v),
  conjunction(conjunc)
{}


TValueFilterList::iterator TFilter_values::findCondition(PVariable const &var)
{
    TValueFilterList::iterator condi(conditions->begin()), conde(conditions->end());
    while((condi!=conde) && ((*condi)->getVariable() != var)) {
        condi++;
    }
    return condi;
}


void TFilter_values::updateCondition(PValueFilter const &filter)
{
    TValueFilterList::iterator condi = findCondition(filter->getVariable());
    if (condi == conditions->end()) {
        conditions->push_back(filter);
    }
    else {
        *condi = filter;
    }
}


void TFilter_values::addCondition(
    PVariable const &var, TValue const val, bool const negate)
{
    TValueFilterList::iterator condi =  findCondition(var);
    TValueFilter_discrete *valueFilter;
    if (condi == conditions->end()) {
        valueFilter = new TValueFilter_discrete(var);
        conditions->push_back(PValueFilter(valueFilter));
    }
    else {
        valueFilter = dynamic_cast<TValueFilter_discrete *>(condi->borrowPtr());
        if (!valueFilter) {
            raiseError(PyExc_TypeError,
                "addCondition(value) can be used only for discrete filters");
        }
    }
    if (isnan(val)) {
        valueFilter->acceptMissing = 1;
    }
    else {
        valueFilter->values->clear();
        valueFilter->values->push_back(val);
    }
    valueFilter->negate = negate;
}


void TFilter_values::addCondition(
    PVariable const &var, PValueList const &vallist, bool const negate)
{
    TValueFilterList::iterator condi = findCondition(var);
    if (condi==conditions->end()) {
        PValueFilter_discrete filter(new TValueFilter_discrete(var, vallist));
        filter->negate = negate;
        conditions->push_back(filter);
    }
    else {
        TValueFilter_discrete *valueFilter =
            dynamic_cast<TValueFilter_discrete *>(condi->borrowPtr());
        if (!valueFilter) {
            raiseError(PyExc_TypeError,
                "addCondition(value) can be used only for discrete filters");
        }
        else {
            valueFilter->values = vallist;
        }
        valueFilter->negate = negate;
    }
}


void TFilter_values::addCondition(
    PVariable const &var, int const oper, TValue const min, TValue const max)
{
    updateCondition(PValueFilter(new TValueFilter_continuous(var, oper, min, max)));
}


void TFilter_values::addCondition(
    PVariable const &var, int const oper, string const &min, string const &max)
{
    updateCondition(PValueFilter(new TValueFilter_string(var, oper, min, max)));
}


void TFilter_values::addCondition(
    PVariable const &var, PStringList const &slist, bool const negate)
{
    updateCondition(PValueFilter(new TValueFilter_stringList(var, slist, 0, negate)));
}


void TFilter_values::removeCondition(PVariable const &var)
{
    TValueFilterList::iterator condi = findCondition(var);
    if (condi==conditions->end()) {
        raiseError(PyExc_KeyError,
            "there is no condition on value of '%s'", var->cname());
    }
    conditions->erase(condi);
}
  

bool TFilter_values::operator()(TExample const *const exam)
{ 
    checkProperty(conditions);
    if (domain) {
        PExample converted = exam->convertedTo(domain);
        TExample *econv = converted.borrowPtr();
        PITERATE(TValueFilterList, fi, conditions) {
            const int r = (**fi)(econv);
            if ((r==0) && conjunction) {
                return negate;
            }
            if ((r==1) && !conjunction) {
                return !negate;
            }
        }
    }
    else {
        PITERATE(TValueFilterList, fi, conditions) {
            const int r = (**fi)(exam);
            if ((r==0) && conjunction) {
                return negate;
            }
            if ((r==1) && !conjunction) {
                return !negate;
            }
        }
    }
    return conjunction!=negate;
}


PFilter TFilter_values::deepCopy() const
{
    PValueFilterList newValueFilters(new TValueFilterList());
    const_PITERATE(TValueFilterList, vi, conditions) {
        newValueFilters->push_back((*vi)->deepCopy());
    }
    return PFilter(new TFilter_values(
        newValueFilters,conjunction,negate,domain));
}



TFilter_sameExample::TFilter_sameExample(bool const aneg)
: TFilter(aneg)
{}


TFilter_sameExample::TFilter_sameExample(
    PExample const &anexample, bool const aneg)
: TFilter(anexample->domain, aneg),
example(anexample)
{}


bool TFilter_sameExample::operator()(TExample const *const other)
{
    return example && ((example->cmp(*other->convertedTo(example->domain)) == 0) != negate);
}



TFilter_compatibleExample::TFilter_compatibleExample(bool const aneg)
: TFilter(aneg)
{}


TFilter_compatibleExample::TFilter_compatibleExample(
    PExample const &anexample, bool const aneg)
: TFilter(anexample->domain, aneg),
  example(anexample)
{}


bool TFilter_compatibleExample::operator()(TExample const *const other)
{ 
    if (!example) {
        return negate;
    }
    PExample converted = other->convertedTo(example->domain);
    TExample::const_iterator e1i(example->begin()), e1e(example->end());
    TExample::const_iterator e2i(converted->begin()); 
    while(e1i != e1e) {
        if (!values_compatible(*e1i++, *e2i++)) {
            return negate;
        }
    }
    return !negate;
}


TFilter_conjunction::TFilter_conjunction()
: filters(new TFilterList())
{}


TFilter_conjunction::TFilter_conjunction(PFilterList const &af)
: filters(af)
{}


bool TFilter_conjunction::operator()(TExample const *const ex)
{
    if (filters) {
        PITERATE(TFilterList, fi, filters) {
            if (!(**fi)(ex)) {
                return negate;
            }
        }
    }
    return !negate;
}


TFilter_disjunction::TFilter_disjunction()
: filters(new TFilterList())
{}


TFilter_disjunction::TFilter_disjunction(PFilterList const &af)
: filters(af)
{}


bool TFilter_disjunction::operator()(TExample const *const ex)
{
    if (filters) {
        PITERATE(TFilterList, fi, filters) {
            if ((**fi)(ex)) {
                return !negate;
            }
        }
    }
    return negate;
}
