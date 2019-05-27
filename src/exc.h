#ifndef EXC_H
#define EXC_H

// This file contains various exceptions for use within this program.

#include <iostream>
#include <string>
#include <sstream>

#include "util.h"

#define THROW(type, what){throw exc::type(util::pretty_trim(__PRETTY_FUNCTION__, __func__) + what);}

namespace exc
{

// An abstract exception type. Any exception created by me should derive from this. Any exceptions
// which make it to main() uncaught, and do not derive from this, are a bug. This exception should
// NOT be thrown, only caught.
class exception
{
    public:
    exception(const std::string& what): _what(what) { }

    void print(const char* name, std::ostream& out = std::cerr) const
    { out << name << ": error: " << _what << std::endl; }

    const std::string& what() const { return _what; }

    protected:
    // In case you want to derive from this. You better know what you're doing!
    exception(): _what() { };
    std::string _what;
};

// To throw when something is not yet implemented. Use this as a placeholder while building the
// program, or in the body of a method which is meant to be overridden. If this ever ends up getting
// thrown, there's a bug somewhere.
class unimplemented : public exception
{
    public:
    // Since the THROW macro includes the calling function name, `what` is basically useless. Just
    // append 'not implemented' to it, so you can use 'THROW(unimplemented, "")' to get
    // '`function()`: not implemented'
    // still use that macro easily, we still take a `what` argument, but just ignore it.
    unimplemented(const std::string& what): exception(what + "not implemented") { }
};

// To throw when there's some file I/O error.
class file : public exception
{
    public:
    file(const std::string& what): exception(what) { }
};

// To throw when someone tries to make a memory allocation that's too big and beautiful.
class too_big : public exception
{
    public:
    too_big(const std::string& what): exception(what) { }
};

// To throw when an argument to a function had an invalid value. This is intended to represent
// errors caused by the programmer, not the user, so if this ever makes it to `main()`, there's a
// bug somewhere. If this was caused by the user entering a dodgy value, throw something else.
class arg : public exception
{
    public:
    arg(const std::string& what): exception(what) { }
};

}

#endif
