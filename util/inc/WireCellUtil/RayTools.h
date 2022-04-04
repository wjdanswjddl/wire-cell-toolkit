#ifndef WIRECELLUTIL_RAYTOOLS
#define WIRECELLUTIL_RAYTOOLS

#include "WireCellUtil/RayTiling.h"

namespace WireCell::RayGrid {

     // Return a distance between centers of two strips relative to their widths
     //
     // A value of 0.0 means the two strips share a common center.
     // A value of 1.0 means they are non overlapping but share and edge.
     double relative_distance(const grid_range_t& a, const grid_range_t& b);

     // Return a relative distance between two sets of strips spanning
     // the same layer ordering.
     //
     // This is the L2 norm of all the layer relative distances.
     double relative_distance(const strips_t& a, const strips_t& b);
     
}

#endif
