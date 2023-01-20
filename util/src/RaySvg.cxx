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

svggpp::view_t WireCell::RaySvg::project(const Ray& bbray)
{
    auto bb1 = project(bbray.first);
    auto bbvec = project(ray_vector(bbray));
    view_t view;
    view.x = bb1.first;
    view.y = bb1.second;
    view.width = bbvec.first;
    view.height = bbvec.second;
    return view;
}

// convert raygrid to svggpp

static
svggpp::xml_t to_point(const Point& pt)
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

svggpp::xml_t WireCell::RaySvg::svg_dual(const Geom& geom, const RayGrid::activities_t& acts, const RayGrid::blobs_t& blobs)
{
    auto aa = svg_active_area(geom);
    id(aa, "aa");
    auto aabb = viewbox(aa);
    attrs(aa).erase("viewBox");
    auto common = defs(aa);
    
    const double nominal_size = 512;

    // Nominal view
    auto aabb1 = aabb;

    // A view displaced diagonally by size, same size
    auto aabb2 = aabb1;
    aabb2.x += aabb2.width;
    aabb2.y += aabb2.height;

    // what we will return.
    auto top = svg(svg_header);
    id(top, "top");
    view_t topv = aabb;
    topv.x = topv.y = 0;
    if (topv.width > topv.height) { // landscape
        topv.height = nominal_size * topv.height/topv.width;
        topv.width = nominal_size;
    }
    else {                      // portrait
        topv.width = nominal_size * topv.width/topv.height;
        topv.height = nominal_size;
    }
    viewport(top, topv);
    viewbox(top, aabb1);

    // coarse
    xml_t s1a;
    viewport(s1a, aabb2);
    viewbox(s1a, aabb);
    s1a.update({{"id","s1"}, {"class","coarse"}});
    auto s1 = svg(s1a);

    auto v1 = view("coarse");
    viewbox(v1, aabb2);


    // fine view, for zooms.  viewport diagonal from above
    xml_t s2a;
    viewport(s2a, aabb1);
    viewbox(s2a, aabb);
    s2a.update({{"id","s2"}, {"class","fine"}});
    auto s2 = svg(s2a);

    auto v2 = view("fine");
    viewbox(v2, aabb1);
    
    // Details to put in coarse or fine views

    auto sas = g_active_strips(geom, acts);
    id(sas, "sas");
    body(common).push_back(sas);
    auto saw = g_active_wires(geom, acts);
    id(saw, "saw");
    body(common).push_back(saw);
    auto sba = g_blob_areas(geom, blobs);
    id(sba, "sba");
    body(common).push_back(sba);
    auto sbc = g_blob_corners(geom, blobs);
    id(sbc,"sbc");
    body(common).push_back(sbc);

    // Coarse

    auto& s1b = body(s1);
    s1b.push_back(use(link(aa))); // active
    s1b.push_back(use(link(sas))); // strips
    s1b.push_back(use(link(sba))); // blobs

    // Fine 

    auto& s2b = body(s2);
    s2b.push_back(use(link(aa))); // active
    s2b.push_back(use(link(saw))); // wires
    s2b.push_back(use(link(sba))); // blobs
    s2b.push_back(use(link(sbc))); // corners

    // Top 
    auto& topb = body(top);

    topb.push_back(common);
    topb.push_back(v1);
    topb.push_back(v2);
    topb.push_back(anchor(link(v2), nullptr, s1));
    topb.push_back(anchor(link(v1), nullptr, s2));

    return top;

    // coarse
    // - use aa with x=0,y=0
    // - active strips
    // - blob areas with link to fine blob areas

    // fine
    // - use aa with x=100,y=100
    // - active wires
    // - blob areas with link to coarse 
    // - blob corners


}
svggpp::xml_t WireCell::RaySvg::svg_full(const Geom& geom, const RayGrid::activities_t& acts, const RayGrid::blobs_t& blobs)
{
    auto aa = svg_active_area(geom);
    id(aa, "aa");
    auto aabb = viewbox(aa);
    attrs(aa).erase("viewBox");
    auto common = defs(aa);
    
    const double nominal_size = 2048;

    // what we will return.
    auto top = svg(svg_header);
    id(top, "top");
    view_t topv = aabb;
    topv.x = topv.y = 0;
    if (topv.width > topv.height) { // landscape
        topv.height = nominal_size * topv.height/topv.width;
        topv.width = nominal_size;
    }
    else {                      // portrait
        topv.width = nominal_size * topv.width/topv.height;
        topv.height = nominal_size;
    }
    viewport(top, topv);
    viewbox(top, aabb);

    // fine view, for zooms.  viewport diagonal from above
    xml_t s2a;
    viewport(s2a, aabb);
    viewbox(s2a, aabb);
    s2a.update({{"id","s2"}, {"class","fine"}});
    auto s2 = svg(s2a);

    auto v2 = view("fine");
    viewbox(v2, aabb);
    
    // Details to put in coarse or fine views

    auto saw = g_active_wires(geom, acts);
    id(saw, "saw");
    body(common).push_back(saw);
    auto sba = g_blob_areas(geom, blobs);
    id(sba, "sba");
    body(common).push_back(sba);
    auto sbc = g_blob_corners(geom, blobs);
    id(sbc,"sbc");
    body(common).push_back(sbc);

    auto& s2b = body(s2);
    s2b.push_back(use(link(aa))); // active
    s2b.push_back(use(link(saw))); // wires
    s2b.push_back(use(link(sba))); // blobs
    s2b.push_back(use(link(sbc))); // corners

    // Top 
    auto& topb = body(top);

    topb.push_back(common);
    topb.push_back(v2);
    topb.push_back(anchor(link(v2), nullptr, s2));

    return top;

    // coarse
    // - use aa with x=0,y=0
    // - active strips
    // - blob areas with link to fine blob areas

    // fine
    // - use aa with x=100,y=100
    // - active wires
    // - blob areas with link to coarse 
    // - blob corners


}

svggpp::xml_t WireCell::RaySvg::svg_active_area(const Geom& geom)
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

    viewbox(attr, project(bb.bounds()));

    auto top = svg(attr, pgon);
    return top;
}


svggpp::xml_t WireCell::RaySvg::g_active_strips(const Geom& geom, const RayGrid::activities_t& acts)
{
    BoundingBox bb;

    auto top = group(css_class("activity strips"));
    auto& topb = body(top);

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
                css_class(format("plane%d", plane)));
            topb.push_back(spgon);
        }    
    }

    return top;
}


svggpp::xml_t WireCell::RaySvg::g_active_wires(const Geom& geom, const RayGrid::activities_t& acts)
{
    BoundingBox bb;

    auto top = group(css_class("activity wires"));
    auto& topb = body(top);

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
                // std::cerr << strip << " " << wip<< std::endl;
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
                    css_class(format("plane%d wip%d", plane, wip)));
                topb.push_back(spgon);
            }
        }
    }

    return top;
}

svggpp::xml_t WireCell::RaySvg::g_blob_corners(const Geom& geom, const RayGrid::blobs_t& blobs)
{
    BoundingBox bb;
    int count=0;
    auto top = group(css_class("blob_corners"));
    auto& topb = body(top);

    for (const auto& blob : blobs) {
        ++count;

        auto grp = group(css_class(format("blobnum%d", count)));

        for (const auto& [a,b] : blob.corners()) {
            auto pt = geom.coords.ray_crossing(a,b);
            bb(pt);
            auto cir = circle(project(pt)); // fixme: radius
            topb.push_back(cir);
        }
    }

    return top;
}


svggpp::xml_t WireCell::RaySvg::g_blob_areas(const Geom& geom, const RayGrid::blobs_t& blobs)
{
    int count=0;
    auto top = group(css_class("blob_areas"));
    auto& topb = body(top);

    for (const auto& blob : blobs) {
        ++count;

        BoundingBox bbb;
        std::vector<Point> pts;
        for (const auto& [a,b] : blob.corners()) {
            pts.push_back(geom.coords.ray_crossing(a,b));
        }

        bbb(pts.begin(), pts.end());
        bbb.pad_rel(0.1);

        std::string bname = format("blobnum%d", count);

        auto spgon = polygon(
            to_ring( pts ),
            css_class(bname));
        auto v = view(bname);
        viewbox(v, project(bbb.bounds()));
        auto a = anchor(link(v), {}, spgon);
        topb.push_back(a);
        topb.push_back(v);
    }

    return top;
}



