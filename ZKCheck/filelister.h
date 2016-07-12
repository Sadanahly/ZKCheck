/*

 */

#include <string>
#include <set>
#include <map>

#ifndef filelisterH
#define filelisterH

class PathMatch;

/** @brief Cross-platform FileLister */
class FileLister {
public:
    /**
     * @brief Recursively add source files to a map.
     * Add source files from given directory and all subdirectries to the
     * given map. Only files with accepted extensions
     * (*.c;*.cpp;*.cxx;*.c++;*.cc;*.txx) are added.
     * @param files output map that associates the size of each file with its name
     * @param path root path
     * @param ignored ignored paths
     */
    static void recursiveAddFiles(std::map<std::string, std::size_t> &files, const std::string &path, const PathMatch& ignored) {
        const std::set<std::string> extra;
        recursiveAddFiles(files, path, extra, ignored);
    }

    /**
     * @brief Recursively add source files to a map.
     * Add source files from given directory and all subdirectries to the
     * given map. Only files with accepted extensions
     * (*.c;*.cpp;*.cxx;*.c++;*.cc;*.txx) are added.
     * @param files output map that associates the size of each file with its name
     * @param path root path
     * @param extra Extra file extensions
     * @param ignored ignored paths
     */
    static void recursiveAddFiles(std::map<std::string, std::size_t> &files, const std::string &path, const std::set<std::string> &extra, const PathMatch& ignored);

    /**
     * @brief (Recursively) add source files to a map.
     * Add source files from given directory and all subdirectries to the
     * given map. Only files with accepted extensions
     * (*.c;*.cpp;*.cxx;*.c++;*.cc;*.txx) are added.
     * @param files output map that associates the size of each file with its name
     * @param path root path
     * @param extra Extra file extensions
     * @param recursive Enable recursion
     * @param ignored ignored paths
     */
    static void addFiles(std::map<std::string, std::size_t> &files, const std::string &path, const std::set<std::string> &extra, bool recursive, const PathMatch& ignored);

    /**
     * @brief Is given path a directory?
     * @return returns true if the path is a directory
     */
    static bool isDirectory(const std::string &path);

    /**
      * @brief Check if the given path is a file and if it exists?
      * @return true if path points to file and the file exists.
      */
    static bool fileExists(const std::string &path);
};

#endif