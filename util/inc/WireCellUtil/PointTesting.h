/** Helpers for testing point-related. */

#ifndef WIRECELLUTIL_POINTTESTING
#define WIRECELLUTIL_POINTTESTING

#include "WireCellUtil/PointCloud.h"
#include "WireCellUtil/Point.h"

namespace WireCell::PointTesting {

    /// Return a dataset with x,y,z,q made by crudely sampling a line
    PointCloud::Dataset make_janky_track(
        // The track line segment.
        const Ray& track = Ray(Point(-1, -2, -3), Point(1, 2, 3)),
        // Sampling step size.
        double step_size = 0.1);
}

#endif
