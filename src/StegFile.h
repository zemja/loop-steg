#ifndef STEGFILE_H
#define STEGFILE_H

#include <string>
#include <vector>

#include "CachedFile.h"

// This is where the magic happens. Introducing `StegFile`: where the steganography actually goes
// down. Like `CachedFile`, which it derives from, it caches file contents in memory upon calling
// `.read()` or `.write()`, and flushes them to the file system when calling `.sync()`. The
// difference here is that these contents are hidden within an image file using steganography. For
// now, it's just simple LSB steganography. This class caches the hidden bytes in memory, not the
// entire uncompressed image data, and only performs steganography when reading and writing, to save
// memory.
//
// NOTE: This class assumes that on your system, `unsigned char` comprises a single 8-bit byte. It
// almost certainly does, but still...
class StegFile : public CachedFile
{
    public:
    // `StegFile` constructor.
    //
    // path: The path to the image file to wrap.
    //
    // Throws `exc::file` if `path` does not end in '.png,' '.bmp,' or '.tga.' (Not case sensitive.)
    // Throws `exc::file` if the image at `path` is a BMP file with 4 channels. (Not supported for
    //        now; see the implementation for an explanation.)
    // Throws `exc::file` if the image at `path` could not be read.
    StegFile(const std::string& path);

    // Move constructors, so we can push `StegFile`s back to a vector without shenanigans.
    StegFile(StegFile&& other)      = default;
    StegFile& operator=(StegFile&&) = default;

    // Delete these just in case. We deleted them in `CachedFile`, but g++ complains if we don't do
    // it again, thanks to -Weffc++.
    StegFile(const StegFile& other)            = delete;
    StegFile& operator=(const StegFile& other) = delete;

    // Flush everything to the file system. See `CachedFile::sync()`, for more info. This method
    // does NOT call `CachedFile::sync()`, but it behaves similarly.
    // Throws `exc::file` if the image at `.path()` could not be read.
    // Throws `exc::file` if the image at `.path()` could not be written to.
    // Throws `exc::file` if the image at `.path()` has changed in the file system since the
    //        `StegFile` was created.
    void sync();

    private:
    // Loads the contents of the image from the file system and extracts the hidden data from it,
    // saving it in `_bytes`. See `CachedFile::prepare()`. This method does NOT call
    // `CachedFile::prepare()`, but it behaves similarly.
    // Throws `exc::too_big` if not memory allocation for the hidden data failed.
    // Throws `exc::file` if the image at `.path()` could not be read.
    // Throws `exc::file` if the image at `.path()` has changed in the file system since the
    //        `StegFile` was created.
    void prepare();

    // Image dimensions when reading the image, sued to determine whether image has been changed in
    // the file system by something other than ourselves. (And to know what size to write the image
    // at, since the image data is just stored as a 1-dimensional block.)
    int _x, _y, _n;

    // File extension of the input image, so we know what format to save it as.
    std::string _extension;
};

#endif
