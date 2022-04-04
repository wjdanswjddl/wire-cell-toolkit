#include "WireCellUtil/RayTools.h"
#include <cassert>
#include <iostream>

using namespace WireCell::RayGrid;

int main()
{
    {
        double d = relative_distance({5,60}, {60,90});
        std::cerr << d << "\n";
        assert (std::abs(d-1.0) < 1e-8);
    }
    {
        double d = relative_distance({5,15}, {9,11});
        std::cerr << d << "\n";
        assert (std::abs(d) < 1e-8);
    }
    {
        double d = relative_distance({1,2}, {4,5});
        std::cerr << d << "\n";
        assert (std::abs(d-3) < 1e-8);
    }
}
