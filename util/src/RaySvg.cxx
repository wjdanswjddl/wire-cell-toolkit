#include "WireCellUtil/RaySvg.h"

#include <boost/range/irange.hpp>

auto irange = [](const std::pair<int,int>& p) -> auto {
    return boost::irange(p.first, p.second);
};

using namespace WireCell;
using namespace WireCell::RaySvg;


svg::Document RaySvg::document(const std::string& svgname, const Ray& bounds, double scale)
{
    const auto rel = ray_vector(bounds);

    double real_width = x(rel);
    double real_height = y(rel);

    double width = real_width*scale;
    double height = real_height*scale;


    // const svg::Dimensions dims(x(rel), y(rel));
    const svg::Dimensions dims(width, height);
    const svg::Point origin_offset(-x(bounds.first), -y(bounds.first));
    svg::Layout layout(dims, svg::Layout::TopLeft, scale, origin_offset);
    return svg::Document(svgname, layout);
}

Scene::Scene(const RayGrid::Coordinates& coords,
             const WireSchema::Store& wires_store)
    : m_coords(coords)
    , m_wires(wires_store)
{
    for (auto plane : wires_store.planes()) {
        m_wireobjs.push_back(wires_store.wires(plane));
        m_pvecs.push_back(wires_store.mean_pitch(plane));
    }
}

void Scene::operator()(const RayGrid::activities_t& acts)
{
    for (const auto& act : acts) {
        m_activities.push_back(act);
        for (const RayGrid::Strip& strip : act.make_strips()) {
            m_astrips.push_back(strip);
            for (auto grid : irange(strip.bounds)) {
                int pind = strip.layer-2;
                if (pind < 0) continue;
                const auto& w = m_wireobjs[pind][grid];
                Ray r(w.tail, w.head);
                m_astrips_bb(r);     // fixme, use region
            }
        }
    }
}
void Scene::operator()(const RayGrid::blobs_t& blobs)
{
    for (const auto& blob : blobs) {
        m_blobs.push_back(blob);
        for (const auto& [a,b] : blob.corners()) {
            const auto c = m_coords.ray_crossing(a,b);
            m_blobs_bb(c);
        }
        for (const auto& strip : blob.strips()) {
            m_bstrips.push_back(strip);
            for (auto grid : irange(strip.bounds)) {
                int pind = strip.layer-2;
                if (pind < 0) continue;
                const auto& w = m_wireobjs[pind][grid];
                Ray r(w.tail, w.head);
                m_bstrips_bb(r);     // fixme, use region
            }
        }
    }
}

Scene::shape_p Scene::strip_shape(int layer, int grid, bool inblob)
{
    int pind = layer-2;
    if (pind < 0) return nullptr;
    const auto& phalf = 0.4*m_pvecs[pind];
    const auto& w = m_wireobjs[pind][grid];

    svg::Fill fill;
    if (inblob) {
        fill = svg::Fill(m_layer_colors[layer]);        
    }
    else {
        fill = svg::Fill(m_layer_colors[layer], 0.5);
    }
    
    auto pg = std::make_unique<svg::Polygon>(fill, svg::Stroke(0.15, m_outline_color));
    (*pg) << RaySvg::point(w.tail-phalf);
    (*pg) << RaySvg::point(w.tail+phalf);
    (*pg) << RaySvg::point(w.head+phalf);
    (*pg) << RaySvg::point(w.head-phalf);
    (*pg) << RaySvg::point(w.tail-phalf);
    return pg;
};
Scene::shape_p Scene::corner_shape(const RayGrid::crossing_t& c)
{
    const auto [a,b] = c;
    const auto cvec = m_coords.ray_crossing(a,b);

    return std::make_unique<svg::Circle>(
        RaySvg::point(cvec), m_corner_radius,
        svg::Fill(m_corner_color),
        svg::Stroke(m_thin_line, m_outline_color));
}
                                             
void RaySvg::Scene::blob_view(const std::string& svgname)
{
    auto bb = m_blobs_bb;
    double pad = 10;
    const Vector vpad(pad,pad,pad);
    bb(bb.bounds().first - vpad);
    bb(bb.bounds().second + vpad);
    auto doc = RaySvg::document(svgname, bb.bounds(), m_scale);

    bool coalesce_strips = false; // fixme: make config
    bool inblob=false;

    for (const auto& strip : m_astrips) {
        if (coalesce_strips) {
            // fixme: provide a strip()->shape_p
        }
        else {
            for (auto grid : irange(strip.bounds)) {
                if (strip.layer<=1) continue;
                auto s = strip_shape(strip.layer, grid, inblob);
                if (!s) continue;
                doc << *s;
            }
        }
    }

    inblob=true;
    for (const auto& strip : m_bstrips) {
        if (coalesce_strips) {
            // fixme: provide a strip()->shape_p
        }
        else {
            for (auto grid : irange(strip.bounds)) {
                if (strip.layer<=1) continue;
                auto s = strip_shape(strip.layer, grid, inblob);
                if (!s) continue;
                doc << *s;
            }
        }
    }
    for (const auto& blob : m_blobs) {
        for (const auto& c : blob.corners()) {
            auto s = corner_shape(c);
            if (!s) continue;
            doc << *s;
        }
    }
    doc.save();
}
void RaySvg::Scene::wire_view(const std::string& svgname)
{
}

