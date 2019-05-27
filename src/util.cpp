#include <string>

#include "util.h"

using namespace std;

namespace util
{

string upper(const string& s)
{
    string result(s);
    for (char& c : result) c = toupper(c);
    return result;
}

}
