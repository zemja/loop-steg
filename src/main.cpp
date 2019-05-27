// Copyright (C) 2019  Ethan Ansell
//
//     This program is free software: you can redistribute it and/or modify it under the terms of
//     the GNU General Public License as published by the Free Software Foundation, either version 3
//     of the License, or (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//     without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
//     the GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License along with this program. If
//     not, see <https://www.gnu.org/licenses/>.
//
//     Contact: <mail@zemja.org>.

#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <chrono>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

#define FUSE_USE_VERSION 34
#include <fuse3/fuse.h>

// For now, the only place I use <stb_image.h> is in <StegFile.cpp>. Configure it here, and at the
// start of `main()`.
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include "exc.h"
#include "fs.h"
#include "CachedFile.h"
#include "Manager.h"
#include "StegFile.h"

// This program uses a FUSE file system to expose one virtual file to the operating system. Any
// reads or writes done by other programs to this file are distributed randomly across a series of
// steganographic cover files. The idea is that you can make a block device out of the virtual FUSE
// file, encrypt it with LUKS, and store encrypted files among the images.
//
// This program caches file contents in memory, and writes them for real when the file system is
// unmounted, for efficiency. To do this, the `CachedFile` class is used, in <CachedFile.h>. This
// class wraps a file in the file system. Whenever a `.read()` or `.write()` request is made to a
// `CachedFile`, it is done in memory. The file is flushed to the file system, and the writes are
// done for real, when `.sync()` is called. The file is only loaded from the file system upon the
// first `.read()` or `.write()`. This is how write caching is implemented: every cover image is
// represented by a `CachedFile`, and when the FUSE file system is flushed, all the `CachedFile`s
// are flushed.
//
// So how do we deal with the complicated business of splitting writes randomly across many files?
// Well, we have a class which derives from `CachedFile`, namely `Manager`. This class takes the
// path to a directory and creates `CachedFile`s out of every file it finds in there. Then it
// pretends to the outside as if it's just one big file, and any `.read()`s or `.write()`s done to
// this 'file' are distributed randomly across all the `CachedFile`s behind the scenes.
//
// Strictly speaking, it's actually `StegFile` which is used, in <StegFile.h>, which derives from
// `CachedFile`, and performs steganography on an image file to save its data when `.sync()`-ing.
// You could add support for more file types, or more methods of steganography, by providing more
// implementations of `CachedFile`.
//
// So that's basically how the program works. Other points of interest include:
//
// <Shuffler.h>: `Shuffler` class, which calculates random permutations of integers between two
//               values. Used by `Manager` to randomly distribute bytes.
// <exc.h>:      Exception classes.
// <fs.h>:       Interactions with the file system. (I mean the real file system, i.e. reading and
//               writing to files, nothing to do with FUSE.)
// <util.h>:     Miscellaneous utilities.
//
//
// This file, <main.cpp>, contains the FUSE file system functions, anything to do with
// instantiating/using the one instance of `Manager`, and of course houses the `main()` function.

using namespace std;

// If true, don't print messages upon entering the various FUSE functions.
const bool SHUT_UP = false;

// We'll initialise these in `main()`.
unique_ptr<Manager> MANAGER; // Where all the magic happens.
const char* NAME = nullptr;  // `argv[0]`, for passing to `exc::exception::print()`.

// The name of the one file in our FUSE file system.
const char* FILENAME = "data";

// Initialises the file system.
void* init(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
    (void)conn;

    // This option disables flushing the cache of file contents on every `open()`.
    // This is only usable on file systems where the files cannot be accessed in other ways
    // outside this FUSE file system. (That's us!)
    cfg->kernel_cache = 1;

    return NULL;
}

// Gets a file's attributes, i.e. last modified time, permissions, stuff like that.
int getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
    (void)fi;

    if (!SHUT_UP) cout << "`getattr()`: entering function" << endl;

    int result = 0;

    memset(stbuf, 0, sizeof(struct stat));

    // Only let the user get attributes of the root directory itself, or the one and only file
    // within.

    // Root directory itself.
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode  = S_IFDIR | 0755; // rwx r-x r-x
        stbuf->st_nlink = 2; // Two links, one for / and one for /.
    }

    // Single file.
    else if (strcmp(path + 1, FILENAME) == 0)
    {
        stbuf->st_mode  = S_IFREG | 0755; // rwx r-x r-x
        stbuf->st_nlink = 1;
        stbuf->st_size  = MANAGER->capacity();
    }

    else result = -ENOENT;

    return result;
}

// Listing a directory.
int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    if (!SHUT_UP) cout << "`readdir()`: entering function" << endl;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".",      NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..",     NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, FILENAME, NULL, 0, (fuse_fill_dir_flags)0);

    return 0;
}

// Opening a file.
int open(const char* path, struct fuse_file_info* fi)
{
    (void)fi;

    if (!SHUT_UP) cout << "`open()`: entering function" << endl;

    // Only let them open our one file.
    if (strcmp(path + 1, FILENAME) != 0)
        return -ENOENT;

    // Don't actually need to do anything.

    return 0;
}

// Reading from a file.
int read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    (void)fi;

    if (!SHUT_UP) cout << "`read()`: path: '" << path << "' size: " << size << " offset: " << offset
        << endl;

    // Only let them read our one file.
    if (strcmp(path + 1, FILENAME) != 0)
        return -ENOENT;

    int result = 0;

    try { result = MANAGER->read(buf, size, offset); }

    catch (const exc::exception& e)
    {
        e.print(NAME);
        result = -EIO;
    }

    return result;
}

// Writing to a file.
int write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    (void)fi;

    if (!SHUT_UP) cout << "`write()`: path: '" << path << "' size: " << size << " offset: "
        << offset << endl;

    // You know how it is by now.
    if (strcmp(path + 1, FILENAME) != 0)
        return -ENOENT;

    // You'd think we'd need to check to ensure nobody's writing off the end of the file here, since
    // our file isn't supposed to change size. Actually, we don't really care about that. Not our
    // job. Once the file is mounted as a loop device, we won't be able to write off the end of that
    // anyway.

    int result = 0;

    try { result = MANAGER->write(buf, size, offset); }

    catch (const exc::exception& e)
    {
        e.print(NAME);
        result = -EIO;
    }

    return result;
}

int main(int argc, char *argv[])
{
    // TODO 5 Document somewhere or somehow make it obvious to the user that any modifications made
    //        to mounted files by external programs while this program is running will mess
    //        everything up.

    // stbi_image_write options; disable PNG compression.
    stbi_write_png_compression_level = 0;
    stbi_write_force_png_filter      = 0;

    NAME = argv[0];

    if (argc < 4)
    {
        cout << "Usage: " << NAME << " <seed file> <target directory> <mount point>"
            " [<FUSE mount options>]" << endl;
        return 1;
    }

    // Let the seed file be the first argument, and the target directory be the second. Now shuffle
    // the arguments so FUSE just sees "./a.out <mount point> [<FUSE mount options>]".
    string seed(argv[1]);
    string path(argv[2]);

    // Make the path absolute, otherwise terrible things happen. If the path doesn't exist, just
    // leave it how it is. Some other part of our code can handle that.
    {
        char* c_path;

        if ((c_path = realpath(argv[2], nullptr)))
        {
            path = c_path;
            free(c_path);
        }
    }

    // Remove the first two arguments.
    argv[2] = argv[0];
    argv += 2;
    argc -= 2;

    // If this isn't static, then you get a 'transport endpoint not connected' error for some
    // bizarre reason I don't understand.
    static struct fuse_operations oper;
    oper.getattr = getattr;
    oper.open    = open;
    oper.read    = read;
    oper.write   = write;
    oper.readdir = readdir;
    oper.init    = init;

    int result = 1;

    try
    {
        seed = fs::read_to_string(seed);

        auto start = chrono::high_resolution_clock::now();
        MANAGER = unique_ptr<Manager>(new Manager(path, seed));
        auto end = chrono::high_resolution_clock::now();

        if (!SHUT_UP)
            cout << "Set up time: "
                << chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000.0
                << "ms" << endl;

        result = fuse_main(argc, argv, &oper, NULL);

        start = chrono::high_resolution_clock::now();
        MANAGER->sync();
        end = chrono::high_resolution_clock::now();

        if (!SHUT_UP)
            cout << "Sync time: "
                << chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1000000.0
                << "ms" << endl;
    }

    catch (const exc::exception& e)
    {
        e.print(NAME);
        return 1;
    }

    return result;
}
