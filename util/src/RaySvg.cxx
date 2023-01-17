#include "WireCellUtil/RaySvg.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Range.h"

#include <boost/range/irange.hpp>

using namespace svggpp;


// using namespace WireCell;
using WireCell::Ray;
using WireCell::Point;
using WireCell::Vector;
using namespace WireCell::Range;
using namespace WireCell::RaySvg;
using namespace WireCell::String;

// Define the projection from 3D space to 2D space.  Avoid performing
// the projection inline and instead call this.
svggpp::point_t WireCell::RaySvg::project(const Point& pt)
{
    return svggpp::point_t(pt.z(), pt.y());
}

svggpp::attr_t WireCell::RaySvg::viewbox(const Ray& bbray)
{
    auto bb1 = project(bbray.first);
    auto bbvec = project(ray_vector(bbray));
    return svggpp::viewbox(bb1.first, bb1.second,
                           bbvec.first, bbvec.second);
}


// convert raygrid to svggpp

static
svggpp::attr_t to_point(const Point& pt)
{
    return point(project(pt));
}
static
std::vector<svggpp::point_t> to_points(const std::vector<Point>& pts)
{
    std::vector<svggpp::point_t> spts;
    for (const auto& pt : pts) {
        spts.emplace_back(project(pt));
    }
    return spts;
}
static
std::vector<svggpp::point_t> to_ring(const std::vector<Point>& pts)
{
    size_t npts = pts.size();
    Point avg;
    for (const auto& pt : pts) {
        avg += pt;
    }
    avg = avg / (double)npts;
    std::vector<double> ang(npts);
    std::vector<size_t> indices(npts);
    for (size_t ind : irange(npts)) {
        auto dir = pts[ind] - avg;
        ang[ind] = atan2(dir[2], dir[1]);
        indices[ind] = ind;
    }
    std::sort(indices.begin(), indices.end(),
              [&](size_t a, size_t b) -> bool {
                  return ang[a] < ang[b];
              });    

    std::vector<svggpp::point_t> spts(npts);
    for (size_t ind : irange(npts)) {
        spts[ind] = project(pts[indices[ind]]);
    }
    return spts;
}

const WireCell::WireSchema::Wire& get_wire(const std::vector<WireCell::WireSchema::Wire>& wires, int ind)
{
    if (ind < 0) {
        ind = 0;
    }
    else if (ind >= (int)wires.size()) {
        ind = ((int)wires.size())-1;
    }
    return wires[ind];
}

svggpp::elem_t WireCell::RaySvg::render(const Geom& geom, const RayGrid::activities_t& acts, const RayGrid::blobs_t& blobs)
{
    auto aa = render_active_area(geom);
    aa.update({{"id","bounds"}});

    auto vport = viewport(aa);
    const double width = cast<double>(vport["width"]);
    const double height = cast<double>(vport["height"]);

    // Initial outer svg viewbox and inner viewbox of each major svg
    const auto vbox = svggpp::viewbox(0, 0, width, height);

    auto top = svg(vbox, defs(aa));
    
    auto s1 = svg(vbox);
    s1.update(point(0,0));
    s1.update(size(width, height));
    auto v1 = view("coarse", vbox);
    svggpp::append(s1, use(link(aa)));

    auto s2 = svg(vbox);
    s2.update(point(width,height));
    s2.update(size(width,height));
    auto v2 = view("fine", svggpp::viewbox(width, height, width, height));
    svggpp::append(s2, use(link(aa)));
    
    svggpp::append(top, anchor(link(v2), {}, s1));
    svggpp::append(top, anchor(link(v1), {}, s2));

    // coarse
    // - use aa with x=0,y=0
    // - active strips
    // - blob areas with link to fine blob areas

    // fine
    // - use aa with x=100,y=100
    // - active wires
    // - blob areas with link to coarse 
    // - blob corners

           
    return top;
}


svggpp::elem_t WireCell::RaySvg::render_active_area(const Geom& geom)
{
    BoundingBox bb;
    RayGrid::crossings_t xings = {
        { {0,0}, {1,0} },
        { {0,0}, {1,1} },
        { {0,1}, {1,1} },
        { {0,1}, {1,0} } };
    auto bpts = geom.coords.ring_points(xings);
    bb(bpts.begin(), bpts.end());

    auto pts = to_points( bpts ) ;
    auto pgon = polygon( pts );

    auto attr = css_class("active bounds");
    attr.update(viewbox(bb.bounds()));
    auto top = svg(attr, pgon);
    return top;
}

#if 0

svggpp::elem_t WireCell::RaySvg::render_active_strips(const Geom& geom, const RayGrid::activities_t& acts)
{
    BoundingBox bb;

    auto top = svg(css_class("active strips"));

    std::vector<int> plane_numbers = {2,1,0};

    for (int plane : plane_numbers) {
        int layer = plane+2;
        const auto & act = acts[layer];
        
        const auto& wires = geom.wires[plane];

        const auto& pdir = geom.coords.pitch_dirs()[layer];
        const auto& pmag = geom.coords.pitch_mags()[layer];
        const auto phalf = 0.4*pmag*pdir;

        const auto strips = act.make_strips();
        // std::cerr << "plane=" << plane << " nstrips="<<strips.size() << "\n";
        for (const RayGrid::Strip& strip : strips) {
            const auto& w1 = get_wire(wires, strip.bounds.first);
            const auto& w2 = get_wire(wires, strip.bounds.second-1);
            std::vector<Point> pts = {
                w1.tail-phalf,
                w1.head-phalf,
                w2.head+phalf,
                w2.tail+phalf };
            bb(pts.begin(), pts.end());
            auto spgon = polygon(
                to_ring( pts ),
                css_class(format("activity plane%d", plane)));
            svggpp::append(top, spgon);
        }    
    }

    update(top, viewbox(bb.bounds()));
    return top;
}


svggpp::elem_t WireCell::RaySvg::render_active_wires(const Geom& geom, const RayGrid::activities_t& acts)
{
    BoundingBox bb;

    auto top = svg(css_class("active strips"));

    std::vector<int> plane_numbers = {2,1,0};

    for (int plane : plane_numbers) {
        int layer = plane+2;
        const auto & act = acts[layer];
        
        const auto& wires = geom.wires[plane];

        const auto& pdir = geom.coords.pitch_dirs()[layer];
        const auto& pmag = geom.coords.pitch_mags()[layer];
        const auto phalf = 0.4*pmag*pdir;

        const auto strips = act.make_strips();
        // std::cerr << "plane=" << plane << " nstrips="<<strips.size() << "\n";
        for (const RayGrid::Strip& strip : strips) {
            for (auto wip : irange(strip.bounds)) {
                std::cerr << strip << " " << wip<< std::endl;
                const auto& w = get_wire(wires, wip);
                std::vector<Point> pts = {
                    w.tail-phalf,
                    w.tail+phalf,
                    w.head+phalf,
                    w.head-phalf
                };
                bb(pts.begin(), pts.end());
                auto spgon = polygon(
                    to_ring( pts ),
                    css_class(format("activity plane%d wip%d", plane, wip)));
                svggpp::append(top, spgon);
            }
        }
    }

    update(top, viewbox(bb.bounds()));
    return top;
}


svggpp::elem_t WireCell::RaySvg::render_blob_corners(const Geom& geom, const RayGrid::blobs_t& blobs)
{
    BoundingBox bb;
    int count=0;
    auto top = svg(css_class("blob_corners"));

    for (const auto& blob : blobs) {
        ++count;

        auto grp = group(css_class(format("blobnum%d", count)));

        for (const auto& [a,b] : blob.corners()) {
            auto pt = geom.coords.ray_crossing(a,b);
            bb(pt);
            auto cir = circle(project(pt)); // fixme: radius
            svggpp::append(grp, cir);
        }
    }

    update(top, viewbox(bb.bounds()));
    return top;
}


svggpp::elem_t WireCell::RaySvg::render_blob_areas(const Geom& geom, const RayGrid::blobs_t& blobs)
{
    BoundingBox bb;
    int count=0;
    auto top = svg(css_class("blob_areas"));

    for (const auto& blob : blobs) {
        ++count;

        BoundingBox bbb;
        std::vector<Point> pts;
        for (const auto& [a,b] : blob.corners()) {
            pts.push_back(geom.coords.ray_crossing(a,b));
        }

        bb(pts.begin(), pts.end());
        bbb(pts.begin(), pts.end());

        std::string bname = format("blobnum%d", count);

        auto spgon = polygon(
            to_ring( pts ),
            css_class(bname));
        auto v = view(bname, viewbox(bbb.bounds()));
        auto a = anchor(link(v), {}, spgon);
        svggpp::append(top, a);

        svggpp::append(top, v);
    }

    update(top, viewbox(bb.bounds()));
    return top;
}




#endif



