#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

#include "WireCellUtil/Math.h"

using namespace WireCell;

int main(void)
{
    const double fraction = 0.02;

    //for (auto capacity : capacities) {
    for (size_t capacity = 3; capacity < 10000; ++capacity) {
        const size_t target = (1-fraction)*capacity;
        auto got = nearest_coprime(capacity, target);
        assert (got != 0);
        int err = got-target;
        std::cerr << "capacity=" << capacity
                  << " target=" << target
                  << " got=" << got
                  << " error=" << err << "\n";
    }
    return 0;
}
