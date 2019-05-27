#include <sstream>
#include <memory>
#include <exception>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include "StegFile.h"
#include "exc.h"
#include "util.h"

using namespace std;

StegFile::StegFile(const std::string& path): _x(0), _y(0), _n(0), _extension()
{
    // TODO 3 Kind of stupid to decode the entire image just to get its size. Change this to
    //        something more efficient?

    _path  = path;
    _bytes = nullptr;

    {
        // Set `_extension` to everything after the dot. If there's no dot, or there's nothing after
        // the dot, it will be empty. In which case, loading the image will fail anyway in a moment.
        auto dot = path.rfind(".");
        _extension = dot == string::npos ? "" : util::upper(path.substr(dot + 1));
    }

    // Check the extension now, otherwise it will only fail when we come to write. Be nice and do it
    // sooner.
    if (!(_extension == "PNG" || _extension == "BMP" || _extension == "TGA"))
        THROW(file, "only PNG, BMP and TGA images are supported, for now");

    unique_ptr<unsigned char, void(*)(unsigned char*)> image
    (
        stbi_load(path.c_str(), &_x, &_y, &_n, 0),
        [](unsigned char* image){ stbi_image_free(image); }
    );

    // smb_image_write doesn't output the fourth channel, but smb_image will read it (but ignore
    // it.) So what happens is, when we `.sync()` once, it will be written with 3 channels, and when
    // we go to `.sync()` again, it will complain that the file has changed, because it has. To fix
    // this, I could use a different image library, but not today.
    if (_extension == "BMP" && _n == 4)
        THROW(file, "4-channel BMP is not supported");

    if (!image)
    {
        stringstream ss;
        ss << "could not open image at '" << path << "': " << stbi_failure_reason();
        THROW(file, ss.str());
    }

    _capacity = (_x * _y * _n) / 8;
}

void StegFile::prepare()
{
    // Allocate `_bytes` if it isn't already allocated.
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

    // Load the image so we can read its bits and store them in `_bytes`.
    int x, y, n;

    // Yes, I know: the image consists of unsigned chars, but we're reading into a buffer of chars.
    // This doesn't matter.

    unique_ptr<unsigned char, void(*)(unsigned char*)> image
    (
        stbi_load(_path.c_str(), &x, &y, &n, 0),
        [](unsigned char* image){ stbi_image_free(image); }
    );

    if (!image)
    {
        stringstream ss;
        ss << "could not open image at '" << _path << "': " << stbi_failure_reason();
        THROW(file, ss.str());
    }

    // Strictly speaking, the image could change as long as it stays the same size, but if that
    // happens your whole file system is ruined anyway. Don't do that, you imbecile.
    if (x != _x || y != _y || n != _n)
    {
        stringstream ss;
        ss << "image at '" << _path << "' has changed";
        THROW(file, ss.str());
    }

    // For each byte in `_bytes`, look at 8 bytes in `image`, construct the resulting byte from the
    // last bits of these, and store it in `_bytes`.
    for (size_t i = 0; i < _capacity; ++i)
    {
        // Location of the 8 bytes in `image` we're going to look at.
        size_t loc = i * 8;

        // Byte we're building up, to add to `_bytes` when we're done with it.
        unsigned char byte = 0;

        // For each byte in the 8 bytes starting at image[loc]...
        for (short bit = 0; bit < 8; ++bit)
        {
            // If the last bit is set, add it in the right place in the byte we're building.
            if (image.get()[loc + bit] & 1)
                byte += 1 << bit;
        }

        // Now set the byte we built.
        _bytes[i] = byte;
    }
}

void StegFile::sync()
{
    // Check if we're already synced.
    if (synced()) return;

    // First, we load the image from the file system. Then we hide `_bytes` in it and write the
    // result.
    int x, y, n;

    unique_ptr<unsigned char, void(*)(unsigned char*)> image
    (
        stbi_load(_path.c_str(), &x, &y, &n, 0),
        [](unsigned char* image){ stbi_image_free(image); }
    );

    if (!image)
    {
        stringstream ss;
        ss << "could not open image at '" << _path << "': " << stbi_failure_reason();
        THROW(file, ss.str());
    }

    if (x != _x || y != _y || n != _n)
    {
        stringstream ss;
        ss << "image at '" << _path << "' has changed";
        THROW(file, ss.str());
    }

    // For every byte in `_bytes`...
    for (size_t i = 0; i < _capacity; ++i)
    {
        size_t loc = i * 8;

        // ...iterate through every bit.
        for (short bit = 0; bit < 8; ++bit)
        {
            // Set the last bit of the corresponding byte in `image` to the same value as this bit.
            if (_bytes[i] & (1 << bit)) image.get()[loc + bit] |= 1;
            else                        image.get()[loc + bit] &= ~1;
        }
    }

    // Now it comes time to write this bad boy.
    int result = 0;

    if (_extension == "PNG")
        result = stbi_write_png(_path.c_str(), _x, _y, _n, image.get(), _x * _n);
    else if (_extension == "BMP")
        result = stbi_write_bmp(_path.c_str(), _x, _y, _n, image.get());
    else if (_extension == "TGA")
        result = stbi_write_tga(_path.c_str(), _x, _y, _n, image.get());

    if (!result)
    {
        stringstream ss;
        ss << "could not write image to '" << _path << "'";
        THROW(file, ss.str());
    }
}
