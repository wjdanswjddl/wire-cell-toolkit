#include "WireCellUtil/RayTools.h"

using namespace WireCell::RayGrid;

double WireCell::RayGrid::relative_distance(const grid_range_t& a, const grid_range_t& b)
{
    double num = std::abs((a.first+a.second) -         (b.first+b.second));
    double den = std::abs( a.first-a.second) + std::abs(b.first-b.second);
    return num/den;
}


double WireCell::RayGrid::relative_distance(const strips_t& a, const strips_t& b)
{
    double ret = 0;
    size_t nlayers = a.size();
    for (size_t ind=0; ind<nlayers; ++ind) {
        double lrd = relative_distance(a[ind].bounds, b[ind].bounds);
        ret += lrd*lrd;
    }
    return ret/nlayers;
}
