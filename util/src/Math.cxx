#include "WireCellUtil/Math.h"

size_t WireCell::GCD(size_t a, size_t b)
{
    while(true) {
        a = a % b;
        if (a == 0) {
            return b;
        }
        b = b % a;
        if (b == 0) {
            return a;
        }
    }
}

size_t WireCell::nearest_coprime(size_t number, size_t target)
{
    size_t lo=0, hi=number/2;
    if (target > number/2) {
        lo = number/2;
        hi = number;
    }
    const size_t maxstep = std::max(target-lo, hi-target);
    for (size_t step = 0; step < maxstep; ++step) {
        {
            const size_t below = target - step;
            if (below >= lo) {
                if (1 == WireCell::GCD(number, below)) {
                    return below;
                }
            }
        }
        {
            const size_t above = target + step;
            if (above < hi) {
                if (1 == WireCell::GCD(number, above)) {
                    return above;
                }
            }
        }
    }
    return 0;
}
