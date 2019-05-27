#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <sstream>
#include <thread>
#include <future>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "StegFile.h"
#include "Manager.h"
#include "fs.h"
#include "exc.h"

using namespace std;

Manager::Manager(const string& path, const string& seed):
    _files(),
    _cum_cap(),
    _shuffler(0, 0, "")
{
    _path  = path;
    _bytes = nullptr; // Not used.

    // Find the paths of all the regular files under `path`, and create `StegFile`s out of them.
    {
        vector<string> paths = fs::list_files(path);

        if (paths.empty())
        {
            stringstream ss;
            ss << "directory at '" << path << "' contains no regular files";
            THROW(file, ss.str());
        }

        _files.reserve(paths.size());

        vector<future<StegFile>> futures;
        futures.reserve(paths.size());

        for (string& s : paths)
            futures.emplace_back(async([&s]() -> StegFile { return StegFile(s); }));

        for (auto& f : futures)
            _files.emplace_back(move(f.get()));
    }

    // Now work out the cumulative capacity of each file. The user doesn't need this; it's to help
    // with `.which_file()`, see that method's body for an explanation.
    _cum_cap.emplace_back(_files.front().capacity());

    for (auto it = _files.cbegin() + 1; it != _files.cend(); ++it)
        _cum_cap.emplace_back(_cum_cap.back() + it->capacity());

    _capacity = _cum_cap.back();

    // Initialise the shuffler.
    _shuffler = Shuffler(0, _capacity, seed);
}

size_t Manager::write(const char* buf, size_t size, off_t offset)
{
    if (offset < 0)                  THROW(arg, "`offset` must be positive");
    if ((size_t)offset >= _capacity) THROW(arg, "`offset` must be < `.capacity()`");

    size = min(size, _capacity - offset);

    StegFile* file = nullptr;

    // TODO 3 Support reading/writing in blocks. What happens if `offset` isn't a block boundary?

    for (size_t i = 0; i < size; ++i, ++offset)
    {
        size_t file_offs = which_file(_shuffler[offset], file);
        file->write(buf + i, 1, file_offs);
    }

    return size;
}

int Manager::read(char* buf, size_t size, off_t offset)
{
    if (offset < 0)                  THROW(arg, "`offset` must be positive");
    if ((size_t)offset >= _capacity) THROW(arg, "`offset` must be < `.capacity()`");

    size = min(size, _capacity - offset);

    StegFile* file = nullptr;

    for (size_t i = 0; i < size; ++i, ++offset)
    {
        size_t file_offs = which_file(_shuffler[offset], file);
        file->read(buf + i, 1, file_offs);
    }

    return size;
}

void Manager::sync()
{
    vector<future<void>> futures;
    futures.reserve(_files.size());

    for (StegFile& f : _files)
        futures.emplace_back(async(&StegFile::sync, &f));

    for (auto& f : futures)
        f.wait();
}

bool Manager::synced()
{
    for (StegFile& f : _files)
        if (!f.synced())
            return false;

    return true;
}

size_t Manager::which_file(size_t byte, StegFile*& out_file)
{
    // Find the first entry in `_cum_cap` (which contains the cumulative capacity of all the files
    // in `_files`) which is greater than `byte`.
    for (size_t i = 0; i < _cum_cap.size(); ++i)
    {
        // We found it! That means that `byte` lies within `_files[i]`.
        if (_cum_cap[i] > byte)
        {
            // The offset within this file is just `byte` - (the cumulative capacity of the file
            // before this one). If there was no file before this one, the offset is just `byte`.
            out_file = &_files[i];
            return byte - (i ? _cum_cap[i - 1] : 0);
        }
    }

    // If we get here, then no entry within `_cum_cap` was greater than `byte`. Therefore, `byte`
    // was greater than `_capacity`, (i.e., it's out of bounds), which is a bug. Tell myself off
    // with a stern exception message.
    out_file = nullptr;

    stringstream ss;
    ss << "`byte` (" << byte << ") must be < `.capacity()` (" << _capacity << ")";
    THROW(arg, ss.str());
}
