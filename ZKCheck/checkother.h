/*

 */

#include "config.h"
#include "check.h"

#ifndef checkotherH
#define checkotherH

class Function;
class Variable;

/** @brief Various small checks */

class ZKCHECKLIB CheckOther : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckOther() : Check(myName()) {
    }

    /** @brief This constructor is used when running checks. */
    CheckOther(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
        CheckOther checkOther(tokenizer, settings, errorLogger);

        // Checks
        checkOther.warningOldStylePointerCast();
        checkOther.invalidPointerCast();
        checkOther.checkCharVariable();
        checkOther.checkRedundantAssignment();
        checkOther.checkRedundantAssignmentInSwitch();
        checkOther.checkSuspiciousCaseInSwitch();
        checkOther.checkDuplicateBranch();
        checkOther.checkDuplicateExpression();
        checkOther.checkUnreachableCode();
        checkOther.checkSuspiciousSemicolon();
        checkOther.checkVariableScope();
        checkOther.checkSignOfUnsignedVariable();  // don't ignore casts (#3574)
        checkOther.checkIncompleteArrayFill();
        checkOther.checkVarFuncNullUB();
        checkOther.checkNanInArithmeticExpression();
        checkOther.checkCommaSeparatedReturn();
        checkOther.checkRedundantPointerOp();
        checkOther.checkZeroDivision();
        checkOther.checkNegativeBitwiseShift();
        checkOther.checkInterlockedDecrement();
        checkOther.checkUnusedLabel();
        checkOther.checkEvaluationOrder();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
        CheckOther checkOther(tokenizer, settings, errorLogger);

        // Checks
        checkOther.clarifyCalculation();
        checkOther.clarifyStatement();
        checkOther.checkConstantFunctionParameter();
        checkOther.checkIncompleteStatement();
        checkOther.checkCastIntToCharAndBack();

        checkOther.checkMisusedScopedObject();
        checkOther.checkMemsetZeroBytes();
        checkOther.checkMemsetInvalid2ndParam();
        checkOther.checkSwitchCaseFallThrough();
        checkOther.checkPipeParameterSize();

        checkOther.checkInvalidFree();
        checkOther.checkRedundantCopy();
        checkOther.checkSuspiciousEqualityComparison();
        checkOther.checkComparisonFunctionIsAlwaysTrueOrFalse();
    }

    /** @brief Clarify calculation for ".. a * b ? .." */
    void clarifyCalculation();

    /** @brief Suspicious statement like '*A++;' */
    void clarifyStatement();

    /** @brief Are there C-style pointer casts in a c++ file? */
    void warningOldStylePointerCast();

    /** @brief Check for pointer casts to a type with an incompatible binary data representation */
    void invalidPointerCast();

    /** @brief %Check scope of variables */
    void checkVariableScope();
    static bool checkInnerScope(const Token *tok, const Variable* var, bool& used);

    /** @brief %Check for comma separated statements in return */
    void checkCommaSeparatedReturn();

    /** @brief %Check for constant function parameter */
    void checkConstantFunctionParameter();

    /** @brief Using char variable as array index / as operand in bit operation */
    void checkCharVariable();

    /** @brief Incomplete statement. A statement that only contains a constant or variable */
    void checkIncompleteStatement();

    /** @brief %Check zero division*/
    void checkZeroDivision();

    /** @brief Check for NaN (not-a-number) in an arithmetic expression */
    void checkNanInArithmeticExpression();

    /** @brief copying to memory or assigning to a variable twice */
    void checkRedundantAssignment();

    /** @brief %Check for assigning to the same variable twice in a switch statement*/
    void checkRedundantAssignmentInSwitch();

    /** @brief %Check for code like 'case A||B:'*/
    void checkSuspiciousCaseInSwitch();

    /** @brief %Check for code like 'case A||B:'*/
    void checkSuspiciousEqualityComparison();

    /** @brief %Check for switch case fall through without comment */
    void checkSwitchCaseFallThrough();

    /** @brief %Check for objects that are destroyed immediately */
    void checkMisusedScopedObject();

    /** @brief %Check for filling zero bytes with memset() */
    void checkMemsetZeroBytes();

    /** @brief %Check for invalid 2nd parameter of memset() */
    void checkMemsetInvalid2ndParam();

    /** @brief %Check for suspicious code where if and else branch are the same (e.g "if (a) b = true; else b = true;") */
    void checkDuplicateBranch();

    /** @brief %Check for suspicious code with the same expression on both sides of operator (e.g "if (a && a)") */
    void checkDuplicateExpression();

    /** @brief %Check for code that gets never executed, such as duplicate break statements */
    void checkUnreachableCode();

    /** @brief %Check for testing sign of unsigned variable */
    void checkSignOfUnsignedVariable();

    /** @brief %Check for suspicious use of semicolon */
    void checkSuspiciousSemicolon();

    /** @brief %Check for free() operations on invalid memory locations */
    void checkInvalidFree();
    void invalidFreeError(const Token *tok, bool inconclusive);

    /** @brief %Check for code creating redundant copies */
    void checkRedundantCopy();

    /** @brief %Check for bitwise shift with negative right operand */
    void checkNegativeBitwiseShift();

    /** @brief %Check for buffers that are filled incompletely with memset and similar functions */
    void checkIncompleteArrayFill();

    /** @brief %Check that variadic function calls don't use NULL. If NULL is \#defined as 0 and the function expects a pointer, the behaviour is undefined. */
    void checkVarFuncNullUB();

    /** @brief %Check that calling the POSIX pipe() system call is called with an integer array of size two. */
    void checkPipeParameterSize();

    /** @brief %Check to avoid casting a return value to unsigned char and then back to integer type.  */
    void checkCastIntToCharAndBack();

    /** @brief %Check for using of comparison functions evaluating always to true or false. */
    void checkComparisonFunctionIsAlwaysTrueOrFalse();

    /** @brief %Check for redundant pointer operations */
    void checkRedundantPointerOp();

    /** @brief %Check for race condition with non-interlocked access after InterlockedDecrement() */
    void checkInterlockedDecrement();

    /** @brief %Check for unused labels */
    void checkUnusedLabel();

    /** @brief %Check for expression that depends on order of evaluation of side effects */
    void checkEvaluationOrder();

private:
    // Error messages..
    void checkComparisonFunctionIsAlwaysTrueOrFalseError(const Token* tok, const std::string &strFunctionName, const std::string &varName, const bool result);
    void checkCastIntToCharAndBackError(const Token *tok, const std::string &strFunctionName);
    void checkPipeParameterSizeError(const Token *tok, const std::string &strVarName, const std::string &strDim);
    void clarifyCalculationError(const Token *tok, const std::string &op);
    void clarifyStatementError(const Token* tok);
    void cstyleCastError(const Token *tok);
    void invalidPointerCastError(const Token* tok, const std::string& from, const std::string& to, bool inconclusive);
    void passedByValueError(const Token *tok, const std::string &parname);
    void constStatementError(const Token *tok, const std::string &type);
    void signedCharArrayIndexError(const Token *tok);
    void unknownSignCharArrayIndexError(const Token *tok);
    void charBitOpError(const Token *tok);
    void variableScopeError(const Token *tok, const std::string &varname);
    void zerodivError(const Token *tok, bool inconclusive);
    void zerodivcondError(const Token *tokcond, const Token *tokdiv, bool inconclusive);
    void nanInArithmeticExpressionError(const Token *tok);
    void redundantAssignmentError(const Token *tok1, const Token* tok2, const std::string& var, bool inconclusive);
    void redundantAssignmentInSwitchError(const Token *tok1, const Token *tok2, const std::string &var);
    void redundantCopyError(const Token *tok1, const Token* tok2, const std::string& var);
    void redundantCopyInSwitchError(const Token *tok1, const Token* tok2, const std::string &var);
    void redundantBitwiseOperationInSwitchError(const Token *tok, const std::string &varname);
    void switchCaseFallThrough(const Token *tok);
    void suspiciousCaseInSwitchError(const Token* tok, const std::string& operatorString);
    void suspiciousEqualityComparisonError(const Token* tok);
    void selfAssignmentError(const Token *tok, const std::string &varname);
    void misusedScopeObjectError(const Token *tok, const std::string &varname);
    void memsetZeroBytesError(const Token *tok);
    void memsetFloatError(const Token *tok, const std::string &var_value);
    void memsetValueOutOfRangeError(const Token *tok, const std::string &value);
    void duplicateBranchError(const Token *tok1, const Token *tok2);
    void duplicateExpressionError(const Token *tok1, const Token *tok2, const std::string &op);
    void duplicateExpressionTernaryError(const Token *tok);
    void duplicateBreakError(const Token *tok, bool inconclusive);
    void unreachableCodeError(const Token* tok, bool inconclusive);
    void unsignedLessThanZeroError(const Token *tok, const std::string &varname, bool inconclusive);
    void pointerLessThanZeroError(const Token *tok, bool inconclusive);
    void unsignedPositiveError(const Token *tok, const std::string &varname, bool inconclusive);
    void pointerPositiveError(const Token *tok, bool inconclusive);
    void SuspiciousSemicolonError(const Token *tok);
    void negativeBitwiseShiftError(const Token *tok, int op);
    void redundantCopyError(const Token *tok, const std::string &varname);
    void incompleteArrayFillError(const Token* tok, const std::string& buffer, const std::string& function, bool boolean);
    void varFuncNullUBError(const Token *tok);
    void commaSeparatedReturnError(const Token *tok);
    void redundantPointerOpError(const Token* tok, const std::string& varname, bool inconclusive);
    void raceAfterInterlockedDecrementError(const Token* tok);
    void unusedLabelError(const Token* tok, bool inSwitch);
    void unknownEvaluationOrder(const Token* tok);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckOther c(0, settings, errorLogger);

        // error
        c.zerodivError(0, false);
        c.zerodivcondError(0,0,false);
        c.misusedScopeObjectError(NULL, "varname");
        c.invalidPointerCastError(0, "float", "double", false);
        c.negativeBitwiseShiftError(0,1);
        c.checkPipeParameterSizeError(0, "varname", "dimension");
        c.raceAfterInterlockedDecrementError(0);

        //performance
        c.redundantCopyError(0, "varname");
        c.redundantCopyError(0, 0, "var");
        c.redundantAssignmentError(0, 0, "var", false);

        // style/warning
        c.checkComparisonFunctionIsAlwaysTrueOrFalseError(0,"isless","varName",false);
        c.checkCastIntToCharAndBackError(0,"func_name");
        c.cstyleCastError(0);
        c.passedByValueError(0, "parametername");
        c.constStatementError(0, "type");
        c.signedCharArrayIndexError(0);
        c.unknownSignCharArrayIndexError(0);
        c.charBitOpError(0);
        c.variableScopeError(0, "varname");
        c.redundantAssignmentInSwitchError(0, 0, "var");
        c.redundantCopyInSwitchError(0, 0, "var");
        c.switchCaseFallThrough(0);
        c.suspiciousCaseInSwitchError(0, "||");
        c.suspiciousEqualityComparisonError(0);
        c.selfAssignmentError(0, "varname");
        c.memsetZeroBytesError(0);
        c.memsetFloatError(0, "varname");
        c.memsetValueOutOfRangeError(0, "varname");
        c.clarifyCalculationError(0, "+");
        c.clarifyStatementError(0);
        c.duplicateBranchError(0, 0);
        c.duplicateExpressionError(0, 0, "&&");
        c.duplicateExpressionTernaryError(0);
        c.duplicateBreakError(0, false);
        c.unreachableCodeError(0, false);
        c.unsignedLessThanZeroError(0, "varname", false);
        c.unsignedPositiveError(0, "varname", false);
        c.pointerLessThanZeroError(0, false);
        c.pointerPositiveError(0, false);
        c.SuspiciousSemicolonError(0);
        c.incompleteArrayFillError(0, "buffer", "memset", false);
        c.varFuncNullUBError(0);
        c.nanInArithmeticExpressionError(0);
        c.commaSeparatedReturnError(0);
        c.redundantPointerOpError(0, "varname", false);
        c.unusedLabelError(0, true);
        c.unusedLabelError(0, false);
        c.unknownEvaluationOrder(0);
    }

    static std::string myName() {
        return "Other";
    }

    std::string classInfo() const {
        return "Other checks\n"

               // error
               "- division with zero\n"
               "- scoped object destroyed immediately after construction\n"
               "- assignment in an assert statement\n"
               "- free() or delete of an invalid memory location\n"
               "- bitwise operation with negative right operand\n"
               "- provide wrong dimensioned array to pipe() system command (--std=posix)\n"
               "- cast the return values of getc(),fgetc() and getchar() to character and compare it to EOF\n"
               "- race condition with non-interlocked access after InterlockedDecrement() call\n"
               "- expression 'x = x++;' depends on order of evaluation of side effects\n"

               // warning
               "- either division by zero or useless condition\n"
               "- memset() with a value out of range as the 2nd parameter\n"

               // performance
               "- redundant data copying for const variable\n"
               "- subsequent assignment or copying to a variable or buffer\n"

               // portability
               "- memset() with a float as the 2nd parameter\n"
               "- Passing NULL pointer to function with variable number of arguments leads to UB.\n"

               // style
               "- C-style pointer cast in C++ code\n"
               "- casting between incompatible pointer types\n"
               "- passing parameter by value\n"
               "- [Incomplete statement](IncompleteStatement)\n"
               "- [check how signed char variables are used](CharVar)\n"
               "- variable scope can be limited\n"
               "- unusual pointer arithmetic. For example: \"abc\" + 'd'\n"
               "- redundant assignment, increment, or bitwise operation in a switch statement\n"
               "- redundant strcpy in a switch statement\n"
               "- Suspicious case labels in switch()\n"
               "- assignment of a variable to itself\n"
               "- Comparison of values leading always to true or false\n"
               "- Clarify calculation with parentheses\n"
               "- suspicious comparison of '\\0' with a char* variable\n"
               "- duplicate break statement\n"
               "- unreachable code\n"
               "- testing if unsigned variable is negative/positive\n"
               "- Suspicious use of ; at the end of 'if/for/while' statement.\n"
               "- Array filled incompletely using memset/memcpy/memmove.\n"
               "- NaN (not a number) value used in arithmetic expression.\n"
               "- comma in return statement (the comma can easily be misread as a semicolon).\n"
               "- prefer erfc, expm1 or log1p to avoid loss of precision.\n"
               "- identical code in both branches of if/else or ternary operator.\n"
               "- redundant pointer operation on pointer like &*some_ptr.\n"
               "- find unused 'goto' labels.\n";
    }
};

#endif
