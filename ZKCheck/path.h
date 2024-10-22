/*

 */

#include "config.h"
#include <set>
#include <string>
#include <vector>

#ifndef pathH
#define pathH

/**
 * @brief Path handling routines.
 * Internally cppcheck wants to store paths with / separator which is also
 * native separator for Unix-derived systems. When giving path to user
 * or for other functions we convert path separators back to native type.
 */
class ZKCHECKLIB Path {
public:
    /**
     * Convert path to use native separators.
     * @param path Path string to convert.
     * @return converted path.
     */
    static std::string toNativeSeparators(std::string path);

    /**
      * Convert path to use internal path separators.
      * @param path Path string to convert.
      * @return converted path.
      */
    static std::string fromNativeSeparators(std::string path);

    /**
     * @brief Simplify path "foo/bar/.." => "foo"
     * @param originalPath path to be simplified, must have / -separators.
     * @return simplified path
     */
    static std::string simplifyPath(std::string originalPath);

    /**
     * @brief Lookup the path part from a filename (e.g., '/tmp/a.h' -> '/tmp/', 'a.h' -> '')
     * @param filename filename to lookup, must have / -separators.
     * @return path part of the filename
     */
    static std::string getPathFromFilename(const std::string &filename);

    /**
     * @brief Compare filenames to see if they are the same.
     * On Linux the comparison is case-sensitive. On Windows it is case-insensitive.
     * @param fname1 one filename
     * @param fname2 other filename
     * @return true if the filenames match on the current platform
     */
    static bool sameFileName(const std::string &fname1, const std::string &fname2);

    /**
     * @brief Remove quotation marks (") from the path.
     * @param path path to be cleaned.
     * @return Cleaned path without quotation marks.
     */
    static std::string removeQuotationMarks(std::string path);

    /**
      * @brief Get an extension of the filename.
      * @param path Path containing filename.
      * @return Filename extension (containing the dot, e.g. ".h" or ".CPP").
      */
    static std::string getFilenameExtension(const std::string &path);

    /**
      * @brief Get an extension of the filename in lower case.
      * @param path Path containing filename.
      * @return Filename extension (containing the dot, e.g. ".h").
      */
    static std::string getFilenameExtensionInLowerCase(const std::string &path);

    /**
      * @brief Create a relative path from an absolute one, if absolute path is inside the basePaths.
      * @param absolutePath Path to be made relative.
      * @param basePaths Paths to which it may be made relative.
      * @return relative path, if possible. Otherwise absolutePath is returned unchanged
      */
    static std::string getRelativePath(const std::string& absolutePath, const std::vector<std::string>& basePaths);

    /**
      * @brief Get an absolute file path from a relative one.
      * @param filePath File path to be made absolute.
      * @return absolute path, if possible. Otherwise an empty path is returned
      */
    static std::string getAbsoluteFilePath(const std::string& filePath);

    /**
     * @brief Check if the file extension indicates that it's a C/C++ source file.
     * Check if the file has source file extension: *.c;*.cpp;*.cxx;*.c++;*.cc;*.txx
     * @param filename filename to check. path info is optional
     * @return true if the file extension indicates it should be checked
     */
    static bool acceptFile(const std::string &filename) {
        const std::set<std::string> extra;
        return acceptFile(filename, extra);
    }

    /**
     * @brief Check if the file extension indicates that it's a C/C++ source file.
     * Check if the file has source file extension: *.c;*.cpp;*.cxx;*.c++;*.cc;*.txx
     * @param filename filename to check. path info is optional
     * @param extra    extra file extensions
     * @return true if the file extension indicates it should be checked
     */
    static bool acceptFile(const std::string &filename, const std::set<std::string> &extra);

    /**
     * @brief Identify language based on file extension.
     * @param path filename to check. path info is optional
     * @return true if extension is meant for C files
     */
    static bool isC(const std::string &path);

    /**
     * @brief Identify language based on file extension.
     * @param extensionInLowerCase filename to check. path info is optional
     * @return true if extension is meant for C++ files
     */
    static bool isCPP(const std::string &extensionInLowerCase);

    /**
     * @brief Is filename a header based on file extension
     * @param path filename to check. path info is optional
     * @return true if filename extension is meant for headers
     */
    static bool isHeader(const std::string &path);
};

#endif
