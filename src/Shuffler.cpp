#include <vector>
#include <random>
#include <algorithm>

#include "Shuffler.h"

using namespace std;

// TODO 3 This is temporary, replace this with a MUCH better implementation. Take a look at feistel
//        networks.
// For now, `Shuffler` just generates the sequence and shuffles it randomly based on a seed. This
// sucks because the whole sequence is in memory all the time.

Shuffler::Shuffler(size_t a, size_t b, const string& seed):
    _min(min(a, b)),
    _max(max(a, b)),
    _v(_max - _min, _min)
{
    for (size_t i = 1 ; i < _v.size(); ++i) _v[i] += i;

    seed_seq seq(seed.cbegin(), seed.cend());
    mt19937_64 gen(seq);
    uniform_int_distribution<> dis(0, _v.size() - 1);

    for (size_t i = 0, temp = 0, rand = 0; i < _v.size(); ++i)
    {
        rand = dis(gen);
        temp = _v[i];
        _v[i] = _v[rand];
        _v[rand] = temp;
    }

    // Probably useless, but just in case.
    _v.shrink_to_fit();
}

size_t Shuffler::operator[](size_t i)
{ return _v[i]; }

vector<size_t> Shuffler::get(size_t i, size_t n)
{ return vector<size_t>(_v.cbegin() + i, min(_v.cbegin() + i + n, _v.cend())); }
