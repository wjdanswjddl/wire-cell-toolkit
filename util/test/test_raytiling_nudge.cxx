#include "WireCellUtil/RayClustering.h"
#include "WireCellUtil/RayTiling.h"
#include <WireCellUtil/DetectorWires.h>
#include <WireCellUtil/Testing.h>
#include <WireCellUtil/RaySvg.h>
#include <vector>
#include <iostream>

using namespace WireCell;

int main(int argc, char* argv[])
{
    WireSchema::StoreDB storedb;
    std::vector<size_t> nwires = {2400, 2400, 3456};
    std::vector<size_t> wire_offset = {0, 2400, 4800};
    // auto wo = [&](size_t l) -> size_t { return wire_offset[l-2]; };

    for (size_t ind=0; ind<3; ++ind) {
        auto& plane = get_append(storedb, ind, 0, 0, 0);
        size_t got = DetectorWires::plane(storedb, plane, DetectorWires::uboone[ind]);
        std::cerr << ind << ": " << nwires[ind] << "\n";
        Assert(got == nwires[ind]);
    }
    DetectorWires::flat_channels(storedb);

    WireSchema::Store store(std::make_shared<WireSchema::StoreDB>(storedb));

    std::vector<std::vector<WireSchema::Wire>> wireobjs(3);
    std::vector<Vector> pitches(3);
    for (auto plane : store.planes()) {
        wireobjs[plane.ident] = store.wires(plane);
        pitches[plane.ident] = store.mean_pitch(plane);
    }

    // const double corner_radius = 0.5; // svg circle for corners
    // std::vector<std::string> colors = {
    //     "black", "black", "red", "green", "blue",
    // };
    // std::vector<svg::Color> svgcolors = {
    //     svg::Color::White,
    //     svg::Color::White,
    //     svg::Color::Red,
    //     svg::Color::Green,
    //     svg::Color::Blue,
    // };


    // using svgshape = std::unique_ptr<svg::Shape>;

    // auto wbox = [&](int layer, int grid) -> svgshape {
    //     int wind = layer-2;
    //     const auto& phalf = 0.4*pitches[wind];
    //     const auto& w = wireobjs[wind][grid];

    //     const auto fillcolor = svgcolors[layer];
    //     const auto linecolor = svg::Color::White;

    //     svg::Fill fill(fillcolor);
    //     auto pg = std::make_unique<svg::Polygon>(fill, svg::Stroke(0.15, linecolor));
    //     (*pg) << RaySvg::point(w.tail-phalf);
    //     (*pg) << RaySvg::point(w.tail+phalf);
    //     (*pg) << RaySvg::point(w.head+phalf);
    //     (*pg) << RaySvg::point(w.head-phalf);
    //     (*pg) << RaySvg::point(w.tail-phalf);
    //     return pg;
    // };
    // auto wline = [&](int layer, int grid) -> svgshape {
    //     int wind = layer-2;
    //     const auto& phalf = 0.4*pitches[wind];
    //     const auto& w = wireobjs[wind][grid];
    //     const auto fillcolor = svgcolors[layer];
    //     return std::make_unique<svg::Line>(RaySvg::point(w.tail), RaySvg::point(w.head),
    //                                        svg::Stroke(2.5, fillcolor));
    // };

    std::string fname = argv[0];
    fname += ".json.bz2";
    WireSchema::dump(fname.c_str(), store);

    auto raypairs = WireSchema::ray_pairs(store, store.faces()[0]);
    // const int nlayers = raypairs.size();

    RayGrid::Coordinates coords(raypairs);
    const double nudge = 0.0;

    const double hit=1.0;
    for (size_t span=1; span<10; span +=2 ) 
    {
        RayGrid::activities_t activities;
        activities.emplace_back(0, 1, hit);
        activities.emplace_back(1, 1, hit);
        activities.emplace_back(2, span+1, hit, nwires[0]/2);
        activities.emplace_back(3, span+2, hit, nwires[1]/2);
        activities.emplace_back(4, span, hit, nwires[2]/2);
        // we overwrite this last in each step of the following scan

        for (size_t iwire=0; iwire<nwires[2]-span; ++iwire) {
            activities[4] = RayGrid::Activity(4, span, hit, iwire);
            auto blobs = RayGrid::make_blobs(coords, activities, nudge);

            if (blobs.empty()) {
                continue;
            }
            std::cerr << iwire << ": " << blobs.size() << "\n";

            RaySvg::Scene scene(coords, store);

            scene(blobs);
            scene(activities);

            std::stringstream ss;
            ss << argv[0] << "-s" << span << "-w" << iwire << ".svg";
            scene.blob_view(ss.str());
            std::cerr << ss.str() << "\n";
            // std::vector<svgshape> svgwires;
            // std::vector<Point> corners;


            // for (const auto& blob : blobs) {
            //     std::cerr << "\t" << blob.as_string() << "\n";

            //     std::stringstream swires;
            //     std::string comma = "";
            //     for (const auto& strip : blob.strips()) {
            //         if (strip.layer <= 1) continue;

            //         for (auto wip = strip.bounds.first; wip < strip.bounds.second; ++wip) {
            //             svgwires.emplace_back(wline(strip.layer, wip));
            //         }

            //         swires << comma
            //                << strip.bounds.first + wo(strip.layer)
            //                << ":"
            //                << strip.bounds.second + wo(strip.layer)
            //                << ":1:red";
            //             // << colors[strip.layer];
            //         comma = ", ";
            //     }
            //     for (const auto& [a,b] : blob.corners()) {
            //         const auto c = coords.ray_crossing(a,b);
            //         bb(c);
            //         corners.push_back(c);

            //         if (a.layer >= 2) {
            //             swires << comma
            //                    << a.grid + wo(a.layer)
            //                    << ":"
            //                    << a.grid + wo(a.layer) + 1
            //                    << ":1:green";
            //             comma = ", ";
            //         }

            //         if (b.layer >= 2) {
            //             swires << comma
            //                    << b.grid + wo(b.layer)
            //                    << ":"
            //                    << b.grid + wo(b.layer) + 1
            //                    << ":1:green";
            //             comma = ", ";
            //         }
            //     }
            //     std::cerr << swires.str() << "\n";
            // } // blobs

            // Vector pad(10,10,10);
            // bb(bb.bounds().first - pad);
            // bb(bb.bounds().second + pad);

            // std::stringstream ss;
            // ss << argv[0] << "-" << iwire << ".svg";
            // auto doc = RaySvg::document(ss.str(), bb.bounds());
            // for (const auto& svgwire : svgwires) {
            //     doc << *svgwire;
            // }
            // for (const auto& pt : corners) {
            //     doc << RaySvg::circle(pt, corner_radius);
            // }
            // doc.save();
            // std::cerr << ss.str() << "\n";
        }

    }
    return 0;
}

