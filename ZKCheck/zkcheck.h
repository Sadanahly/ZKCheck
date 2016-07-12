/*

 */

#include "config.h"
#include "settings.h"
#include "errorlogger.h"
#include "check.h"

#include <string>
#include <list>
#include <istream>

class Tokenizer;

#ifndef zkcheckH
#define zkcheckH

/**

 */
class ZKCHECKLIB ZKCheck : ErrorLogger {
public:
    /**
     * @brief Constructor.
     */
    ZKCheck(ErrorLogger &errorLogger, bool useGlobalSuppressions);

    /**
     * @brief Destructor.
     */
    virtual ~ZKCheck();

    /**
     * @brief This starts the actual checking. Note that you must call
     * parseFromArgs() or settings() and addFile() before calling this.
     * @return amount of errors found or 0 if none were found.
     */

    /**
      * @brief Check the file.
      * This function checks one given file for errors.
      * @param path Path to the file to check.
      * @return amount of errors found or 0 if none were found.
      * @note You must set settings before calling this function (by calling
      *  settings()).
      */
    unsigned int check(const std::string &path);

    /**
      * @brief Check the file.
      * This function checks one "virtual" file. The file is not read from
      * the disk but the content is given in @p content. In errors the @p path
      * is used as a filename.
      * @param path Path to the file to check.
      * @param content File content as a string.
      * @return amount of errors found or 0 if none were found.
      * @note You must set settings before calling this function (by calling
      *  settings()).
      */
    unsigned int check(const std::string &path, const std::string &content);

    /**
     * @brief Get reference to current settings.
     * @return a reference to current settings
     */
    Settings &settings();

    /**
     * @brief Returns current version number as a string.
     * @return version, e.g. "1.38"
     */
    static const char * version();

    /**
     * @brief Returns extra version info as a string.
     * This is for returning extra version info, like Git commit id, build
     * time/date etc.
     * @return extra version info, e.g. "04d42151" (Git commit id).
     */
    static const char * extraVersion();

    virtual void reportStatus(unsigned int fileindex, unsigned int filecount, std::size_t sizedone, std::size_t sizetotal);

    /**
     * @brief Terminate checking. The checking will be terminated as soon as possible.
     */
    void terminate() {
        _settings.terminate();
    }

    /**
     * @brief Call all "getErrorMessages" in all registered Check classes.
     * Also print out XML header and footer.
     */
    void getErrorMessages();

    void tooManyConfigsError(const std::string &file, const std::size_t numberOfConfigurations);
    void purgedConfigurationMessage(const std::string &file, const std::string& configuration);

    void dontSimplify() {
        _simplify = false;
    }

    /** analyse whole program, run this after all TUs has been scanned. */
    void analyseWholeProgram();

    /** Check if the user wants to check for unused functions
     * and if it's possible at all */
    bool unusedFunctionCheckIsEnabled() const;

private:

    /** @brief There has been an internal error => Report information message */
    void internalError(const std::string &filename, const std::string &msg);

    /**
     * @brief Process one file.
     * @param filename file name
     * @param fileStream stream the file content can be read from
     * @return amount of errors found
     */
    unsigned int processFile(const std::string& filename, std::istream& fileStream);

    /**
     * @brief Check raw tokens
     * @param tokenizer
     */
    void checkRawTokens(const Tokenizer &tokenizer);

    /**
     * @brief Check normal tokens
     * @param tokenizer
     */
    void checkNormalTokens(const Tokenizer &tokenizer);

    /**
     * @brief Check simplified tokens
     * @param tokenizer
     */
    void checkSimplifiedTokens(const Tokenizer &tokenizer);

    /**
     * @brief Execute rules, if any
     * @param tokenlist token list to use (normal / simple)
     * @param tokenizer tokenizer
     */
    void executeRules(const std::string &tokenlist, const Tokenizer &tokenizer);

    /**
     * @brief Errors and warnings are directed here.
     *
     * @param msg Errors messages are normally in format
     * "[filepath:line number] Message", e.g.
     * "[main.cpp:4] Uninitialized member variable"
     */
    virtual void reportErr(const ErrorLogger::ErrorMessage &msg);

    /**
     * @brief Information about progress is directed here.
     *
     * @param outmsg Message to show, e.g. "Checking main.cpp..."
     */
    virtual void reportOut(const std::string &outmsg);

    std::list<std::string> _errorList;
    Settings _settings;

    void reportProgress(const std::string &filename, const char stage[], const std::size_t value);

    /**
     * Output information messages.
     */
    virtual void reportInfo(const ErrorLogger::ErrorMessage &msg);

    ErrorLogger &_errorLogger;

    /** @brief Current preprocessor configuration */
    std::string cfg;

    unsigned int exitcode;

    bool _useGlobalSuppressions;

    /** Are there too many configs? */
    bool tooManyConfigs;

    /** Simplify code? true by default */
    bool _simplify;

    /** File info used for whole program analysis */
    std::list<Check::FileInfo*> fileInfo;
};

#endif
