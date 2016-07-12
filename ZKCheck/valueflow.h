/*

 */

#include <string>
#include "config.h"

class Token;
class TokenList;
class SymbolDatabase;
class ErrorLogger;
class Settings;

#ifndef valueflowH
#define valueflowH

namespace ValueFlow {
    class ZKCHECKLIB Value {
    public:
        explicit Value(long long val = 0) : intvalue(val), tokvalue(nullptr), varvalue(val), condition(0), varId(0U), conditional(false), inconclusive(false), defaultArg(false), valueKind(ValueKind::Possible) {}
        Value(const Token *c, long long val) : intvalue(val), tokvalue(nullptr), varvalue(val), condition(c), varId(0U), conditional(false), inconclusive(false), defaultArg(false), valueKind(ValueKind::Possible) {}

        /** int value */
        long long intvalue;

        /** token value - the token that has the value. this is used for pointer aliases, strings, etc. */
        const Token *tokvalue;

        /** For calculated values - variable value that calculated value depends on */
        long long varvalue;

        /** Condition that this value depends on (TODO: replace with a 'callstack') */
        const Token *condition;

        /** For calculated values - varId that calculated value depends on */
        unsigned int varId;

        /** Conditional value */
        bool conditional;

        /** Is this value inconclusive? */
        bool inconclusive;

        /** Is this value passed as default parameter to the function? */
        bool defaultArg;

        /** How known is this value */
        enum ValueKind {
            /** This value is possible, other unlisted values may also be possible */
            Possible,
            /** Only listed values are possible */
            Known,
            /** Max value. Greater values are impossible. */
            Max,
            /** Min value. Smaller values are impossible. */
            Min
        } valueKind;

        void setKnown() {
            valueKind = ValueKind::Known;
        }

        bool isKnown() const {
            return valueKind == ValueKind::Known;
        }

        void setPossible() {
            valueKind = ValueKind::Possible;
        }

        bool isPossible() const {
            return valueKind == ValueKind::Possible;
        }

        void changeKnownToPossible() {
            if (isKnown())
                valueKind = ValueKind::Possible;
        }
    };

    void setValues(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings);

    std::string eitherTheConditionIsRedundant(const Token *condition);
}

#endif
