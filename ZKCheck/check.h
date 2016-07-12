/*

 */

#include "config.h"
#include "token.h"
#include "tokenize.h"
#include "settings.h"
#include "errorlogger.h"

#include <list>
#include <set>

#ifndef checkH
#define checkH

/**
 * @brief Interface class that cppcheck uses to communicate with the checks.
 * All checking classes must inherit from this class
 */
class ZKCHECKLIB Check {
public:
    /** This constructor is used when registering the CheckClass */
    explicit Check(const std::string &aname);

    /** This constructor is used when running checks. */
    Check(const std::string &aname, const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : _tokenizer(tokenizer), _settings(settings), _errorLogger(errorLogger), _name(aname) {
    }

    virtual ~Check() {
        if (!_tokenizer)
            instances().remove(this);
    }

    /** List of registered check classes. This is used by Cppcheck to run checks and generate documentation */
    static std::list<Check *> &instances();


	//-------------------------------------------add test before simplify--------------------------------------//
	/** run checks, the token list is not simplified */
	virtual void runRawChecks(const Tokenizer *, const Settings *, ErrorLogger *) {
	}

    /** run checks, the token list is not simplified */
    virtual void runChecks(const Tokenizer *, const Settings *, ErrorLogger *) {
    }

    /** run checks, the token list is simplified */
    virtual void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) = 0;

    /** get error messages */
    virtual void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const = 0;

    /** class name, used to generate documentation */
    const std::string& name() const {
        return _name;
    }

    /** get information about this class, used to generate documentation */
    virtual std::string classInfo() const = 0;

    /**
     * Write given error to errorlogger or to out stream in xml format.
     * This is for for printout out the error list with --errorlist
     * @param errmsg Error message to write
     */
    static void reportError(const ErrorLogger::ErrorMessage &errmsg);

    bool inconclusiveFlag() const {
        return _settings && _settings->inconclusive;
    }

    /** Base class used for whole-program analysis */
    class FileInfo {
    public:
        FileInfo() {}
        virtual ~FileInfo() {}
    };

    virtual FileInfo * getFileInfo(const Tokenizer *tokenizer, const Settings *settings) const {
        (void)tokenizer;
        (void)settings;
        return nullptr;
    }

    virtual void analyseWholeProgram(const std::list<FileInfo*> &fileInfo, const Settings& settings, ErrorLogger &errorLogger) {
        (void)fileInfo;
        (void)settings;
        (void)errorLogger;
    }

protected:
    const Tokenizer * const _tokenizer;
    const Settings * const _settings;
    ErrorLogger * const _errorLogger;

    /** report an error */
    template<typename T, typename U>
    void reportError(const Token *tok, const Severity::SeverityType severity, const T id, const U msg) {
        reportError(tok, severity, id, msg, CWE(0U), false);
    }

    /** report an error */
    template<typename T, typename U>
    void reportError(const Token *tok, const Severity::SeverityType severity, const T id, const U msg, const CWE &cwe, bool inconclusive) {
        const std::list<const Token *> callstack(1, tok);
        reportError(callstack, severity, id, msg, cwe, inconclusive);
    }

    /** report an error */
    template<typename T, typename U>
    void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, const T id, const U msg) {
        reportError(callstack, severity, id, msg, CWE(0U), false);
    }

    /** report an error */
    template<typename T, typename U>
    void reportError(const std::list<const Token *> &callstack, Severity::SeverityType severity, const T id, const U msg, const CWE &cwe, bool inconclusive) {
        const ErrorLogger::ErrorMessage errmsg(callstack, _tokenizer?&_tokenizer->list:0, severity, id, msg, cwe, inconclusive);
        if (_errorLogger)
            _errorLogger->reportErr(errmsg);
        else
            reportError(errmsg);
    }

private:
    const std::string _name;

    /** disabled assignment operator and copy constructor */
    void operator=(const Check &);
    explicit Check(const Check &);
};

#endif
