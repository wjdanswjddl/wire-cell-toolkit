/*
  RaySvg provides functions and objects to convert C++ objects related
  to RayGrid into SVG.  RayGrid objects include includes 3D objects of
  type Point, Ray (wire center or region or Activity) and Blob
  (corners, bounding rays).

 */

#ifndef WIRECELLUTIL_RAYSVG
#define WIRECELLUTIL_RAYSVG

#include "WireCellUtil/RayTiling.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/WireSchema.h"
#include "WireCellUtil/Units.h"

#include "WireCellUtil/svggpp.hpp"

#include <string>
#include <vector>

namespace WireCell::RaySvg {
        
    // Define the projection from 3D space to 2D space.  Avoid
    // performing the projection inline and instead call this.
    using svggpp::point_t;
    point_t project(const Point& pt);

    // Return a viewBox attribute set from a BoundingBox.bounds() Ray.
    svggpp::attr_t viewbox(const Ray& bbray);

    using wire_vector_t = std::vector<WireSchema::Wire>;

    struct Geom {
        RayGrid::Coordinates coords;
        std::vector<wire_vector_t> wires;
    };

    /// The following functions render their arguments to an <svg>
    /// represented as an object.  

    /// A full rendering.  This composes a number of <svg> from other
    /// render_*() functions.  It returns an <svg> with two major
    /// drawing areas with common width and height in user
    /// coordinates.  First is coarse and at x=0,y=0 and second is
    /// fine and at x=width,y=height.  Each contains links to views of
    /// the other.
    svggpp::elem_t render(const Geom& geom, const RayGrid::activities_t& acts, const RayGrid::blobs_t& blobs);

    // Render the first two layers providing horiz/vert active area.
    svggpp::elem_t render_active_area(const Geom& geom);

    // // Render wire plane activities as strips.
    // svggpp::elem_t render_active_strips(const Geom& geom, const RayGrid::activities_t& acts);

    // // Render activities as individual wires.
    // svggpp::elem_t render_active_wires(const Geom& geom, const RayGrid::activities_t& acts);

    // // Render blobs represented by their corners
    // svggpp::elem_t render_blob_corners(const Geom& geom, const RayGrid::blobs_t& blobs);

    // // Render blobs represented by areas
    // svggpp::elem_t render_blob_areas(const Geom& geom, const RayGrid::blobs_t& blobs);

};

#endif
