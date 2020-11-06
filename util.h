#ifndef __util_h
#define __util_h

#include <inttypes.h>

#include <iostream>
#include <vector>

/// User defined literal for metric lengths.
constexpr int32_t operator"" _m(const unsigned long long meters) {
    return static_cast<int32_t>(meters);
}
constexpr double operator"" _m(const long double meters) { return meters; }

/// User defined literal for meters/second velocities.
constexpr double operator"" _mps(const long double mps) { return mps; }

/// User defined literal for byte counts.
constexpr int32_t operator"" _b(const unsigned long long bytes) {
    return static_cast<int32_t>(bytes);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); i++) {
        os << vec[i];
        if (i < vec.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

#endif