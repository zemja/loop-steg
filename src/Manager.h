#ifndef MANAGER_H
#define MANAGER_H

#include <vector>
#include <string>

#include "Shuffler.h"
#include "StegFile.h"
#include "exc.h"

// This class is a specialisation of `CachedFile`. It's intended to act as a 'manager' for
// loop-steg, which behaves like a `CachedFile` in that it buffers file contents to memory and
// flushes them with `.sync()`, but actually has multiple `StegFile`s behind the scenes, and
// provides an interface as if they are one big file. It also handles the complicated business of
// reading/writing randomly across all the files, So You Don't Have To™.
class Manager : public CachedFile
{
    public:
    // `Manager` constructor. Constructs from a directory full of regular files.
    //
    // path: The path to a directory full of regular files to construct `StegFile`s out of. This
    //       directory is searched recursively. Anything other than regular files are ignored.
    // seed: String used as source of randomness when randomly scattering reads/writes. Same seed =
    //       same read/write locations.
    //
    // Throws `exc::file` if the directory at `path` contains no regular files.
    // Throws anything `fs::list_files()` throws.
    // Throws anything `StegFile::StegFile(const string&)` throws.
    Manager(const std::string& path, const std::string& seed);

    // See `CachedFile::write()`.
    // Throws anything `.which_file()` throws.
    size_t write(const char* buf, size_t size, off_t offset);

    // See `CachedFile::read()`.
    // Throws anything `.which_file()` throws.
    int read(char* buf, size_t size, off_t offset);

    // See `StegFile::sync()`. Calls `.sync()` on every `StegFile` managed by this `Manager`.
    void sync();

    // See `CachedFile::synced()`.
    //
    // Returns true if `.synced()` returned true for every `StegFile` managed by this `Manager`,
    //         false otherwise.
    bool synced();

    protected:
    // Don't want to accidentally use this.
    void prepare() { THROW(unimplemented, ""); }

    private:
    // The files we're managing.
    std::vector<StegFile> _files;

    // Cumulative capacity of each of the files in `_files`. Used by `.which_file()`. See the body
    // of `.which_file()` for an explanation.
    std::vector<size_t> _cum_cap;

    // Shuffler used to randomise read/write locations.
    Shuffler _shuffler;

    // Given a byte location, works out which file from `_files` that byte lies in, and its offset
    // within that file.
    //
    // byte:     The byte location to find.
    // out_file: This is overwritten with a pointer to the file, in `_files`, that the byte location
    //           given by `byte` lies in, or nullptr if an exception was thrown.
    //
    // Returns the offset, in bytes, that the byte location given by `byte` lies in within the file
    //         pointed to by `out_file`.
    //
    // Throws `exc::arg` if `byte` >= `.capacity()`.
    //
    // NOTE: To explain, say you've got three files of 100 bytes in size. What file would the 250th
    // byte lie in? The third one, of course! And it would be the 50th byte in that file.
    // `.which_file()` is designed to work this out.
    size_t which_file(size_t byte, StegFile*& out_file);
};

#endif
