/*

 */

#include "valueflow.h"
#include "astutils.h"
#include "errorlogger.h"
#include "mathlib.h"
#include "settings.h"
#include "symboldatabase.h"
#include "token.h"
#include "tokenlist.h"
#include <stack>

namespace {
    struct ProgramMemory {
        std::map<unsigned int, MathLib::bigint> intvalues;
        std::map<unsigned int, const Token *> tokvalues;

        void setValue(unsigned int varid, const ValueFlow::Value &value) {
            if (value.tokvalue) {
                intvalues.erase(varid);
                tokvalues[varid] = value.tokvalue;
            } else {
                intvalues[varid] = value.intvalue;
                tokvalues.erase(varid);
            }
        }

        bool getIntValue(unsigned int varid, MathLib::bigint* result) const {
            const std::map<unsigned int, MathLib::bigint>::const_iterator it = intvalues.find(varid);
            if (it != intvalues.end())
                *result = it->second;
            return (it != intvalues.end());
        }

        void setIntValue(unsigned int varid, MathLib::bigint value) {
            intvalues[varid] = value;
            tokvalues.erase(varid);
        }

        bool hasValue(unsigned int varid) {
            return intvalues.find(varid) != intvalues.end() ||
                   tokvalues.find(varid) != tokvalues.end();
        }

        void swap(ProgramMemory &pm) {
            intvalues.swap(pm.intvalues);
            tokvalues.swap(pm.tokvalues);
        }

        void clear() {
            intvalues.clear();
            tokvalues.clear();
        }

        bool empty() const {
            return intvalues.empty() && tokvalues.empty();
        }
    };
}

static void execute(const Token *expr,
                    ProgramMemory * const programMemory,
                    MathLib::bigint *result,
                    bool *error);

static void bailout(TokenList *tokenlist, ErrorLogger *errorLogger, const Token *tok, const std::string &what)
{
    std::list<ErrorLogger::ErrorMessage::FileLocation> callstack;
    callstack.push_back(ErrorLogger::ErrorMessage::FileLocation(tok,tokenlist));
    ErrorLogger::ErrorMessage errmsg(callstack, Severity::debug, "ValueFlow bailout: " + what, "valueFlowBailout", false);
    errorLogger->reportErr(errmsg);
}

static bool bailoutFunctionPar(const Token *tok, const ValueFlow::Value &value, const Settings *settings, bool *inconclusive)
{
    if (!tok)
        return false;

    // address of variable
    const bool addressOf = tok && Token::simpleMatch(tok->previous(), "&");

    // passing variable to subfunction?
    if (Token::Match(tok->tokAt(-2), ") & %name% [,)]") && Token::Match(tok->linkAt(-2)->previous(), "[,(] ("))
        ;
    else if (Token::Match(tok->tokAt(addressOf?-2:-1), "[(,] &| %name% [,)]"))
        ;
    else
        return false;

    // reinterpret_cast etc..
    if (Token::Match(tok->tokAt(-3), "> ( & %name% ) [,)]") &&
        tok->linkAt(-3) &&
        Token::Match(tok->linkAt(-3)->tokAt(-2), "[,(] %type% <"))
        tok = tok->linkAt(-3);

    // goto start of function call and get argnr
    unsigned int argnr = 0;
    while (tok && tok->str() != "(") {
        if (tok->str() == ",")
            ++argnr;
        else if (tok->str() == ")")
            tok = tok->link();
        tok = tok->previous();
    }
    tok = tok ? tok->previous() : nullptr;
    if (!Token::Match(tok,"%name% ("))
        return false; // not a function => do not bailout

    if (!tok->function()) {
        // if value is 0 and the library says 0 is invalid => do not bailout
        if (value.intvalue==0 && settings->library.isnullargbad(tok, 1+argnr))
            return false;
        // addressOf => inconclusive
        if (!addressOf) {
            *inconclusive = true;
            return false;
        }
        return true;
    }

    const Variable *arg = tok->function()->getArgumentVar(argnr);

    if (addressOf && !(arg && arg->isConst()))
        return true;

    return arg && !arg->isConst() && arg->isReference();
}

/**
 * Is condition always false when variable has given value?
 * \param condition   top ast token in condition
 * \param programMemory   program memory
 */
static bool conditionIsFalse(const Token *condition, const ProgramMemory &programMemory)
{
    if (!condition)
        return false;
    if (condition->str() == "&&") {
        const bool result1 = conditionIsFalse(condition->astOperand1(), programMemory);
        const bool result2 = result1 ? true : conditionIsFalse(condition->astOperand2(), programMemory);
        return result2;
    }
    ProgramMemory progmem(programMemory);
    MathLib::bigint result = 0;
    bool error = false;
    execute(condition, &progmem, &result, &error);
    return !error && result == 0;
}

/**
 * Is condition always true when variable has given value?
 * \param condition   top ast token in condition
 * \param programMemory   program memory
 */
static bool conditionIsTrue(const Token *condition, const ProgramMemory &programMemory)
{
    if (!condition)
        return false;
    if (condition->str() == "||") {
        const bool result1 = conditionIsTrue(condition->astOperand1(), programMemory);
        const bool result2 = result1 ? true : conditionIsTrue(condition->astOperand2(), programMemory);
        return result2;
    }
    ProgramMemory progmem(programMemory);
    bool error = false;
    MathLib::bigint result = 0;
    execute(condition, &progmem, &result, &error);
    return !error && result == 1;
}

/**
 * Get program memory by looking backwards from given token.
 */
static ProgramMemory getProgramMemory(const Token *tok, unsigned int varid, const ValueFlow::Value &value)
{
    ProgramMemory programMemory;
    programMemory.setValue(varid, value);
    if (value.varId)
        programMemory.setIntValue(value.varId, value.varvalue);
    const ProgramMemory programMemory1(programMemory);
    int indentlevel = 0;
    for (const Token *tok2 = tok; tok2; tok2 = tok2->previous()) {
        if (Token::Match(tok2, "[;{}] %varid% = %var% ;", varid)) {
            const Token *vartok = tok2->tokAt(3);
            programMemory.setValue(vartok->varId(), value);
        }
        if (Token::Match(tok2, "[;{}] %var% =")) {
            const Token *vartok = tok2->next();
            if (!programMemory.hasValue(vartok->varId())) {
                MathLib::bigint result = 0;
                bool error = false;
                execute(tok2->tokAt(2)->astOperand2(), &programMemory, &result, &error);
                if (!error)
                    programMemory.setIntValue(vartok->varId(), result);
            }
        }
        if (tok2->str() == "{") {
            if (indentlevel <= 0)
                break;
            --indentlevel;
        }
        if (tok2->str() == "}") {
            const Token *cond = tok2->link();
            cond = Token::simpleMatch(cond->previous(), ") {") ? cond->linkAt(-1) : nullptr;
            if (cond && conditionIsFalse(cond->astOperand2(), programMemory1))
                tok2 = cond->previous();
            else if (cond && conditionIsTrue(cond->astOperand2(), programMemory1)) {
                ++indentlevel;
                continue;
            } else
                break;
        }
    }
    return programMemory;
}

/**
 * Should value be skipped because it's hidden inside && || or ?: expression.
 * Example: ((x!=NULL) && (*x == 123))
 * If 'valuetok' points at the x in '(*x == 123)'. Then the '&&' will be returned.
 * @param valuetok original variable token
 * @return NULL=>don't skip, non-NULL=>The operator token that cause the skip. For instance the '&&'.
 * */
static const Token * skipValueInConditionalExpression(const Token * const valuetok)
{
    // Walk up the ast
    const Token *prev = valuetok;
    for (const Token *tok = valuetok->astParent(); tok; tok = tok->astParent()) {
        const bool prevIsLhs = (prev == tok->astOperand1());
        prev = tok;

        if (prevIsLhs || !Token::Match(tok, "%oror%|&&|?|:"))
            continue;

        // Is variable protected in LHS..
        std::stack<const Token *> tokens;
        tokens.push(tok->astOperand1());
        while (!tokens.empty()) {
            const Token * const tok2 = tokens.top();
            tokens.pop();
            if (!tok2 || tok2->str() == ".")
                continue;
            // A variable is seen..
            if (tok2 != valuetok && tok2->variable() && (tok2->varId() == valuetok->varId() || !tok2->variable()->isArgument())) {
                // TODO: limit this bailout
                return tok;
            }
            tokens.push(tok2->astOperand2());
            tokens.push(tok2->astOperand1());
        }

    }
    return nullptr;
}

static bool bailoutSelfAssignment(const Token * const tok)
{
    const Token *parent = tok;
    while (parent) {
        const Token *op = parent;
        parent = parent->astParent();

        // Assignment where lhs variable exists in rhs => return true
        if (parent                         != nullptr      &&
            parent->astOperand2()          == op           &&
            parent->astOperand1()          != nullptr      &&
            parent->str()                  == "=") {
            for (const Token *lhs = parent->astOperand1(); lhs; lhs = lhs->astOperand1()) {
                if (lhs->varId() == tok->varId())
                    return true;
                if (lhs->astOperand2() && lhs->astOperand2()->varId() == tok->varId())
                    return true;
            }
        }
    }
    return false;
}

/** Add token value. Return true if value is added. */
static bool addValue(Token *tok, const ValueFlow::Value &value)
{
    if (value.isKnown()) {
        // Clear all other values since value is known
        tok->values.clear();
    }

    // Don't handle more than 10 values for performance reasons
    // TODO: add setting?
    if (tok->values.size() >= 10U)
        return false;

    // if value already exists, don't add it again
    std::list<ValueFlow::Value>::iterator it;
    for (it = tok->values.begin(); it != tok->values.end(); ++it) {
        // different intvalue => continue
        if (it->intvalue != value.intvalue)
            continue;

        // different tokvalue => continue
        if ((it->tokvalue == nullptr) != (value.tokvalue == nullptr))
            continue;
        if ((value.tokvalue != nullptr) && (it->tokvalue != value.tokvalue) && (it->tokvalue->str() != value.tokvalue->str()))
            continue;

        // same value, but old value is inconclusive so replace it
        if (it->inconclusive && !value.inconclusive) {
            *it = value;
            if (it->varId == 0)
                it->varId = tok->varId();
            break;
        }

        // Same value already exists, don't  add new value
        return false;
    }

    // Add value
    if (it == tok->values.end()) {
        ValueFlow::Value v(value);
        if (v.varId == 0)
            v.varId = tok->varId();
        tok->values.push_back(v);
    }

    return true;
}

/** set ValueFlow value and perform calculations if possible */
static void setTokenValue(Token* tok, const ValueFlow::Value &value)
{
    if (!addValue(tok,value))
        return;

    Token *parent = const_cast<Token*>(tok->astParent());
    if (!parent)
        return;

    // Cast..
    if (parent->str() == "(" && tok == parent->link()->next()) {
        setTokenValue(parent,value);
    }

    else if (parent->str() == ":") {
        setTokenValue(parent,value);
    }

    else if (parent->str() == "?" && tok->str() == ":" && tok == parent->astOperand2()) {
        // is condition only depending on 1 variable?
        std::stack<const Token*> tokens;
        tokens.push(parent->astOperand1());
        unsigned int varId = 0;
        while (!tokens.empty()) {
            const Token *t = tokens.top();
            tokens.pop();
            if (!t)
                continue;
            tokens.push(t->astOperand1());
            tokens.push(t->astOperand2());
            if (t->varId()) {
                if (varId > 0 || value.varId != 0U)
                    return;
                varId = t->varId();
            } else if (t->str() == "(" && Token::Match(t->previous(), "%name%"))
                return; // function call
        }

        ValueFlow::Value v(value);
        v.conditional = true;
        v.changeKnownToPossible();

        if (varId)
            v.varId = varId;

        setTokenValue(parent, v);
    }

    // Calculations..
    else if ((parent->isArithmeticalOp() || parent->isComparisonOp() || (parent->tokType() == Token::eBitOp)) &&
             parent->astOperand1() &&
             parent->astOperand2()) {
        const bool known = ((parent->astOperand1()->values.size() == 1U &&
                             parent->astOperand1()->values.front().isKnown()) ||
                            (parent->astOperand2()->values.size() == 1U &&
                             parent->astOperand2()->values.front().isKnown()));

        // known result when a operand is 0.
        if (Token::Match(parent, "[&*]") && value.isKnown() && value.tokvalue==nullptr && value.intvalue==0) {
            setTokenValue(parent, value);
            return;
        }

        std::list<ValueFlow::Value>::const_iterator value1, value2;
        for (value1 = parent->astOperand1()->values.begin(); value1 != parent->astOperand1()->values.end(); ++value1) {
            if (value1->tokvalue && (!parent->isComparisonOp() || value1->tokvalue->tokType() != Token::eString))
                continue;
            for (value2 = parent->astOperand2()->values.begin(); value2 != parent->astOperand2()->values.end(); ++value2) {
                if (value2->tokvalue && (!parent->isComparisonOp() || value2->tokvalue->tokType() != Token::eString || value1->tokvalue))
                    continue;
                if (known || value1->varId == 0U || value2->varId == 0U ||
                    (value1->varId == value2->varId && value1->varvalue == value2->varvalue && !value1->tokvalue && !value2->tokvalue)) {
                    ValueFlow::Value result(0);
                    result.condition = value1->condition ? value1->condition : value2->condition;
                    result.inconclusive = value1->inconclusive | value2->inconclusive;
                    result.varId = (value1->varId != 0U) ? value1->varId : value2->varId;
                    result.varvalue = (result.varId == value1->varId) ? value1->intvalue : value2->intvalue;
                    if (value1->valueKind == value2->valueKind)
                        result.valueKind = value1->valueKind;
                    switch (parent->str()[0]) {
                    case '+':
                        result.intvalue = value1->intvalue + value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '-':
                        result.intvalue = value1->intvalue - value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '*':
                        result.intvalue = value1->intvalue * value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '/':
                        if (value2->intvalue == 0)
                            break;
                        result.intvalue = value1->intvalue / value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '%':
                        if (value2->intvalue == 0)
                            break;
                        result.intvalue = value1->intvalue % value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '=':
                        if (parent->str() == "==") {
                            if (value1->tokvalue || value2->tokvalue)
                                result.intvalue = 0;
                            else
                                result.intvalue = value1->intvalue == value2->intvalue;
                            setTokenValue(parent, result);
                        }
                        break;
                    case '!':
                        if (parent->str() == "!=") {
                            if (value1->tokvalue || value2->tokvalue)
                                result.intvalue = 1;
                            else
                                result.intvalue = value1->intvalue != value2->intvalue;
                            setTokenValue(parent, result);
                        }
                        break;
                    case '>':
                        if (value1->tokvalue || value2->tokvalue)
                            break;
                        if (parent->str() == ">")
                            result.intvalue = value1->intvalue > value2->intvalue;
                        else if (parent->str() == ">=")
                            result.intvalue = value1->intvalue >= value2->intvalue;
                        else
                            break;
                        setTokenValue(parent, result);
                        break;
                    case '<':
                        if (value1->tokvalue || value2->tokvalue)
                            break;
                        if (parent->str() == "<")
                            result.intvalue = value1->intvalue < value2->intvalue;
                        else if (parent->str() == "<=")
                            result.intvalue = value1->intvalue <= value2->intvalue;
                        else
                            break;
                        setTokenValue(parent, result);
                        break;
                    case '&':
                        result.intvalue = value1->intvalue & value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '|':
                        result.intvalue = value1->intvalue | value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    case '^':
                        result.intvalue = value1->intvalue ^ value2->intvalue;
                        setTokenValue(parent, result);
                        break;
                    default:
                        // unhandled operator, do nothing
                        break;
                    }
                }
            }
        }
    }

    // !
    else if (parent->str() == "!") {
        std::list<ValueFlow::Value>::const_iterator it;
        for (it = tok->values.begin(); it != tok->values.end(); ++it) {
            if (it->tokvalue)
                continue;
            ValueFlow::Value v(*it);
            v.intvalue = !v.intvalue;
            setTokenValue(parent, v);
        }
    }

    // Array element
    else if (parent->str() == "[" && parent->astOperand1() && parent->astOperand2()) {
        std::list<ValueFlow::Value>::const_iterator value1, value2;
        for (value1 = parent->astOperand1()->values.begin(); value1 != parent->astOperand1()->values.end(); ++value1) {
            if (!value1->tokvalue)
                continue;
            for (value2 = parent->astOperand2()->values.begin(); value2 != parent->astOperand2()->values.end(); ++value2) {
                if (value2->tokvalue)
                    continue;
                if (value1->varId == 0U || value2->varId == 0U ||
                    (value1->varId == value2->varId && value1->varvalue == value2->varvalue)) {
                    ValueFlow::Value result(0);
                    result.condition = value1->condition ? value1->condition : value2->condition;
                    result.inconclusive = value1->inconclusive | value2->inconclusive;
                    result.varId = (value1->varId != 0U) ? value1->varId : value2->varId;
                    result.varvalue = (result.varId == value1->varId) ? value1->intvalue : value2->intvalue;
                    if (value1->valueKind == value2->valueKind)
                        result.valueKind = value1->valueKind;
                    if (value1->tokvalue->tokType() == Token::eString) {
                        const std::string s = value1->tokvalue->strValue();
                        const MathLib::bigint index = value2->intvalue;
                        if (index == s.size()) {
                            result.intvalue = 0;
                            setTokenValue(parent, result);
                        } else if (index >= 0 && index < s.size()) {
                            result.intvalue = s[index];
                            setTokenValue(parent, result);
                        }
                    } else if (value1->tokvalue->str() == "{") {
                        MathLib::bigint index = value2->intvalue;
                        const Token *element = value1->tokvalue->next();
                        while (index > 0 && element->str() != "}") {
                            if (element->str() == ",")
                                --index;
                            if (Token::Match(element, "[{}()[]]"))
                                break;
                            element = element->next();
                        }
                        if (Token::Match(element, "%num% [,}]")) {
                            result.intvalue = MathLib::toLongNumber(element->str());
                            setTokenValue(parent, result);
                        }
                    }
                }
            }
        }
    }
}

static void valueFlowNumber(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if ((tok->isNumber() && MathLib::isInt(tok->str())) || (tok->tokType() == Token::eChar)) {
            ValueFlow::Value value(MathLib::toLongNumber(tok->str()));
            value.setKnown();
            setTokenValue(tok, value);
        }
    }

    if (tokenlist->isCPP()) {
        for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
            if (tok->isName() && !tok->varId() && Token::Match(tok, "false|true")) {
                ValueFlow::Value value(tok->str() == "true");
                value.setKnown();
                setTokenValue(tok, value);
            } else if (Token::Match(tok, "[(,] NULL [,)]")) {
                // NULL function parameters are not simplified in the
                // normal tokenlist
                ValueFlow::Value value(0);
                value.setKnown();
                setTokenValue(tok->next(), value);
            }
        }
    }
}

static void valueFlowString(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->tokType() == Token::eString) {
            ValueFlow::Value strvalue;
            strvalue.tokvalue = tok;
            strvalue.setKnown();
            setTokenValue(tok, strvalue);
        }
    }
}

static void valueFlowArray(TokenList *tokenlist)
{
    std::map<unsigned int, const Token *> constantArrays;

    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->varId() > 0U) {
            const std::map<unsigned int, const Token *>::const_iterator it = constantArrays.find(tok->varId());
            if (it != constantArrays.end()) {
                ValueFlow::Value value;
                value.tokvalue = it->second;
                value.setKnown();
                setTokenValue(tok, value);
            }

            // pointer = array
            else if (tok->variable() &&
                     tok->variable()->isArray() &&
                     Token::simpleMatch(tok->astParent(), "=") &&
                     tok == tok->astParent()->astOperand2() &&
                     tok->astParent()->astOperand1() &&
                     tok->astParent()->astOperand1()->variable() &&
                     tok->astParent()->astOperand1()->variable()->isPointer()) {
                ValueFlow::Value value;
                value.tokvalue = tok;
                value.setKnown();
                setTokenValue(tok, value);
            }
            continue;
        }

        if (Token::Match(tok, "const %type% %var% [ %num%| ] = {")) {
            const Token *vartok = tok->tokAt(2);
            const Token *rhstok = vartok->next()->link()->tokAt(2);
            constantArrays[vartok->varId()] = rhstok;
            tok = rhstok->link();
            continue;
        }

        else if (Token::Match(tok, "const char %var% [ %num%| ] = %str% ;")) {
            const Token *vartok = tok->tokAt(2);
            const Token *strtok = vartok->next()->link()->tokAt(2);
            constantArrays[vartok->varId()] = strtok;
            tok = strtok->next();
            continue;
        }
    }
}

static void valueFlowPointerAlias(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        // not address of
        if (tok->str() != "&" || tok->astOperand2())
            continue;

        // parent should be a '='
        if (!Token::simpleMatch(tok->astParent(), "="))
            continue;

        // child should be some buffer or variable
        const Token *vartok = tok->astOperand1();
        while (vartok) {
            if (vartok->str() == "[")
                vartok = vartok->astOperand1();
            else if (vartok->str() == "." || vartok->str() == "::")
                vartok = vartok->astOperand2();
            else
                break;
        }
        if (!(vartok && vartok->variable() && !vartok->variable()->isPointer()))
            continue;

        ValueFlow::Value value;
        value.tokvalue = tok;
        setTokenValue(tok, value);
    }
}

static void valueFlowBitAnd(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->str() != "&")
            continue;

        if (!tok->astOperand1() || !tok->astOperand2())
            continue;

        MathLib::bigint number;
        if (MathLib::isInt(tok->astOperand1()->str()))
            number = MathLib::toLongNumber(tok->astOperand1()->str());
        else if (MathLib::isInt(tok->astOperand2()->str()))
            number = MathLib::toLongNumber(tok->astOperand2()->str());
        else
            continue;

        int bit = 0;
        while (bit <= 60 && ((1LL<<bit) < number))
            ++bit;

        if ((1LL<<bit) == number) {
            setTokenValue(tok, ValueFlow::Value(0));
            setTokenValue(tok, ValueFlow::Value(number));
        }
    }
}

static void valueFlowOppositeCondition(SymbolDatabase *symboldatabase, const Settings *settings)
{
    for (std::list<Scope>::iterator scope = symboldatabase->scopeList.begin(); scope != symboldatabase->scopeList.end(); ++scope) {
        if (scope->type != Scope::eIf)
            continue;
        Token *tok = const_cast<Token *>(scope->classDef);
        if (!Token::simpleMatch(tok, "if ("))
            continue;
        const Token *cond1 = tok->next()->astOperand2();
        if (!cond1 || !cond1->isComparisonOp())
            continue;
        const bool cpp = symboldatabase->isCPP();
        Token *tok2 = tok->linkAt(1);
        while (Token::simpleMatch(tok2, ") {")) {
            tok2 = tok2->linkAt(1);
            if (!Token::simpleMatch(tok2, "} else { if ("))
                break;
            const Token *cond2 = tok2->tokAt(4)->astOperand2();
            if (!cond2 || !cond2->isComparisonOp())
                continue;
            if (isOppositeCond(true, cpp, cond1, cond2, settings->library.functionpure)) {
                ValueFlow::Value value(1);
                value.setKnown();
                setTokenValue(const_cast<Token*>(cond2), value);
            }
            tok2 = tok2->linkAt(4);
        }
    }
}

static void valueFlowReverse(TokenList *tokenlist,
                             Token *tok,
                             const Token * const varToken,
                             ValueFlow::Value val,
                             ValueFlow::Value val2,
                             ErrorLogger *errorLogger,
                             const Settings *settings)
{
    const MathLib::bigint    num        = val.intvalue;
    const Variable * const   var        = varToken->variable();
    if (!var)
        return;

    const unsigned int       varid      = varToken->varId();
    const Token * const      startToken = var->nameToken();

    for (Token *tok2 = tok->previous(); ; tok2 = tok2->previous()) {
        if (!tok2 ||
            tok2 == startToken ||
            (tok2->str() == "{" && tok2->scope()->type == Scope::ScopeType::eFunction)) {
            break;
        }

        if (tok2->varId() == varid) {
            // bailout: assignment
            if (Token::Match(tok2->previous(), "!!* %name% =")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "assignment of " + tok2->str());
                break;
            }

            // increment/decrement
            if (Token::Match(tok2->previous(), "[;{}] %name% ++|-- ;"))
                val.intvalue += (tok2->strAt(1)=="++") ? -1 : 1;
            else if (Token::Match(tok2->tokAt(-2), "[;{}] ++|-- %name% ;"))
                val.intvalue += (tok2->strAt(-1)=="++") ? -1 : 1;
            else if (Token::Match(tok2->previous(), "++|-- %name%") || Token::Match(tok2, "%name% ++|--")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "increment/decrement of " + tok2->str());
                break;
            }

            // bailout: variable is used in rhs in assignment to itself
            if (bailoutSelfAssignment(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + tok2->str() + " is used in rhs in assignment to itself");
                break;
            }

            if (Token::Match(tok2->previous(), "sizeof|.")) {
                const Token *prev = tok2->previous();
                while (Token::Match(prev,"%name%|.") && prev->str() != "sizeof")
                    prev = prev->previous();
                if (prev && prev->str() == "sizeof")
                    continue;
            }

            // assigned by subfunction?
            bool inconclusive = false;
            if (bailoutFunctionPar(tok2,val2.condition ? val2 : val, settings, &inconclusive)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "possible assignment of " + tok2->str() + " by subfunction");
                break;
            }
            val.inconclusive |= inconclusive;
            val2.inconclusive |= inconclusive;

            // skip if variable is conditionally used in ?: expression
            if (const Token *parent = skipValueInConditionalExpression(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok2,
                            "no simplification of " + tok2->str() + " within " + (Token::Match(parent,"[?:]") ? "?:" : parent->str()) + " expression");
                continue;
            }

            setTokenValue(tok2, val);
            if (val2.condition)
                setTokenValue(tok2,val2);
            if (tok2 == var->nameToken())
                break;
        }

        // skip sizeof..
        if (tok2->str() == ")" && Token::Match(tok2->link()->previous(), "typeof|sizeof ("))
            tok2 = tok2->link();

        // goto label
        if (Token::Match(tok2, "[;{}] %name% :")) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2->next(), "variable " + var->name() + " stopping on goto label");
            break;
        }

        if (tok2->str() == "}") {
            const Token *vartok = Token::findmatch(tok2->link(), "%varid%", tok2, varid);
            while (Token::Match(vartok, "%name% = %num% ;") && !vartok->tokAt(2)->getValue(num))
                vartok = Token::findmatch(vartok->next(), "%varid%", tok2, varid);
            if (vartok) {
                if (settings->debugwarnings) {
                    std::string errmsg = "variable ";
                    errmsg += var->name() + " ";
                    errmsg += "stopping on }";
                    bailout(tokenlist, errorLogger, tok2, errmsg);
                }
                break;
            } else {
                tok2 = tok2->link();
            }
        } else if (tok2->str() == "{") {
            // if variable is assigned in loop don't look before the loop
            if (tok2->previous() &&
                (Token::simpleMatch(tok2->previous(), "do") ||
                 (tok2->strAt(-1) == ")" && Token::Match(tok2->linkAt(-1)->previous(), "for|while (")))) {

                const Token *start = tok2;
                const Token *end   = start->link();
                if (isVariableChanged(start,end,varid)) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in loop. so valueflow analysis bailout when start of loop is reached.");
                    break;
                }
            }

            // Global variable : stop when leaving the function scope
            if (!var->isLocal()) {
                if (!Token::Match(tok2->previous(), ")|else|do {"))
                    break;
                if ((tok2->previous()->str() == ")") &&
                    !Token::Match(tok2->linkAt(-1)->previous(), "if|for|while ("))
                    break;
            }
        } else if (tok2->str() == ";") {
            const Token *parent = tok2->previous();
            while (parent && !Token::Match(parent, "return|break|continue|goto"))
                parent = parent->astParent();
            // reaching a break/continue/return
            if (parent) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " stopping on " + parent->str());
                break;
            }
        }
    }

}

static void valueFlowBeforeCondition(TokenList *tokenlist, SymbolDatabase *symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    const std::size_t functions = symboldatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symboldatabase->functionScopes[i];
        for (Token* tok = const_cast<Token*>(scope->classStart); tok != scope->classEnd; tok = tok->next()) {
            MathLib::bigint num = 0;
            const Token *vartok = nullptr;
            if (tok->isComparisonOp() && tok->astOperand1() && tok->astOperand2()) {
                if (tok->astOperand1()->isName() && tok->astOperand2()->isNumber()) {
                    vartok = tok->astOperand1();
                    num = MathLib::toLongNumber(tok->astOperand2()->str());
                } else if (tok->astOperand1()->isNumber() && tok->astOperand2()->isName()) {
                    vartok = tok->astOperand2();
                    num = MathLib::toLongNumber(tok->astOperand1()->str());
                } else {
                    continue;
                }
            } else if (Token::Match(tok->previous(), "if|while ( %name% %oror%|&&|)") ||
                       Token::Match(tok, "%oror%|&& %name% %oror%|&&|)")) {
                vartok = tok->next();
                num = 0;
            } else if (tok->str() == "!" && tok->astOperand1() && tok->astOperand1()->isName()) {
                vartok = tok->astOperand1();
                num = 0;
            } else {
                continue;
            }

            unsigned int varid = vartok->varId();
            const Variable * const var = vartok->variable();

            if (varid == 0U || !var)
                continue;

            // bailout: global non-const variables
            if (!(var->isLocal() || var->isArgument()) && !var->isConst()) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok, "global variable " + var->name());
                continue;
            }

            // bailout: for/while-condition, variable is changed in while loop
            for (const Token *tok2 = tok; tok2; tok2 = tok2->astParent()) {
                if (tok2->astParent() || tok2->str() != "(" || !Token::simpleMatch(tok2->link(), ") {"))
                    continue;

                // Variable changed in 3rd for-expression
                if (Token::simpleMatch(tok2->previous(), "for (")) {
                    if (tok2->astOperand2() && tok2->astOperand2()->astOperand2() && isVariableChanged(tok2->astOperand2()->astOperand2(), tok2->link(), varid)) {
                        varid = 0U;
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok, "variable " + var->name() + " used in loop");
                    }
                }

                // Variable changed in loop code
                if (Token::Match(tok2->previous(), "for|while (")) {
                    const Token * const start = tok2->link()->next();
                    const Token * const end   = start->link();

                    if (isVariableChanged(start,end,varid)) {
                        varid = 0U;
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok, "variable " + var->name() + " used in loop");
                    }
                }

                // if,macro => bailout
                else if (Token::simpleMatch(tok2->previous(), "if (") && tok2->previous()->isExpandedMacro()) {
                    varid = 0U;
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok, "variable " + var->name() + ", condition is defined in macro");
                }
            }
            if (varid == 0U)
                continue;

            // extra logic for unsigned variables 'i>=1' => possible value can also be 0
            if (Token::Match(tok, "<|>")) {
                if (num != 0)
                    continue;
                if (!var->typeStartToken()->isUnsigned())
                    continue;
            }
            ValueFlow::Value val(tok, num);
            val.varId = varid;
            ValueFlow::Value val2;
            if (num==1U && Token::Match(tok,"<=|>=")) {
                if (var->typeStartToken()->isUnsigned()) {
                    val2 = ValueFlow::Value(tok,0);
                    val2.varId = varid;
                }
            }
            valueFlowReverse(tokenlist,
                             tok,
                             vartok,
                             val,
                             val2,
                             errorLogger,
                             settings);

        }
    }
}

static void removeValues(std::list<ValueFlow::Value> &values, const std::list<ValueFlow::Value> &valuesToRemove)
{
    for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end();) {
        bool found = false;
        for (std::list<ValueFlow::Value>::const_iterator it2 = valuesToRemove.begin(); it2 != valuesToRemove.end(); ++it2) {
            if (it->intvalue == it2->intvalue) {
                found = true;
                break;
            }
        }
        if (found)
            values.erase(it++);
        else
            ++it;
    }
}

static void valueFlowAST(Token *tok, unsigned int varid, const ValueFlow::Value &value)
{
    if (!tok)
        return;
    if (tok->varId() == varid)
        setTokenValue(tok, value);
    valueFlowAST(const_cast<Token*>(tok->astOperand1()), varid, value);
    valueFlowAST(const_cast<Token*>(tok->astOperand2()), varid, value);
}

/** if known variable is changed in loop body, change it to a possible value */
static void handleKnownValuesInLoop(const Token                 *startToken,
                                    const Token                 *endToken,
                                    std::list<ValueFlow::Value> *values,
                                    unsigned int                varid)
{
    bool isChanged = false;
    for (std::list<ValueFlow::Value>::iterator it = values->begin(); it != values->end(); ++it) {
        if (it->isKnown()) {
            if (!isChanged) {
                if (!isVariableChanged(startToken, endToken, varid))
                    break;
                isChanged = true;
            }

            it->setPossible();
        }
    }
}

static bool valueFlowForward(Token * const               startToken,
                             const Token * const         endToken,
                             const Variable * const      var,
                             const unsigned int          varid,
                             std::list<ValueFlow::Value> values,
                             const bool                  constValue,
                             TokenList * const           tokenlist,
                             ErrorLogger * const         errorLogger,
                             const Settings * const      settings)
{
    int indentlevel = 0;
    unsigned int number_of_if = 0;
    int varusagelevel = -1;
    bool returnStatement = false;  // current statement is a return, stop analysis at the ";"
    bool read = false;  // is variable value read?

    for (Token *tok2 = startToken; tok2 && tok2 != endToken; tok2 = tok2->next()) {
        if (indentlevel >= 0 && tok2->str() == "{")
            ++indentlevel;
        else if (indentlevel >= 0 && tok2->str() == "}") {
            --indentlevel;
            if (indentlevel <= 0 && isReturnScope(tok2) && Token::Match(tok2->link()->previous(), "else|) {")) {
                const Token *condition = tok2->link();
                const bool iselse = Token::simpleMatch(condition->tokAt(-2), "} else {");
                if (iselse)
                    condition = condition->linkAt(-2);
                if (condition && Token::simpleMatch(condition->previous(), ") {"))
                    condition = condition->linkAt(-1)->astOperand2();
                else
                    condition = nullptr;
                if (!condition) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, bailing out since it's unknown if conditional return is executed");
                    return false;
                }

                bool bailoutflag = false;
                for (std::list<ValueFlow::Value>::const_iterator it = values.begin(); it != values.end(); ++it) {
                    if (!iselse && conditionIsTrue(condition, getProgramMemory(condition->astParent(), varid, *it))) {
                        bailoutflag = true;
                        break;
                    }
                    if (iselse && conditionIsFalse(condition, getProgramMemory(condition->astParent(), varid, *it))) {
                        bailoutflag = true;
                        break;
                    }
                }
                if (bailoutflag) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, conditional return is assumed to be executed");
                    return false;
                }
            }
        }

        if (Token::Match(tok2, "[;{}] %name% :") || tok2->str() == "case") {
            for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end(); ++it)
                it->changeKnownToPossible();
            tok2 = tok2->tokAt(2);
            continue;
        }

        if (Token::Match(tok2, "sizeof|typeof|typeid ("))
            tok2 = tok2->linkAt(1);

        else if (Token::simpleMatch(tok2, "else {")) {
            // Should scope be skipped because variable value is checked?
            bool skipelse = false;
            const Token *condition = tok2->linkAt(-1);
            condition = condition ? condition->linkAt(-1) : nullptr;
            condition = condition ? condition->astOperand2() : nullptr;
            for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end(); ++it) {
                if (conditionIsTrue(condition, getProgramMemory(tok2, varid, *it))) {
                    skipelse = true;
                    break;
                }
            }
            if (skipelse) {
                tok2 = tok2->linkAt(1);
                continue;
            }
        }

        else if (Token::simpleMatch(tok2, "do {")) {
            const Token *start = tok2->next();
            const Token *end   = start->link();
            if (Token::simpleMatch(end, "} while ("))
                end = end->linkAt(2);

            if (isVariableChanged(start, end, varid)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, assignment in do-while");
                return false;
            }

            handleKnownValuesInLoop(start, end, &values, varid);
        }

        // conditional block of code that assigns variable..
        else if (Token::Match(tok2, "%name% (") && Token::simpleMatch(tok2->linkAt(1), ") {")) {
            // is variable changed in condition?
            if (isVariableChanged(tok2->next(), tok2->next()->link(), varid)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, assignment in condition");
                return false;
            }

            // if known variable is changed in loop body, change it to a possible value..
            if (Token::Match(tok2, "for|while"))
                handleKnownValuesInLoop(tok2, tok2->linkAt(1)->linkAt(1), &values, varid);

            // Set values in condition
            for (Token* tok3 = tok2->tokAt(2); tok3 != tok2->next()->link(); tok3 = tok3->next()) {
                if (tok3->varId() == varid) {
                    for (std::list<ValueFlow::Value>::const_iterator it = values.begin(); it != values.end(); ++it)
                        setTokenValue(tok3, *it);
                } else if (Token::Match(tok3, "%oror%|&&|?|;")) {
                    break;
                }
            }

            const Token * const condTok = tok2->next()->astOperand2();
            const bool condAlwaysTrue = (condTok && condTok->values.size() == 1U && condTok->values.front().isKnown() && condTok->values.front().intvalue != 0);

            // Should scope be skipped because variable value is checked?
            std::list<ValueFlow::Value> truevalues;
            for (std::list<ValueFlow::Value>::const_iterator it = values.begin(); it != values.end(); ++it) {
                if (condAlwaysTrue || !conditionIsFalse(condTok, getProgramMemory(tok2, varid, *it)))
                    truevalues.push_back(*it);
            }
            if (truevalues.size() != values.size() || condAlwaysTrue) {
                // '{'
                Token * const startToken1 = tok2->linkAt(1)->next();

                valueFlowForward(startToken1->next(),
                                 startToken1->link(),
                                 var,
                                 varid,
                                 truevalues,
                                 constValue,
                                 tokenlist,
                                 errorLogger,
                                 settings);

                if (isVariableChanged(startToken1, startToken1->link(), varid)) {
                    removeValues(values, truevalues);

                    std::list<ValueFlow::Value>::iterator it;
                    for (it = values.begin(); it != values.end(); ++it)
                        it->changeKnownToPossible();
                }

                // goto '}'
                tok2 = startToken1->link();

                if (condAlwaysTrue && isReturnScope(tok2))
                    return false;

                continue;
            }

            Token * const start = tok2->linkAt(1)->next();
            Token * const end   = start->link();
            bool varusage = (indentlevel >= 0 && constValue && number_of_if == 0U) ?
                            isVariableChanged(start,end,varid) :
                            (nullptr != Token::findmatch(start, "%varid%", end, varid));
            if (!read) {
                read = bool(nullptr != Token::findmatch(tok2, "%varid% !!=", end, varid));
            }
            if (varusage) {
                varusagelevel = indentlevel;

                if (indentlevel < 0 && tok2->str() == "switch")
                    return false;

                // TODO: don't check noreturn scopes
                if (read && (number_of_if > 0U || Token::findmatch(tok2, "%varid%", start, varid))) {
                    // Set values in condition
                    const Token * const condend = tok2->linkAt(1);
                    for (Token *condtok = tok2; condtok != condend; condtok = condtok->next()) {
                        if (condtok->varId() == varid) {
                            std::list<ValueFlow::Value>::const_iterator it;
                            for (it = values.begin(); it != values.end(); ++it)
                                setTokenValue(condtok, *it);
                        }
                        if (Token::Match(condtok, "%oror%|&&|?|;"))
                            break;
                    }
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in conditional code");
                    return false;
                }

                if (var->isStatic()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " bailout when conditional code that contains var is seen");
                    return false;
                }

                // Remove conditional values
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end();) {
                    if (it->condition || it->conditional)
                        values.erase(it++);
                    else {
                        it->changeKnownToPossible();
                        ++it;
                    }
                }
            }

            // stop after conditional return scopes that are executed
            if (isReturnScope(end)) {
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end();) {
                    if (conditionIsTrue(tok2->next()->astOperand2(), getProgramMemory(tok2, varid, *it)))
                        values.erase(it++);
                    else
                        ++it;
                }
                if (values.empty())
                    return false;
            }

            // noreturn scopes..
            if ((number_of_if > 0 || Token::findmatch(tok2, "%varid%", start, varid)) &&
                (Token::findmatch(start, "return|continue|break|throw", end) ||
                 (Token::simpleMatch(end,"} else {") && Token::findmatch(end, "return|continue|break|throw", end->linkAt(2))))) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + ". noreturn conditional scope.");
                return false;
            }

            if (isVariableChanged(start, end, varid)) {
                if ((!read || number_of_if == 0) &&
                    Token::simpleMatch(tok2, "if (") &&
                    !(Token::simpleMatch(end, "} else {") &&
                      (Token::findmatch(end, "%varid%", end->linkAt(2), varid) ||
                       Token::findmatch(end, "return|continue|break|throw", end->linkAt(2))))) {
                    ++number_of_if;
                    tok2 = end;
                } else {
                    // loop that conditionally set variable and then break => either loop condition is
                    // redundant or the variable can be unchanged after the loop.
                    bool loopCondition = false;
                    if (Token::simpleMatch(tok2, "while (") && Token::Match(tok2->next()->astOperand2(), "%op%"))
                        loopCondition = true;
                    else if (Token::simpleMatch(tok2, "for (") &&
                             Token::simpleMatch(tok2->next()->astOperand2(), ";") &&
                             Token::simpleMatch(tok2->next()->astOperand2()->astOperand2(), ";") &&
                             Token::Match(tok2->next()->astOperand2()->astOperand2()->astOperand1(), "%op%"))
                        loopCondition = true;

                    bool bail = true;
                    if (loopCondition) {
                        const Token *tok3 = Token::findmatch(start, "%varid%", end, varid);
                        if (Token::Match(tok3, "%varid% =", varid) &&
                            tok3->scope()->classEnd                &&
                            Token::Match(tok3->scope()->classEnd->tokAt(-3), "[;}] break ;") &&
                            !Token::findmatch(tok3->next(), "%varid%", end, varid)) {
                            bail = false;
                            tok2 = end;
                        }
                    }

                    if (bail) {
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in conditional code");
                        return false;
                    }
                }
            }
        }

        else if (tok2->str() == "}" && indentlevel == varusagelevel) {
            ++number_of_if;

            // Set "conditional" flag for all values
            std::list<ValueFlow::Value>::iterator it;
            for (it = values.begin(); it != values.end(); ++it) {
                it->conditional = true;
                it->changeKnownToPossible();
            }

            if (Token::simpleMatch(tok2,"} else {"))
                tok2 = tok2->linkAt(2);
        }

        else if (Token::Match(tok2, "break|continue|goto")) {
            const Scope *scope = tok2->scope();
            if (indentlevel > 0) {
                const Token *tok3 = tok2->tokAt(2);
                int indentlevel2 = indentlevel;
                while (indentlevel2 > 0 &&
                       tok3->str() == "}" &&
                       Token::Match(tok3->link()->previous(), "!!)")) {
                    indentlevel2--;
                    tok3 = tok3->next();
                    if (tok3 && tok3->str() == ";")
                        tok3 = tok3->next();
                }
                if (indentlevel2 > 0)
                    continue;
                scope = tok3->scope();
                indentlevel = 0;
            }
            if (tok2->str() == "break") {
                if (scope && scope->type == Scope::eSwitch) {
                    tok2 = const_cast<Token *>(scope->classEnd);
                    if (tok2 == endToken)
                        break;
                    --indentlevel;
                    for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end(); ++it) {
                        it->changeKnownToPossible();
                    }
                    continue;
                }
            }
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + ". noreturn conditional scope.");
            return false;
        }

        else if (indentlevel <= 0 && Token::Match(tok2, "return|throw"))
            returnStatement = true;

        else if (returnStatement && tok2->str() == ";")
            return false;

        // If a ? is seen and it's known that the condition is true/false..
        else if (tok2->str() == "?") {
            const Token *condition = tok2->astOperand1();
            const Token *op2 = tok2->astOperand2();
            if (!condition || !op2) // Ticket #6713
                continue;

            if (condition->values.size() == 1U && condition->values.front().isKnown() && !condition->values.front().tokvalue) {
                const ValueFlow::Value &condValue = condition->values.front();
                const Token *expr = (condValue.intvalue != 0) ? op2->astOperand1() : op2->astOperand2();
                std::list<ValueFlow::Value>::const_iterator it;
                for (it = values.begin(); it != values.end(); ++it)
                    valueFlowAST(const_cast<Token*>(expr), varid, *it);
            } else {
                std::list<ValueFlow::Value>::const_iterator it;
                for (it = values.begin(); it != values.end(); ++it) {
                    const ProgramMemory programMemory(getProgramMemory(tok2, varid, *it));
                    if (conditionIsTrue(condition, programMemory))
                        valueFlowAST(const_cast<Token*>(op2->astOperand1()), varid, *it);
                    else if (conditionIsFalse(condition, programMemory))
                        valueFlowAST(const_cast<Token*>(op2->astOperand2()), varid, *it);
                    else
                        valueFlowAST(const_cast<Token*>(op2), varid, *it);
                }
            }
            // Skip conditional expressions..
            while (tok2->astOperand1() || tok2->astOperand2()) {
                if (tok2->astOperand2())
                    tok2 = const_cast<Token*>(tok2->astOperand2());
                else if (tok2->isUnaryPreOp())
                    tok2 = const_cast<Token*>(tok2->astOperand1());
                else
                    break;
            }
            tok2 = tok2->next();
        }

        if (!tok2) // invalid code #7236
            return false;
        if (tok2->varId() == varid) {
            // bailout: assignment
            if (Token::Match(tok2->previous(), "!!* %name% %op%") && tok2->next()->isAssignmentOp()) {
                // simplify rhs
                for (Token *tok3 = tok2->tokAt(2); tok3; tok3 = tok3->next()) {
                    if (tok3->varId() == varid) {
                        std::list<ValueFlow::Value>::const_iterator it;
                        for (it = values.begin(); it != values.end(); ++it)
                            setTokenValue(tok3, *it);
                    } else if (Token::Match(tok3, "++|--|?|:|;"))
                        break;
                }
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "assignment of " + tok2->str());
                return false;
            }

            // bailout: possible assignment using >>
            if (Token::Match(tok2->previous(), ">> %name% >>|;")) {
                const Token *parent = tok2->previous();
                do {
                    parent = parent->astParent();
                } while (Token::simpleMatch(parent, ">>"));
                if (!parent) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "Possible assignment of " + tok2->str() + " using >>");
                    return false;
                }
            }

            // skip if variable is conditionally used in ?: expression
            if (const Token *parent = skipValueInConditionalExpression(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok2,
                            "no simplification of " + tok2->str() + " within " + (Token::Match(parent,"[?:]") ? "?:" : parent->str()) + " expression");
                const Token *astTop = parent->astTop();
                if (Token::simpleMatch(astTop->astOperand1(), "for ("))
                    tok2 = const_cast<Token*>(astTop->link());
                continue;
            }

            {
                // Is variable usage protected by && || ?:
                const Token *tok3 = tok2;
                const Token *parent = tok3->astParent();
                while (parent && !Token::Match(parent, "%oror%|&&|:")) {
                    tok3 = parent;
                    parent = parent->astParent();
                }
                const bool conditional = parent && (parent->str() == ":" || parent->astOperand2() == tok3);

                std::list<ValueFlow::Value>::const_iterator it;
                for (it = values.begin(); it != values.end(); ++it) {
                    if (!conditional || !it->conditional)
                        setTokenValue(tok2, *it);
                }
            }

            // increment/decrement
            if (Token::Match(tok2->previous(), "++|-- %name%") || Token::Match(tok2, "%name% ++|--")) {
                std::list<ValueFlow::Value>::iterator it;
                // Erase values that are not int values..
                for (it = values.begin(); it != values.end();) {
                    if (it->tokvalue)
                        it = values.erase(it);
                    else
                        ++it;
                }
                if (values.empty()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "increment/decrement of " + tok2->str());
                    return false;
                }
                const bool pre   = Token::Match(tok2->previous(), "++|--");
                Token * const op = pre ? tok2->previous() : tok2->next();
                const bool inc   = (op->str() == "++");
                // Perform increment/decrement..
                for (it = values.begin(); it != values.end(); ++it) {
                    if (!pre)
                        setTokenValue(op, *it);
                    it->intvalue += (inc ? 1 : -1);
                    if (pre)
                        setTokenValue(op, *it);
                }
            }

            // bailout if address of var is taken..
            if (tok2->astParent() && tok2->astParent()->str() == "&" && !tok2->astParent()->astOperand2()) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "Taking address of " + tok2->str());
                return false;
            }

            // bailout if reference is created..
            if (tok2->astParent() && Token::Match(tok2->astParent()->tokAt(-2), "& %name% =")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "Reference of " + tok2->str());
                return false;
            }

            // assigned by subfunction?
            bool inconclusive = false;
            if (bailoutFunctionPar(tok2, ValueFlow::Value(), settings, &inconclusive)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "possible assignment of " + tok2->str() + " by subfunction");
                return false;
            }
            if (inconclusive) {
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end(); ++it) {
                    it->inconclusive = true;
                    it->changeKnownToPossible();
                }
            }
        }

        // Lambda function
        if (Token::simpleMatch(tok2, "= [") &&
            Token::simpleMatch(tok2->linkAt(1), "] (") &&
            Token::simpleMatch(tok2->linkAt(1)->linkAt(1), ") {")) {
            const Token *bodyStart = tok2->linkAt(1)->linkAt(1)->next();
            if (isVariableChanged(bodyStart, bodyStart->link(), varid)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "valueFlowForward, " + var->name() + " is changed in lambda function");
                return false;
            }
        }
    }
    return true;
}

static void valueFlowAfterAssign(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    const std::size_t functions = symboldatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symboldatabase->functionScopes[i];
        for (Token* tok = const_cast<Token*>(scope->classStart); tok != scope->classEnd; tok = tok->next()) {
            // Assignment
            if ((tok->str() != "=") || (tok->astParent()))
                continue;

            // Lhs should be a variable
            if (!tok->astOperand1() || !tok->astOperand1()->varId())
                continue;
            const unsigned int varid = tok->astOperand1()->varId();
            const Variable *var = tok->astOperand1()->variable();
            if (!var || (!var->isLocal() && !var->isArgument()))
                continue;

            const Token * const endOfVarScope = var->typeStartToken()->scope()->classEnd;

            // Rhs values..
            if (!tok->astOperand2() || tok->astOperand2()->values.empty())
                continue;

            std::list<ValueFlow::Value> values = tok->astOperand2()->values;
            const bool constValue = tok->astOperand2()->isNumber();

            // Static variable initialisation?
            if (var->isStatic() && var->nameToken() == tok->astOperand1()) {
                for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end(); ++it) {
                    it->changeKnownToPossible();
                }
            }

            valueFlowForward(tok, endOfVarScope, var, varid, values, constValue, tokenlist, errorLogger, settings);
        }
    }
}

static void valueFlowAfterCondition(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    const std::size_t functions = symboldatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symboldatabase->functionScopes[i];
        for (Token* tok = const_cast<Token*>(scope->classStart); tok != scope->classEnd; tok = tok->next()) {
            const Token *vartok, *numtok;

            // Comparison
            if (Token::Match(tok, "==|!=|>=|<=")) {
                if (!tok->astOperand1() || !tok->astOperand2())
                    continue;
                if (tok->astOperand1()->isNumber()) {
                    numtok = tok->astOperand1();
                    vartok = tok->astOperand2();
                } else {
                    numtok = tok->astOperand2();
                    vartok = tok->astOperand1();
                }
                if (vartok->str() == "=" && vartok->astOperand1() && vartok->astOperand2())
                    vartok = vartok->astOperand1();
                if (!vartok->isName() || !numtok->isNumber() || !MathLib::isInt(numtok->str()))
                    continue;
            } else if (tok->str() == "!") {
                vartok = tok->astOperand1();
                numtok = nullptr;
                if (!vartok || !vartok->isName())
                    continue;

            } else if (tok->isName() &&
                       (Token::Match(tok->astParent(), "%oror%|&&") ||
                        Token::Match(tok->tokAt(-2), "if|while ( %var% [)=]"))) {
                vartok = tok;
                numtok = nullptr;

            } else {
                continue;
            }

            const unsigned int varid = vartok->varId();
            if (varid == 0U)
                continue;
            const Variable *var = vartok->variable();
            if (!var || !(var->isLocal() || var->isArgument()))
                continue;
            std::list<ValueFlow::Value> values;
            values.push_back(ValueFlow::Value(tok, numtok ? MathLib::toLongNumber(numtok->str()) : 0LL));

            if (Token::Match(tok->astParent(), "%oror%|&&")) {
                Token *parent = const_cast<Token*>(tok->astParent());
                const std::string &op(parent->str());

                if (parent->astOperand1() == tok &&
                    ((op == "&&" && Token::Match(tok, "==|>=|<=|!")) ||
                     (op == "||" && Token::Match(tok, "%name%|!=")))) {
                    for (; parent && parent->str() == op; parent = const_cast<Token*>(parent->astParent())) {
                        std::stack<Token *> tokens;
                        tokens.push(const_cast<Token*>(parent->astOperand2()));
                        bool assign = false;
                        while (!tokens.empty()) {
                            Token *rhstok = tokens.top();
                            tokens.pop();
                            if (!rhstok)
                                continue;
                            tokens.push(const_cast<Token*>(rhstok->astOperand1()));
                            tokens.push(const_cast<Token*>(rhstok->astOperand2()));
                            if (rhstok->varId() == varid)
                                setTokenValue(rhstok, values.front());
                            else if (Token::Match(rhstok, "++|--|=") && Token::Match(rhstok->astOperand1(), "%varid%", varid)) {
                                assign = true;
                                break;
                            }
                        }
                        if (assign)
                            break;
                        while (parent->astParent() && parent == parent->astParent()->astOperand2())
                            parent = const_cast<Token*>(parent->astParent());
                    }
                }
            }

            const Token *top = tok->astTop();
            if (top && Token::Match(top->previous(), "if|while (") && !top->previous()->isExpandedMacro()) {
                // does condition reassign variable?
                if (tok != top->astOperand2() &&
                    Token::Match(top->astOperand2(), "%oror%|&&") &&
                    isVariableChanged(top, top->link(), varid)) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok, "assignment in condition");
                    continue;
                }

                // start token of conditional code
                Token *startToken = nullptr;

                // based on the comparison, should we check the if or while?
                int codeblock = 0;
                if (Token::Match(tok, "==|>=|<=|!"))
                    codeblock = 1;
                else if (Token::Match(tok, "%name%|!="))
                    codeblock = 2;

                // determine startToken based on codeblock
                if (codeblock > 0) {
                    // if astParent is "!" we need to invert codeblock
                    const Token *parent = tok->astParent();
                    while (parent && parent->str() == "&&")
                        parent = parent->astParent();
                    if (parent && parent->str() == "!")
                        codeblock = (codeblock == 1) ? 2 : 1;

                    // convert codeblock to a startToken
                    if (codeblock == 1 && Token::simpleMatch(top->link(), ") {"))
                        startToken = top->link()->next();
                    else if (Token::simpleMatch(top->link()->linkAt(1), "} else {"))
                        startToken = top->link()->linkAt(1)->tokAt(2);
                }

                if (startToken) {
                    if (!valueFlowForward(startToken->next(), startToken->link(), var, varid, values, true, tokenlist, errorLogger, settings))
                        continue;
                    if (isVariableChanged(startToken, startToken->link(), varid)) {
                        // TODO: The endToken should not be startToken->link() in the valueFlowForward call
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, startToken->link(), "valueFlowAfterCondition: " + var->name() + " is changed in conditional block");
                        continue;
                    }
                }

                // After conditional code..
                if (Token::simpleMatch(top->link(), ") {")) {
                    Token *after = top->link()->linkAt(1);
                    std::string unknownFunction;
                    if (settings->library.isScopeNoReturn(after, &unknownFunction)) {
                        if (settings->debugwarnings && !unknownFunction.empty())
                            bailout(tokenlist, errorLogger, after, "possible noreturn scope");
                        continue;
                    }

                    bool isreturn = (codeblock == 1 && isReturnScope(after));

                    if (Token::simpleMatch(after, "} else {")) {
                        after = after->linkAt(2);
                        if (Token::simpleMatch(after->tokAt(-2), ") ; }")) {
                            if (settings->debugwarnings)
                                bailout(tokenlist, errorLogger, after, "possible noreturn scope");
                            continue;
                        }
                        isreturn |= (codeblock == 2 && isReturnScope(after));
                    }

                    if (!isreturn) {
                        // TODO: constValue could be true if there are no assignments in the conditional blocks and
                        //       perhaps if there are no && and no || in the condition
                        bool constValue = false;
                        valueFlowForward(after->next(), top->scope()->classEnd, var, varid, values, constValue, tokenlist, errorLogger, settings);
                    }
                }
            }
        }
    }
}

static void execute(const Token *expr,
                    ProgramMemory * const programMemory,
                    MathLib::bigint *result,
                    bool *error)
{
    if (!expr)
        *error = true;

    else if (expr->values.size() == 1U && expr->values.front().isKnown() && !expr->values.front().tokvalue) {
        *result = expr->values.front().intvalue;
    }

    else if (expr->isNumber()) {
        *result = MathLib::toLongNumber(expr->str());
        if (MathLib::isFloat(expr->str()))
            *error = true;
    }

    else if (expr->varId() > 0) {
        if (!programMemory->getIntValue(expr->varId(), result))
            *error = true;
    }

    else if (expr->isComparisonOp()) {
        MathLib::bigint result1(0), result2(0);
        execute(expr->astOperand1(), programMemory, &result1, error);
        execute(expr->astOperand2(), programMemory, &result2, error);
        if (expr->str() == "<")
            *result = result1 < result2;
        else if (expr->str() == "<=")
            *result = result1 <= result2;
        else if (expr->str() == ">")
            *result = result1 > result2;
        else if (expr->str() == ">=")
            *result = result1 >= result2;
        else if (expr->str() == "==")
            *result = result1 == result2;
        else if (expr->str() == "!=")
            *result = result1 != result2;
    }

    else if (expr->str() == "=") {
        execute(expr->astOperand2(), programMemory, result, error);
        if (!*error && expr->astOperand1() && expr->astOperand1()->varId())
            programMemory->setIntValue(expr->astOperand1()->varId(), *result);
        else
            *error = true;
    }

    else if (Token::Match(expr, "++|--")) {
        if (!expr->astOperand1() || expr->astOperand1()->varId() == 0U)
            *error = true;
        else {
            std::map<unsigned int, MathLib::bigint>::iterator var = programMemory->intvalues.find(expr->astOperand1()->varId());
            if (var == programMemory->intvalues.end())
                *error = true;
            else {
                if (var->second == 0 &&
                    expr->str() == "--" &&
                    expr->astOperand1()->variable() &&
                    expr->astOperand1()->variable()->typeStartToken()->isUnsigned())
                    *error = true; // overflow
                *result = var->second + (expr->str() == "++" ? 1 : -1);
                var->second = *result;
            }
        }
    }

    else if (expr->isArithmeticalOp() && expr->astOperand1() && expr->astOperand2()) {
        MathLib::bigint result1(0), result2(0);
        execute(expr->astOperand1(), programMemory, &result1, error);
        execute(expr->astOperand2(), programMemory, &result2, error);
        if (expr->str() == "+")
            *result = result1 + result2;
        else if (expr->str() == "-")
            *result = result1 - result2;
        else if (expr->str() == "*") {
            if (result2 && (result1 > std::numeric_limits<MathLib::bigint>::max()/result2))
                *error = true;
            else
                *result = result1 * result2;
        } else if (result2 == 0)
            *error = true;
        else if (expr->str() == "/")
            *result = result1 / result2;
        else if (expr->str() == "%")
            *result = result1 % result2;
    }

    else if (expr->str() == "&&") {
        bool error1 = false;
        execute(expr->astOperand1(), programMemory, result, &error1);
        if (!error1 && *result == 0)
            *result = 0;
        else {
            bool error2 = false;
            execute(expr->astOperand2(), programMemory, result, &error2);
            if (error1 && error2)
                *error = true;
            if (error2)
                *result = 1;
            else
                *result = !!*result;
        }
    }

    else if (expr->str() == "||") {
        execute(expr->astOperand1(), programMemory, result, error);
        if (*result == 0 && *error == false)
            execute(expr->astOperand2(), programMemory, result, error);
    }

    else if (expr->str() == "!") {
        execute(expr->astOperand1(), programMemory, result, error);
        *result = !(*result);
    }

    else if (expr->str() == "," && expr->astOperand1() && expr->astOperand2()) {
        execute(expr->astOperand1(), programMemory, result, error);
        execute(expr->astOperand2(), programMemory, result, error);
    }

    else if (expr->str() == "[" && expr->astOperand1() && expr->astOperand2()) {
        const Token *tokvalue = nullptr;
        std::map<unsigned int, const Token *>::iterator var = programMemory->tokvalues.find(expr->astOperand1()->varId());
        if (var != programMemory->tokvalues.end()) {
            tokvalue = var->second;
        } else {
            if (expr->astOperand1()->values.size() != 1U) {
                *error = true;
                return;
            }
            tokvalue = expr->astOperand1()->values.front().tokvalue;
        }
        if (!tokvalue || !tokvalue->isLiteral()) {
            *error = true;
            return;
        }
        const std::string strValue = tokvalue->strValue();
        MathLib::bigint index = 0;
        execute(expr->astOperand2(), programMemory, &index, error);
        if (index >= 0 && index < strValue.size())
            *result = strValue[index];
        else if (index == strValue.size())
            *result = 0;
        else
            *error = true;
    }

    else
        *error = true;
}

static bool valueFlowForLoop1(const Token *tok, unsigned int * const varid, MathLib::bigint * const num1, MathLib::bigint * const num2, MathLib::bigint * const numAfter)
{
    tok = tok->tokAt(2);
    if (!Token::Match(tok, "%type%| %var% ="))
        return false;
    const Token * const vartok = Token::Match(tok, "%var% =") ? tok : tok->next();
    *varid = vartok->varId();
    tok = vartok->tokAt(2);
    const Token * const num1tok = Token::Match(tok, "%num% ;") ? tok : nullptr;
    if (num1tok)
        *num1 = MathLib::toLongNumber(num1tok->str());
    while (Token::Match(tok, "%name%|%num%|%or%|+|-|*|/|&|[|]|("))
        tok = (tok->str() == "(") ? tok->link()->next() : tok->next();
    if (!tok || tok->str() != ";")
        return false;
    tok = tok->next();
    const Token *num2tok = nullptr;
    if (Token::Match(tok, "%varid% <|<=|!=", vartok->varId())) {
        tok = tok->next();
        num2tok = tok->astOperand2();
        if (num2tok && num2tok->str() == "(" && !num2tok->astOperand2())
            num2tok = num2tok->astOperand1();
        if (!Token::Match(num2tok, "%num% ;|%oror%")) // TODO: || enlarges the scope of the condition, so it should not cause FP, but it should no lnger be part of this pattern as soon as valueFlowForLoop2 can handle an unknown RHS of || better
            num2tok = 0;
    }
    if (!num2tok)
        return false;
    *num2 = MathLib::toLongNumber(num2tok->str()) - ((tok->str()=="<=") ? 0 : 1);
    *numAfter = *num2 + 1;
    if (!num1tok)
        *num1 = *num2;
    while (tok && tok->str() != ";")
        tok = tok->next();
    if (!Token::Match(tok, "; %varid% ++ ) {", vartok->varId()) && !Token::Match(tok, "; ++ %varid% ) {", vartok->varId()))
        return false;
    return true;
}

static bool valueFlowForLoop2(const Token *tok,
                              ProgramMemory *memory1,
                              ProgramMemory *memory2,
                              ProgramMemory *memoryAfter)
{
    // for ( firstExpression ; secondExpression ; thirdExpression )
    const Token *firstExpression  = tok->next()->astOperand2()->astOperand1();
    const Token *secondExpression = tok->next()->astOperand2()->astOperand2()->astOperand1();
    const Token *thirdExpression = tok->next()->astOperand2()->astOperand2()->astOperand2();

    ProgramMemory programMemory;
    MathLib::bigint result(0);
    bool error = false;
    execute(firstExpression, &programMemory, &result, &error);
    if (error)
        return false;
    execute(secondExpression, &programMemory, &result, &error);
    if (error) {
        // If a variable is reassigned in second expression, return false
        std::stack<const Token *> tokens;
        tokens.push(secondExpression);
        while (!tokens.empty()) {
            const Token *t = tokens.top();
            tokens.pop();
            if (!t)
                continue;
            if (t->str() == "=" && t->astOperand1() && programMemory.hasValue(t->astOperand1()->varId()))
                // TODO: investigate what variable is assigned.
                return false;
            tokens.push(t->astOperand1());
            tokens.push(t->astOperand2());
        }
    }

    ProgramMemory startMemory(programMemory);
    ProgramMemory endMemory;

    unsigned int maxcount = 10000;
    while (result != 0 && !error && --maxcount) {
        endMemory = programMemory;
        execute(thirdExpression, &programMemory, &result, &error);
        if (!error)
            execute(secondExpression, &programMemory, &result, &error);
    }

    memory1->swap(startMemory);
    if (!error) {
        memory2->swap(endMemory);
        memoryAfter->swap(programMemory);
    }

    return true;
}

static void valueFlowForLoopSimplify(Token * const bodyStart, const unsigned int varid, const MathLib::bigint value, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    const Token * const bodyEnd = bodyStart->link();

    // Is variable modified inside for loop
    if (isVariableChanged(bodyStart, bodyEnd, varid))
        return;

    for (Token *tok2 = bodyStart->next(); tok2 != bodyEnd; tok2 = tok2->next()) {
        if (tok2->varId() == varid) {
            const Token * parent = tok2->astParent();
            while (parent) {
                const Token * const p = parent;
                parent = parent->astParent();
                if (!parent || parent->str() == ":")
                    break;
                if (parent->str() == "?") {
                    if (parent->astOperand2() != p)
                        parent = nullptr;
                    break;
                }
            }
            if (parent) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "For loop variable " + tok2->str() + " stopping on ?");
                continue;
            }

            ValueFlow::Value value1(value);
            value1.varId = tok2->varId();
            setTokenValue(tok2, value1);
        }

        if (Token::Match(tok2, "%oror%|&&")) {
            const ProgramMemory programMemory(getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)));
            if ((tok2->str() == "&&" && conditionIsFalse(tok2->astOperand1(), programMemory)) ||
                (tok2->str() == "||" && conditionIsTrue(tok2->astOperand1(), programMemory))) {
                // Skip second expression..
                const Token *parent = tok2;
                while (parent && parent->str() == tok2->str())
                    parent = parent->astParent();
                // Jump to end of condition
                if (parent && parent->str() == "(") {
                    tok2 = parent->link();
                    // cast
                    if (Token::simpleMatch(tok2, ") ("))
                        tok2 = tok2->linkAt(1);
                }
            }

        }
        if ((tok2->str() == "&&" && conditionIsFalse(tok2->astOperand1(), getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)))) ||
            (tok2->str() == "||" && conditionIsTrue(tok2->astOperand1(), getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)))))
            break;

        else if (Token::simpleMatch(tok2, ") {") && Token::findmatch(tok2->link(), "%varid%", tok2, varid)) {
            if (Token::findmatch(tok2, "continue|break|return", tok2->linkAt(1), varid)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "For loop variable bailout on conditional continue|break|return");
                break;
            }
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "For loop variable skipping conditional scope");
            tok2 = tok2->next()->link();
            if (Token::simpleMatch(tok2, "} else {")) {
                if (Token::findmatch(tok2, "continue|break|return", tok2->linkAt(2), varid)) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "For loop variable bailout on conditional continue|break|return");
                    break;
                }

                tok2 = tok2->linkAt(2);
            }
        }

        else if (Token::simpleMatch(tok2, ") {")) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "For loop skipping {} code");
            tok2 = tok2->linkAt(1);
            if (Token::simpleMatch(tok2, "} else {"))
                tok2 = tok2->linkAt(2);
        }
    }
}

static void valueFlowForLoopSimplifyAfter(Token *fortok, unsigned int varid, const MathLib::bigint num, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    const Token *vartok = nullptr;
    for (const Token *tok = fortok; tok; tok = tok->next()) {
        if (tok->varId() == varid) {
            vartok = tok;
            break;
        }
    }
    if (!vartok || !vartok->variable())
        return;

    const Variable *var = vartok->variable();
    const Token *endToken = nullptr;
    if (var->isLocal())
        endToken = var->typeStartToken()->scope()->classEnd;
    else
        endToken = fortok->scope()->classEnd;

    std::list<ValueFlow::Value> values;
    values.push_back(ValueFlow::Value(num));

    valueFlowForward(fortok->linkAt(1)->linkAt(1)->next(),
                     endToken,
                     var,
                     varid,
                     values,
                     false,
                     tokenlist,
                     errorLogger,
                     settings);
}

static void valueFlowForLoop(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (std::list<Scope>::const_iterator scope = symboldatabase->scopeList.begin(); scope != symboldatabase->scopeList.end(); ++scope) {
        if (scope->type != Scope::eFor)
            continue;

        Token* tok = const_cast<Token*>(scope->classDef);
        Token* const bodyStart = const_cast<Token*>(scope->classStart);

        if (!Token::simpleMatch(tok->next()->astOperand2(), ";") ||
            !Token::simpleMatch(tok->next()->astOperand2()->astOperand2(), ";"))
            continue;

        unsigned int varid(0);
        MathLib::bigint num1(0), num2(0), numAfter(0);

        if (valueFlowForLoop1(tok, &varid, &num1, &num2, &numAfter)) {
            if (num1 <= num2) {
                valueFlowForLoopSimplify(bodyStart, varid, num1, tokenlist, errorLogger, settings);
                valueFlowForLoopSimplify(bodyStart, varid, num2, tokenlist, errorLogger, settings);
                valueFlowForLoopSimplifyAfter(tok, varid, numAfter, tokenlist, errorLogger, settings);
            } else
                valueFlowForLoopSimplifyAfter(tok, varid, num1, tokenlist, errorLogger, settings);
        } else {
            ProgramMemory mem1, mem2, memAfter;
            if (valueFlowForLoop2(tok, &mem1, &mem2, &memAfter)) {
                std::map<unsigned int, MathLib::bigint>::const_iterator it;
                for (it = mem1.intvalues.begin(); it != mem1.intvalues.end(); ++it)
                    valueFlowForLoopSimplify(bodyStart, it->first, it->second, tokenlist, errorLogger, settings);
                for (it = mem2.intvalues.begin(); it != mem2.intvalues.end(); ++it)
                    valueFlowForLoopSimplify(bodyStart, it->first, it->second, tokenlist, errorLogger, settings);
                for (it = memAfter.intvalues.begin(); it != memAfter.intvalues.end(); ++it)
                    valueFlowForLoopSimplifyAfter(tok, it->first, it->second, tokenlist, errorLogger, settings);
            }
        }
    }
}

static void valueFlowInjectParameter(TokenList* tokenlist, ErrorLogger* errorLogger, const Settings* settings, const Variable* arg, const Scope* functionScope, const std::list<ValueFlow::Value>& argvalues)
{
    // Is argument passed by value or const reference, and is it a known non-class type?
    if (arg->isReference() && !arg->isConst() && !arg->isClass())
        return;

    // Set value in function scope..
    const unsigned int varid2 = arg->declarationId();
    if (!varid2)
        return;

    valueFlowForward(const_cast<Token*>(functionScope->classStart->next()), functionScope->classEnd, arg, varid2, argvalues, true, tokenlist, errorLogger, settings);
}

static void valueFlowSwitchVariable(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (std::list<Scope>::iterator scope = symboldatabase->scopeList.begin(); scope != symboldatabase->scopeList.end(); ++scope) {
        if (scope->type != Scope::ScopeType::eSwitch)
            continue;
        if (!Token::Match(scope->classDef, "switch ( %var% ) {"))
            continue;
        const Token *vartok = scope->classDef->tokAt(2);
        const Variable *var = vartok->variable();
        if (!var)
            continue;

        // bailout: global non-const variables
        if (!(var->isLocal() || var->isArgument()) && !var->isConst()) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, vartok, "switch variable " + var->name() + " is global");
            continue;
        }

        for (Token *tok = scope->classStart->next(); tok != scope->classEnd; tok = tok->next()) {
            if (tok->str() == "{") {
                tok = tok->link();
                continue;
            }
            if (Token::Match(tok, "case %num% :")) {
                std::list<ValueFlow::Value> values;
                values.push_back(ValueFlow::Value(MathLib::toLongNumber(tok->next()->str())));
                values.back().condition = tok;
                while (Token::Match(tok->tokAt(3), ";| case %num% :")) {
                    tok = tok->tokAt(3);
                    if (!tok->isName())
                        tok = tok->next();
                    values.push_back(ValueFlow::Value(MathLib::toLongNumber(tok->next()->str())));
                    values.back().condition = tok;
                }
                for (std::list<ValueFlow::Value>::const_iterator val = values.begin(); val != values.end(); ++val) {
                    valueFlowReverse(tokenlist,
                                     const_cast<Token*>(scope->classDef),
                                     vartok,
                                     *val,
                                     ValueFlow::Value(),
                                     errorLogger,
                                     settings);
                }
                if (vartok->variable()->scope()) // #7257
                    valueFlowForward(tok, vartok->variable()->scope()->classEnd, vartok->variable(), vartok->varId(), values, false, tokenlist, errorLogger, settings);
            }
        }
    }
}

static void valueFlowSubFunction(TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "%name% ("))
            continue;

        const Function * const currentFunction = tok->function();
        if (!currentFunction)
            continue;

        // Function scope..
        const Scope * const functionScope = currentFunction->functionScope;
        if (!functionScope)
            continue;

        unsigned int argnr = 0U;
        for (const Token *argtok = tok->tokAt(2); argtok; argtok = argtok->nextArgument()) {
            // Get function argument
            const Variable * const arg = currentFunction->getArgumentVar(argnr++);
            if (!arg)
                break;

            std::list<ValueFlow::Value> argvalues;

            // passing value(s) to function
            if (!argtok->values.empty() && Token::Match(argtok, "%name%|%num%|%str% [,)]"))
                argvalues = argtok->values;
            else {
                // bool operator => values 1/0 are passed to function..
                const Token *op = argtok;
                while (op && op->astParent() && !Token::Match(op->astParent(), "[(,]"))
                    op = op->astParent();
                if (Token::Match(op, "%comp%|%oror%|&&|!")) {
                    argvalues.clear();
                    argvalues.push_back(ValueFlow::Value(0));
                    argvalues.push_back(ValueFlow::Value(1));
                } else if (Token::Match(op, "%cop%") && !op->values.empty()) {
                    argvalues = op->values;
                } else {
                    // possible values are unknown..
                    continue;
                }
            }

            // passed values are not "known"..
            for (std::list<ValueFlow::Value>::iterator it = argvalues.begin(); it != argvalues.end(); ++it) {
                it->changeKnownToPossible();
            }

            valueFlowInjectParameter(tokenlist, errorLogger, settings, arg, functionScope, argvalues);
        }
    }
}

static void valueFlowFunctionDefaultParameter(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    if (!tokenlist->isCPP())
        return;

    const std::size_t functions = symboldatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope* scope = symboldatabase->functionScopes[i];
        const Function* function = scope->function;
        if (!function)
            continue;
        for (std::size_t arg = function->minArgCount(); arg < function->argCount(); arg++) {
            const Variable* var = function->getArgumentVar(arg);
            if (var && var->hasDefault() && Token::Match(var->nameToken(), "%var% = %num%|%str% [,)]")) {
                const std::list<ValueFlow::Value> &values = var->nameToken()->tokAt(2)->values;
                std::list<ValueFlow::Value> argvalues;
                for (std::list<ValueFlow::Value>::const_iterator it = values.begin(); it != values.end(); ++it) {
                    ValueFlow::Value v(*it);
                    v.defaultArg = true;
                    v.changeKnownToPossible();
                    if (v.isPossible())
                        argvalues.push_back(v);
                }
                if (!argvalues.empty())
                    valueFlowInjectParameter(tokenlist, errorLogger, settings, var, scope, argvalues);
            }
        }
    }
}

static bool constval(const Token * tok)
{
    return tok && tok->values.size() == 1U && tok->values.front().varId == 0U;
}

static void valueFlowFunctionReturn(TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->str() != "(" || !tok->astOperand1() || !tok->astOperand1()->function())
            continue;

        // Arguments..
        std::vector<MathLib::bigint> parvalues;
        {
            const Token *partok = tok->astOperand2();
            while (partok && partok->str() == "," && constval(partok->astOperand2()))
                partok = partok->astOperand1();
            if (!constval(partok))
                continue;
            parvalues.push_back(partok->values.front().intvalue);
            partok = partok->astParent();
            while (partok && partok->str() == ",") {
                parvalues.push_back(partok->astOperand2()->values.front().intvalue);
                partok = partok->astParent();
            }
            if (partok != tok)
                continue;
        }

        // Get scope and args of function
        const Function * const function = tok->astOperand1()->function();
        const Scope * const functionScope = function->functionScope;
        if (!functionScope || !Token::simpleMatch(functionScope->classStart, "{ return")) {
            if (functionScope && settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok, "function return; nontrivial function body");
            continue;
        }

        ProgramMemory programMemory;
        for (std::size_t i = 0; i < parvalues.size(); ++i) {
            const Variable * const arg = function->getArgumentVar(i);
            if (!arg || !Token::Match(arg->typeStartToken(), "%type% %name% ,|)")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok, "function return; unhandled argument type");
                programMemory.clear();
                break;
            }
            programMemory.setIntValue(arg->declarationId(), parvalues[i]);
        }
        if (programMemory.empty())
            continue;

        // Determine return value of subfunction..
        MathLib::bigint result = 0;
        bool error = false;
        execute(functionScope->classStart->next()->astOperand1(),
                &programMemory,
                &result,
                &error);
        if (!error)
            setTokenValue(tok, ValueFlow::Value(result));
    }
}

void ValueFlow::setValues(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next())
        tok->values.clear();

    valueFlowNumber(tokenlist);
    valueFlowString(tokenlist);
    valueFlowArray(tokenlist);
    valueFlowPointerAlias(tokenlist);
    valueFlowFunctionReturn(tokenlist, errorLogger, settings);
    valueFlowBitAnd(tokenlist);
    valueFlowOppositeCondition(symboldatabase, settings);
    valueFlowForLoop(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowBeforeCondition(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowAfterAssign(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowAfterCondition(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowSwitchVariable(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowSubFunction(tokenlist, errorLogger, settings);
    valueFlowFunctionDefaultParameter(tokenlist, symboldatabase, errorLogger, settings);
}


std::string ValueFlow::eitherTheConditionIsRedundant(const Token *condition)
{
    if (!condition)
        return "Either the condition is redundant";
    if (condition->str() == "case") {
        std::string expr;
        for (const Token *tok = condition; tok && tok->str() != ":"; tok = tok->next()) {
            expr += tok->str();
            if (Token::Match(tok, "%name%|%num% %name%|%num%"))
                expr += ' ';
        }
        return "Either the switch case '" + expr + "' is redundant";
    }
    return "Either the condition '" + condition->expressionString() + "' is redundant";
}
