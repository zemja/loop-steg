#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <sstream>
#include <algorithm>
#include <exception>

#include <cstring>
#include <cerrno>
#include <sys/random.h>

#include "CachedFile.h"
#include "exc.h"
#include "util.h"

using namespace std;

CachedFile::CachedFile(const string& path):
    _capacity(0),
    _path(path),
    _bytes(nullptr),
    _synced(true)
{
    // Only reason we open the file is to get its size with `.tellg()` in a moment.
    ifstream file(_path, ifstream::ate | ifstream::binary);

    if (!file)
    {
        stringstream ss;
        ss << "could not get size of '" << _path << "': " << strerror(errno);
        THROW(file, ss.str());
    }

    _capacity = file.tellg();

    if (!file)
    {
        stringstream ss;
        ss << "could not get size of '" << _path << "': " << strerror(errno);
        THROW(file, ss.str());
    }
}

CachedFile::CachedFile(): _capacity(), _path(), _bytes(nullptr), _synced(true) { }

CachedFile::~CachedFile()
{
    // Overwrite the buffer with randomness, just in case there was some important super secret
    // stuff in there. Set it to zero first in case `getrandom()` fails, since we can't signal an
    // error in a destructor. I'm OK with this since setting it to 0 is probably OK on its own.
    // Randomness is just a bonus and will basically never fail in practice anyway.
    if (_bytes)
    {
        memset(_bytes, 0, _capacity);
        getrandom(_bytes, _capacity, 0);
        delete [] _bytes;
    }
}

const string& CachedFile::path() const { return _path;     }
size_t CachedFile::capacity()    const { return _capacity; }

size_t CachedFile::write(const void* buf, size_t size, off_t offset)
{
    if (offset < 0)                  THROW(arg, "`offset` must be positive");
    if ((size_t)offset >= _capacity) THROW(arg, "`offset` must be < `.capacity()`");

    if (_bytes == nullptr) prepare();
    size = min(size, _capacity - offset);
    memcpy(_bytes + offset, buf, size);
    _synced = false;
    return size;
}

size_t CachedFile::read(char* buf, size_t size, off_t offset)
{
    if (offset < 0)                  THROW(arg, "`offset` must be positive");
    if ((size_t)offset >= _capacity) THROW(arg, "`offset` must be < `.capacity()`");

    if (_bytes == nullptr) prepare();
    size = min(size, _capacity - offset);
    memcpy(buf, _bytes + offset, size);
    return size;
}

void CachedFile::sync()
{
    // Check if we're already synced.
    if (synced()) return;

    ofstream file(_path);

    if (!file)
    {
        stringstream ss;
        ss << "could not open '" << _path << "' for writing: " << strerror(errno);
        THROW(file, ss.str());
    }

    file.write(_bytes, _capacity);

    if (!file)
    {
        stringstream ss;
        ss << "could not write to '" << _path << "': " << strerror(errno);
        THROW(file, ss.str());
    }

    _synced = true;

    delete [] _bytes;
    _bytes = nullptr;
}

bool CachedFile::synced() { return _synced; }

void CachedFile::prepare()
{
    if (!_bytes)
    {
        try { _bytes = new char[_capacity]; }

        catch (const bad_alloc& e)
        {
            stringstream ss;
            ss << "could not allocate memory to cache '" << _path << "': " << e.what();
            THROW(too_big, ss.str());
        }
    }

    // If the file has changed on the disk since `_capacity` was created, very bad things will
    // happen. (Probably.)
    ifstream file(_path, ifstream::ate | ifstream::binary);
    size_t capacity = file.tellg();

    if (!file)
    {
        stringstream ss;
        ss << "could not open '" << _path << "' for reading: " << strerror(errno);
        THROW(file, ss.str());
    }

    if (capacity != _capacity)
    {

        stringstream ss;
        ss << "file '" << _path << "' has changed";
        THROW(file, ss.str());
    }

    file.seekg(0);
    file.read(_bytes, _capacity);

    if (!file)
    {
        stringstream ss;
        ss << "could read from '" << _path << "': " << strerror(errno);
        THROW(file, ss.str());
    }

    _synced = true;
}
