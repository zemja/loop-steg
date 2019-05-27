#ifndef UTIL_H
#define UTIL_H

// This file contains miscellaneous utility functions which don't belong anywhere else.

namespace util
{

// Takes a function signature, such as:
//
// "std::vector<std::__cxx11::basic_string<char> > Class::method(std::stack<unsigned char>)"
//
// And trims it down to:
//
// "`Class::method()`: "
//
// For printing at the start of error messages.
//
// pretty_function: Function signature, as given by __PRETTY_FUNCTION__. This must contain `func` +
//                  '(' somewhere in it!
// func:            Function name within the function signature, to aid in finding it, as given by
//                  __func__.
//
// Returns the trimmed result.
//
// NOTE: You're expected to only really use this function as:
//
// pretty_trim(__PRETTY_FUNCTION_, __func__)
//
// This function was written so I could have a macro, `THROW` (below), which throws an exception and
// uses the line above in the `.what()` message, so exception messages contain the method they were
// created in.
inline std::string pretty_trim(const std::string& pretty_function, const std::string& func)
{
    auto end   = pretty_function.find(func + "(") + func.length();
    auto begin = pretty_function.rfind(' ', end) + 1;
    return std::string("`") + pretty_function.substr(begin, end - begin) + std::string("()`: ");
}

// Converts a string to upper case. Uses `std::toupper()`.
//
// s: The string to convert.
//
// Returns `s` in upper case, as defined by `std::toupper()`.
std::string upper(const std::string& s);

}

#endif
