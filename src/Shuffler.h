#ifndef SHUFFLER_H
#define SHUFFLER_H

#include <string>
#include <vector>

// This class generates random permutations of integers in a given range. Say you want the integers
// 1 to 10, and want to shuffle them into a random order, but don't want to have the whole sequence
// in memory the entire time? Well, `Shuffler`'s got your back. You'd use it like:
//
// Shuffler s(1, 11, 1234); // 1 = min, 11 = max (not inclusive), 1234 = seed
// cout << s[0] << endl;    // First value in the shuffled list.
// cout << s[1] << endl;    // Second value in the shuffled list.
// cout << s[2] << endl;    // ...etc.
//
// TODO 3 Update this when you improve `Shuffler`.
// The whole point of this class is that these values are calculated for you; you don't need to keep
// the list in memory the whole time. In truth, this is not the case for now, the entire list is
// generated and shuffled and kept in memory. Eventually I will replace this with something more
// complex, when the program actually does the thing it's meant to do.
class Shuffler
{
    public:
    // `Shuffler` constructor.
    //
    // a:    Lower bound (inclusive).
    // b:    Upper bound (not inclusive).
    // seed: Seed to use when randomising. Same seed = same output.
    //
    // NOTE: It actually doesn't matter which way around `a` and `b` go. They'll be swapped if
    //       they're the wrong way round. In any case, the upper bound is not inclusive but the
    //       lower bound is.
    Shuffler(size_t a, size_t b, const std::string& seed);

    // Indexes a `Shuffler`, returning a single value.
    //
    // i: The index of the value to retrieve.
    //
    // Returns the value.
    size_t operator[](size_t i);

    // Gets multiple values from a shuffler in a `vector`. This should be more efficient than
    // getting each value one by one.
    //
    // i: The index of the first value to retrieve.
    // n: The number of values to retrieve.
    //
    // Returns a `vector` of the desired values.
    std::vector<size_t> get(size_t i, size_t n);

    private:
    // `a` or `b` from `Shuffler(size_t, size_t, size_t)`, whichever was lesser.
    size_t _min;

    // `a` or `b` from `Shuffler(size_t, size_t, size_t)`, whichever was greater.
    size_t _max;

    // All the values between `_min` and _max`. This is only here temporarily; the whole point of
    // this class is that this vector doesn't need to be kept around in memory.
    std::vector<size_t> _v;
};

#endif
