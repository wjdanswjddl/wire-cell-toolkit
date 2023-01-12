/*
  RaySvg provides functions and objects to convert C++ objects related
  to RayGrid into SVG.  RayGrid objects include includes 3D objects of
  type Point, Ray (wire center or region or Activity) and Blob
  (corners, bounding rays).

  Conversion proceeds in stages:

  - A "projection" callable converts 3D RayGrid object to 2D
    primitives (Point2d, Ray2d, etc).

  - A "scene" callable given a projection and RayGrid objects
    generates transient SVG in the form of one or more svggpp::elem.
    A scene may be composed of sub-sceen generators.

  - A "writer" takes output of one or more scenes and persists it as
    SVG file or other SVG forms.

 */

#ifndef WIRECELLUTIL_RAYSVG
#define WIRECELLUTIL_RAYSVG

#include "WireCellUtil/RayTiling.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/WireSchema.h"
#include "WireCellUtil/BoundingBox.h"
#include "WireCellUtil/Units.h"

#include "WireCellUtil/svggpp.hpp"

#include <string>
#include <vector>

namespace WireCell::RaySvg {
        
    using wire_vector_t = std::vector<WireSchema::Wire>;

    struct Geom {
        RayGrid::Coordinates coords;
        std::vector<wire_vector_t> wires;
    };

    // Specific scene rendering.  These define some particular mapping
    // from RayGrid objects to SVG with the caller having freedom to
    // compose the returned elements.  The elements are in user
    // coordinates and so should be placed in a parent <svg> with a
    // suitable viewBox.

    // Render activities by aggregating neighbors into a single polygon.
    svggpp::elem_t render_active_regions(const Geom& geom, const RayGrid::activities_t& acts);

    // // Render activities as individual wires
    // svggpp::elem_t render_active_wires(const Geom& geom, const RayGrid::activities_t& acts);

    // // Render blobs represented by their corners
    // svggpp::elem_t render_blob_corners(const Geom& geom, const RayGrid::blobs_t& blobs);

    // // Render blobs represented by areas
    // svggpp::elem_t render_blob_areas(const Geom& geom, const RayGrid::blobs_t& blobs);

#if 0
    namespace deprecated {

        inline
        double x(const Point& pt) { return pt.z(); }
        inline
        double y(const Point& pt) { return pt.y(); }

        // Create a document spanning Y vs Z plane.  Note, scale is
        // applied during serializing transient svg:: objects and is not
        // related to use of SVG viewBox attribute nor tranformation
        // functions.  Given zero, the scale is auto computed based on
        // bounds and wpx/wpy.
        svg::Document document(const std::string& svgname, const Ray& bounds, int wpx=512, int hpx=512, double scale=0);

        inline
        svg::Point point(const Point& p)
        {
            return svg::Point(x(p), y(p));
        }

        inline
        svg::Line line(const Ray& r,
                       svg::Stroke const & stroke = svg::Stroke(0.15, svg::Color::Black))
        {
            return svg::Line(point(r.first), point(r.second), stroke);
        }

        inline
        svg::Circle circle(const Point& center, double radius,
                           svg::Fill const & fill = svg::Fill(),
                           svg::Stroke const& stroke = svg::Stroke(1.0, svg::Color::Black))
        {
            return svg::Circle(point(center), radius, fill, stroke);
        }


        inline
        svg::Polygon polyon(const std::vector<Point>& pts, 
                            svg::Fill const & fill = svg::Fill(),
                            svg::Stroke const& stroke = svg::Stroke(0.15, svg::Color::Black))
        {
            auto pg = svg::Polygon(fill, stroke);
            for (const auto& pt : pts) {
                pg << point(pt);
            }
            return pg;
        }

        class Scene {
            const RayGrid::Coordinates& m_coords;
            const WireSchema::Store& m_wires;

            // collect
            RayGrid::activities_t m_activities;
            RayGrid::strips_t m_astrips, m_bstrips;
            RayGrid::blobs_t m_blobs;
            BoundingBox m_blobs_bb, m_astrips_bb, m_bstrips_bb;

            // cache
            std::vector<std::vector<WireSchema::Wire>> m_wireobjs;
            std::vector<Vector> m_pvecs;

            // style
            std::vector<svg::Color> m_layer_colors = {
                svg::Color::White,
                svg::Color::White,
                svg::Color::Red,
                svg::Color::Green,
                svg::Color::Blue,
            };
            svg::Color m_outline_color{ svg::Color::White };
            svg::Color m_corner_color{ svg::Color::Purple };
            double m_corner_radius{ 1.0*units::mm };
            double m_thin_line{ 0.15*units::mm };

          public:
            Scene(const RayGrid::Coordinates& coords, const WireSchema::Store& wires);

            void operator()(const RayGrid::activities_t& acts);
            void operator()(const RayGrid::blobs_t& blobs);

            // Write blob-centric scene to svg file
            void blob_view(const std::string& svgname,
                           int wpx = 512, int hpx = 512,
                           double scale = 0);

            // Write wire-centric scene to svg file
            void wire_view(const std::string& svgname);


            // Return svg representations of ray grid things
            using shape_p = std::unique_ptr<svg::Shape>;
            shape_p strip_shape(int layer, int grid, bool inblob);
            shape_p corner_shape(const RayGrid::crossing_t& c);
            
        };
    }
#endif
};

#endif
