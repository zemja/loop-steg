#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <sstream>
#include <fstream>
#include <memory>

#include <cstring>
#include <cerrno>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

#include "fs.h"
#include "exc.h"
#include "util.h"

using namespace std;

namespace fs
{

vector<string> list_files(string dir_path, bool recursive)
{
    // Because we're joining paths in a moment, we ensure that this first one ends in a '/', as the
    // rest of them will later.
    if (dir_path.back() != '/') dir_path += '/';

    vector<string> result;

    // The way this function works is, we have a queue of directories, which initially just contains
    // `dir_path`. Then, in a loop, we repeatedly take the next item from the queue, and iterate
    // through all the items in that directory. For those that are files, we add them to `result`.
    // For those that are directories, we push them to the queue and keep going. We repeat this
    // until the queue is empty.

    queue<string> dirs;
    dirs.push(dir_path);

    while (!dirs.empty())
    {
        unique_ptr<DIR, void(*)(DIR*)> dir
        (
            opendir(dirs.front().c_str()),
            [](DIR* dir){ closedir(dir); }
        );

        if (!dir)
        {
            stringstream ss;
            ss << "could not open '" << dir_path << "': " << strerror(errno);
            THROW(file, ss.str());
        }

        // Iterate through every entry in the current directory.
        for (struct dirent* ent; (ent = readdir(dir.get()));)
            // If the current entry is a directory other than '.' or '..', add it to the queue, but
            // only bother with this if we're searching recursively.
            if (recursive &&
                    (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
                )
                dirs.emplace(dirs.front() + string(ent->d_name) + "/");
            else if (ent->d_type == DT_REG)
                result.emplace_back(dirs.front() + ent->d_name);

        dirs.pop();
    }

    // Just so that it's in the same order every time.
    sort(result.begin(), result.end());

    // Just in case. Might save us some memory.
    for (string& s : result) s.shrink_to_fit();
    result.shrink_to_fit();
    return result;
}

string read_to_string(const string& path)
{
    ifstream file(path);

    if (!file.good())
    {
        stringstream ss;
        ss << "could not read from '" << path << "': " << strerror(errno);
        THROW(file, ss.str());
    }

    try
    { return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>()); }

    catch (const exception&)
    {
        stringstream ss;
        ss << "could not read from '" << path << "': " << strerror(errno);
        THROW(file, ss.str());
    }
}

}
