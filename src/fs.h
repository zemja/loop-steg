#ifndef FS_H
#define FS_H

// This file contains anything related to interactions with the filesystem. The idea is that
// everything in here can be given multiple implementations in <fs.cpp>, should loop-steg ever be
// made multi-platform. For the same reason, it can handle platform-specific stuff such as joining
// paths with the right path separator.

#include <vector>

#include <string>

namespace fs
{

// Finds the path of every regular file in a directory.
//
// dir_path:  The path to the directory to search in.
// recursive: If true, search recursively. Otherwise just search within the directory at `dir_path`.
//            Defaults to true.
//
// Returns the path of every file found under the directory at `dir_path`. Every entry will begin
//         with `dir_path`, so the resulting paths are absolute only if `dir_path` is absolute.
//
// Throws `exc::file` if `dir_path`, (or any directories beneath it, if `recursive` is true), could
//        not be opened. (For example, if they are not a directory.)
//
// NOTE: The result is sorted using `std::sort()`, so that the result will be the same every time.
std::vector<std::string> list_files(std::string dir_path, bool recursive = true);

// Reads the entire contents of a file into a string.
//
// path: The path to the file to load.
//
// Returns the contents of the file at `path`.
//
// Throws `exc::file` if the file at `path` could not be read.
std::string read_to_string(const std::string& path);

}

#endif
