#ifndef CACHEDFILE_H
#define CACHEDFILE_H

#include <string>
#include <vector>
#include <fstream>

#include <cstring>

// This is intended to be an abstract base class, but provides a simple default implementation
// for testing purposes. This class reperesents a file, the contents of which are to be held in a
// buffer and modified, until the time comes to `.sync()` the contents to the file system and free
// said buffer to save memory. This is how we implement write caching: we have lots of these and
// `.sync()` them as rarely as we can.
//
// The default implementation buffers the entire contents of a file in the file system, but only
// does so when the first read/write request is made, as a form of lazy initialisation. It writes
// the entire contents back to the original file when `.sync()` is called.
//
// NOTE: Neither `CachedFile`, nor the classes that derive from it, call `.sync()` in their
// destructors as you might expect. This is because `.sync()` might throw exceptions, in which case
// Very Bad Things™ will happen.
class CachedFile
{
    public:
    // Constructs CachedFile from a path to a file.
    //
    // path: The path to the file that this `CachedFile` wraps.
    //
    // Throws `exc::file` if the size of the file at `path` could not be determined.
    CachedFile(const std::string& path);

    // Move constructors, so we can push `CachedFile`s back to a vector without shenanigans.
    CachedFile(CachedFile&& other)      = default;
    CachedFile& operator=(CachedFile&&) = default;

    // Delete these, since there should only be one `CachedFile` per file on the disk, else horrible
    // things will happen when they all `.sync()`.
    CachedFile(const CachedFile& other)            = delete;
    CachedFile& operator=(const CachedFile& other) = delete;

    // Override this, please!
    virtual ~CachedFile();

    // How much can you store in this `CachedFile`. For this base implementation, it's just the size
    // of the file in the file system.
    size_t capacity() const;

    // The path of the file in the file system. Same as `path` given to `CachedFile(const string&).`
    const std::string& path() const;

    // Write to this `CachedFile`. Analagous to `pwrite()`.
    //
    // buf:    Buffer of bytes to write.
    // size:   Write up to this many bytes.
    // offset: Start writing at this position in the `CachedFile`.
    //
    // Returns the number of bytes actually written. If there is room to write `size` bytes, this
    // will be `size`. If there isn't, as many as possible will be written, and this amount is
    // returned. (Like `pwrite()`.)
    //
    // Throws `exc::arg` if `offset` was negative.
    // Throws `exc::arg` if `offset` was >= `.capacity()`.
    // Throws anything `.prepare()` throws.
    size_t write(const void* buf, size_t size, off_t offset);

    // Read from this `CachedFile`. Analagous to `pread()`.
    //
    // buf:    Buffer of bytes to write to.
    // size:   Write up to this many bytes.
    // offset: Start reading from this position in the `CachedFile`.
    //
    // Returns the number of bytes actually read. If there are at least `size` bytes left in the
    // `CachedFile` after `offset`, this will be `size`. If there isn't, as many as possible will be
    // read, and this amount is returned. (Like `pread()`.)
    //
    // Throws `exc::arg` if `offset` was negative.
    // Throws `exc::arg` if `offset` was >= `.capacity()`.
    // Throws anything `.prepare()` throws.
    //
    // NOTE: You better be sure `buf` is big enough to handle `size` bytes, else you'll get a buffer
    //       overrun.
    size_t read(char* buf, size_t size, off_t offset);

    // Flushes the contents of the `CachedFile` to the file system, freeing the cached contents from
    // memory in the process. If the `CachedFile` is already synced, does nothing.
    //
    // Throws `exc::file` if the file at `.path()` could not be written to.
    void sync();

    // Gets whether any `.write()`s have been performed since the `CachedFile` was last `.sync()`ed.
    // (If the file is synced, its contents are not buffered in memory.)
    //
    // Returns whether the file is synced with the one in the file system.
    bool synced();

    protected:
    // Empty constructor, so that base classes can derive from this without having to initialise it
    // with a file.
    CachedFile();

    // Size in bytes of our buffer. Basically, the size of the file at `.path()`.
    size_t _capacity;

    // Location in the file system of the file we're wrapping. Same as `path` given to
    // `CachedFile(const string&)`.
    std::string _path;

    // Loads the file from the file system and fills up `_bytes` with its contents. This is called
    // at the beginning of the `.read()` and `.write()` methods if the file is not already in
    // memory. Children of this class should probably override or delete this.
    //
    // Throws `exc::too_big` if the file at `.path()` is larger than the maximum amount of memory we
    //        can allocate.
    // Throws `exc::file` if the file at `.path()` could not be read from.
    // Throws `exc::file` if the file at `.path()` has a different capacity in the file system than
    //        reported by `.capacity()`. (i.e., the file has changed in the file system since this
    //        `CachedFile` was created.)
    virtual void prepare();

    // The buffer where the contents of the file we're wrapping are stored.
    char* _bytes;

    // Whether or not `.write()` has been called since the last call to `.sync()`.
    bool _synced;
};

#endif
