/*

 */

#include "errorlogger.h"
#include <cstdio>
#include <ctime>
#include <set>
#include <map>
#include <string>

class ZKCheck;
class Settings;
class Library;

#ifndef ZKCHECKEXECUTOR_H
#define ZKCHECKEXECUTOR_H

/**

 */
class ZKCheckExecutor : public ErrorLogger {
public:
    /**
     * Constructor
     */
    ZKCheckExecutor();

    /**
     * Destructor
     */
    virtual ~ZKCheckExecutor();

    /**
     * Starts the checking.
     *
     * @param argc from main()
     * @param argv from main()
     * @return EXIT_FAILURE if arguments are invalid or no input files
     *         were found.
     *         If errors are found and --error-exitcode is used,
     *         given value is returned instead of default 0.
     *         If no errors are found, 0 is returned.
     */
    int check(int argc, const char* const argv[]);

    /**
     * Information about progress is directed here. This should be
     * called by the CppCheck class only.
     *
     * @param outmsg Progress message e.g. "Checking main.cpp..."
     */
    virtual void reportOut(const std::string &outmsg);

    /** xml output of errors */
    virtual void reportErr(const ErrorLogger::ErrorMessage &msg);

    void reportProgress(const std::string &filename, const char stage[], const std::size_t value);

    /**
     * Output information messages.
     */
    virtual void reportInfo(const ErrorLogger::ErrorMessage &msg);

    /**
     * Information about how many files have been checked
     *
     * @param fileindex This many files have been checked.
     * @param filecount This many files there are in total.
     * @param sizedone The sum of sizes of the files checked.
     * @param sizetotal The total sizes of the files.
     */
    static void reportStatus(std::size_t fileindex, std::size_t filecount, std::size_t sizedone, std::size_t sizetotal);

    /**
     * @param fp Output file
     */
    static void setExceptionOutput(FILE* fp);
    /**
    * @return file name to be used for output from exception handler. Has to be either "stdout" or "stderr".
    */
    static FILE* getExceptionOutput();

    /**
    * Tries to load a library and prints warning/error messages
    * @return false, if an error occured (except unknown XML elements)
    */
    static bool tryLoadLibrary(Library& destination, const char* basepath, const char* filename);

protected:

    /**
     * Helper function to print out errors. Appends a line change.
     * @param errmsg String printed to error stream
     */
    void reportErr(const std::string &errmsg);

    /**
     * @brief Parse command line args and get settings and file lists
     * from there.
     *
     * @param cppcheck cppcheck instance
     * @param argc argc from main()
     * @param argv argv from main()
     * @return false when errors are found in the input
     */
    bool parseFromArgs(ZKCheck *cppcheck, int argc, const char* const argv[]);

private:

    /**
     * Wrapper around check_internal
     *   - installs optional platform dependent signal handling
     *
     * * @param cppcheck cppcheck instance
    * @param argc from main()
    * @param argv from main()
     **/
    int check_wrapper(ZKCheck& cppcheck, int argc, const char* const argv[]);

    /**
    * Starts the checking.
    *
    * @param cppcheck cppcheck instance
    * @param argc from main()
    * @param argv from main()
    * @return EXIT_FAILURE if arguments are invalid or no input files
    *         were found.
    *         If errors are found and --error-exitcode is used,
    *         given value is returned instead of default 0.
    *         If no errors are found, 0 is returned.
    */
    int check_internal(ZKCheck& cppcheck, int argc, const char* const argv[]);

    /**
     * Pointer to current settings; set while check() is running.
     */
    const Settings* _settings;

    /**
     * Used to filter out duplicate error messages.
     */
    std::set<std::string> _errorList;

    /**
     * Filename associated with size of file
     */
    std::map<std::string, std::size_t> _files;

    /**
     * Report progress time
     */
    std::time_t time1;

    /**
     * Output file name for exception handler
     */
    static FILE* exceptionOutput;

    /**
     * Has --errorlist been given?
     */
    bool errorlist;
};

#endif
