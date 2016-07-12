/*

 */

#ifndef standardsH
#define standardsH

/**
 * @brief This is just a container for standards settings.
 * This struct contains all possible standards that cppcheck recognize.
 */
struct Standards {
    /** C code C89/C99/C11 standard */
    enum cstd_t { C89, C99, C11 } c;

    /** C++ code standard */
    enum cppstd_t { CPP03, CPP11 } cpp;

    /** Code is posix */
    bool posix;

    /** This constructor clear all the variables **/
    Standards() : c(C11), cpp(CPP11), posix(false) {}

    bool setC(const std::string& str) {
        if (str == "c89" || str == "C89") {
            c = C89;
            return true;
        }
        if (str == "c99" || str == "C99") {
            c = C99;
            return true;
        }
        if (str == "c11" || str == "C11") {
            c = C11;
            return true;
        }
        return false;
    }
    bool setCPP(const std::string& str) {
        if (str == "c++03" || str == "C++03") {
            cpp = CPP03;
            return true;
        }
        if (str == "c++11" || str == "C++11") {
            cpp = CPP11;
            return true;
        }
        return false;
    }
};

#endif
