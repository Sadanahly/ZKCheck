/*

 */

#include <string>
#include <sstream>
#include "config.h"

#ifndef mathlibH
#define mathlibH

/** @brief simple math functions that uses operands stored in std::string. useful when performing math on tokens. */
class ZKCHECKLIB MathLib {
public:
    /** @brief value class */
    class value {
    private:
        long long intValue;
        double doubleValue;
        enum { INT, LONG, LONGLONG, FLOAT } type;
        bool isUnsigned;

        void promote(const value &v);

    public:
        explicit value(const std::string &s);
        std::string str() const;
        bool isInt() const {
            return type != FLOAT;
        }
        bool isFloat() const {
            return type == FLOAT;
        }

        double getDoubleValue() const {
            return isFloat() ? doubleValue : (double)intValue;
        }

        static value calc(char op, const value &v1, const value &v2);
        int compare(const value &v) const;
        value add(int v) const;
        value shiftLeft(const value &v) const;
        value shiftRight(const value &v) const;
    };

    typedef long long bigint;
    typedef unsigned long long biguint;

    static bigint toLongNumber(const std::string & str);
    static biguint toULongNumber(const std::string & str);

    template<class T> static std::string toString(T value) {
        std::ostringstream result;
        result << value;
        return result.str();
    }
    static double toDoubleNumber(const std::string & str);

    static bool isInt(const std::string & str);
    static bool isFloat(const std::string &str);
    static bool isDecimalFloat(const std::string &str);
    static bool isNegative(const std::string &str);
    static bool isPositive(const std::string &str);
    static bool isDec(const std::string & str);
    static bool isFloatHex(const std::string& str);
    static bool isIntHex(const std::string& str);
    static bool isOct(const std::string& str);
    static bool isBin(const std::string& str);

    static bool isValidIntegerSuffix(std::string::const_iterator it, std::string::const_iterator end);

    static std::string add(const std::string & first, const std::string & second);
    static std::string subtract(const std::string & first, const std::string & second);
    static std::string multiply(const std::string & first, const std::string & second);
    static std::string divide(const std::string & first, const std::string & second);
    static std::string mod(const std::string & first, const std::string & second);
    static std::string incdec(const std::string & var, const std::string & op);
    static std::string calculate(const std::string & first, const std::string & second, char action);

    static std::string sin(const std::string & tok);
    static std::string cos(const std::string & tok);
    static std::string tan(const std::string & tok);
    static std::string abs(const std::string & tok);
    static bool isEqual(const std::string & first, const std::string & second);
    static bool isNotEqual(const std::string & first, const std::string & second);
    static bool isGreater(const std::string & first, const std::string & second);
    static bool isGreaterEqual(const std::string & first, const std::string & second);
    static bool isLess(const std::string & first, const std::string & second);
    static bool isLessEqual(const std::string & first, const std::string & second);
    static bool isNullValue(const std::string &tok);
    /**
     * Return true if given character is 0,1,2,3,4,5,6 or 7.
     * @param c The character to check
     * @return true if given character is octal digit.
     */
    static bool isOctalDigit(char c);
    /*
     * \param str character literal
     * @return Number of internal representation of the character literal
     * */
    static MathLib::bigint characterLiteralToLongNumber(const std::string& str);

};

MathLib::value operator+(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator-(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator*(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator/(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator%(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator&(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator|(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator^(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator<<(const MathLib::value &v1, const MathLib::value &v2);
MathLib::value operator>>(const MathLib::value &v1, const MathLib::value &v2);

template<> ZKCHECKLIB std::string MathLib::toString(double value); // Declare specialization to avoid linker problems

#endif
