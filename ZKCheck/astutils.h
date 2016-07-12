/*

 */

#include <set>
#include <string>

#ifndef astutilsH
#define astutilsH

class Token;

/** Is expression a 'signed char' if no promotion is used */
bool astIsSignedChar(const Token *tok);
/** Is expression a 'char' if no promotion is used? */
bool astIsUnknownSignChar(const Token *tok);
/** Is expression of integral type? */
bool astIsIntegral(const Token *tok, bool unknown);
/** Is expression of floating point type? */
bool astIsFloat(const Token *tok, bool unknown);
/** Is expression of boolean type? */
bool astIsBool(const Token *tok);

/**
 * Get canonical type of expression. const/static/etc are not included and neither *&.
 * For example:
 * Expression type      Return
 * std::string          std::string
 * int *                int
 * static const int     int
 * std::vector<T>       std::vector
 */
std::string astCanonicalType(const Token *expr);

/** Is given syntax tree a variable comparison against value */
const Token * astIsVariableComparison(const Token *tok, const std::string &comp, const std::string &rhs, const Token **vartok=nullptr);

bool isSameExpression(bool cpp, bool macro, const Token *tok1, const Token *tok2, const std::set<std::string> &constFunctions);

/**
 * Are two conditions opposite
 * @param isNot  do you want to know if cond1 is !cond2 or if cond1 and cond2 are non-overlapping. true: cond1==!cond2  false: cond1==true => cond2==false
 * @param cpp    c++ file
 * @param cond1  condition1
 * @param cond2  condition2
 * @param constFunctions  constFunctions
 */
bool isOppositeCond(bool isNot, bool cpp, const Token * const cond1, const Token * const cond2, const std::set<std::string> &constFunctions);

bool isConstExpression(const Token *tok, const std::set<std::string> &constFunctions);

bool isWithoutSideEffects(bool cpp, const Token* tok);

/** Is scope a return scope (scope will unconditionally return) */
bool isReturnScope(const Token *endToken);

/** Is variable changed in block of code? */
bool isVariableChanged(const Token *start, const Token *end, const unsigned int varid);

/** Determines the number of arguments - if token is a function call or macro
 * @param start token which is supposed to be the function/macro name.
 * \return Number of arguments
 */
int numberOfArguments(const Token *start);

#endif
