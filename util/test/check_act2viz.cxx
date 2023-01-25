#include "WireCellUtil/String.h"
#include "WireCellUtil/Testing.h"
#include <WireCellUtil/DetectorWires.h>
#include <WireCellUtil/RaySvg.h>

#include <fstream>
#include <iostream>

using namespace WireCell;
using namespace svggpp;
using namespace WireCell::String;

void usage()
{
    std::cerr << "Load text dump of activities, do tiling, make SVG\n"
              << "usage:\n\tcheck_act2viz <act.txt> [<out.svg>]\n";
    exit (-1);
}

int main(int argc, char* argv[])
{
    const double nudge=1e-3;

    if (argc < 2 or argc > 3) { usage(); }

    std::string iname = argv[1];
    std::ifstream fstr(iname);
    if (!fstr) usage();
    
    //
    // MicroBooNE'ish
    //

    WireSchema::StoreDB storedb;
    const std::vector<size_t> nwires = {2400, 2400, 3456};
    const std::vector<int> nrays = {2, 2, 2400, 2400, 3456};
    std::vector<RaySvg::wire_vector_t> plane_wires;

    for (size_t ind=0; ind<3; ++ind) {
        auto& plane = get_append(storedb, ind, 0, 0, 0);

        size_t ini = storedb.wires.size();
        size_t got = DetectorWires::plane(storedb, plane, DetectorWires::uboone[ind]);
        AssertMsg(got == nwires[ind], "wrong number of wires in plane");

        // std::cerr << "add wires: " << ini << " + " << got << "\n";
        auto beg = storedb.wires.begin()+ini;
        plane_wires.push_back(RaySvg::wire_vector_t(beg, beg+got));
    }
    DetectorWires::flat_channels(storedb);
    WireSchema::Store store(std::make_shared<WireSchema::StoreDB>(storedb));
    auto raypairs = WireSchema::ray_pairs(store, store.faces()[0]);

    RaySvg::Geom geom{RayGrid::Coordinates(raypairs), plane_wires};

    std::vector<std::vector<RayGrid::Activity::value_t>> measures(5);
    measures[0].push_back(1);
    measures[1].push_back(1);

    // Parse file into mesures/activities
    while (fstr) {
        std::string line;
        std::getline(fstr, line);
        auto chunks = split(line, " ");

        if (chunks.empty()) continue;
        if (!endswith(chunks[0], "strip")) continue;

        int level = atoi(chunks[1].substr(1).c_str());
        if (level < 0 or level >= 5) {
            std::cerr << "bogus level! line:\n" << line;
            return -1;
        }
        if (level < 2) { continue; }

        auto pp = split(chunks[2].substr(6,chunks.size() - 8), ",");
        int pmin = std::max(0,              atoi(pp[0].c_str()));
        int pmax = std::min(nrays[level]-1, atoi(pp[1].c_str()));

        auto& m = measures[level];
        if (m.size() < (size_t)pmax) {
            m.resize(pmax, 0);
        }
        // A strip's upper most bound is not "in" the strip activities
        for (int pind=pmin; pind<pmax; ++pind) {
            m[pind] += 1;
        }
        // std::cerr << "level=" << level << " pind=["<<pmin<<","<<pmax<<"]\n";
    }
    RayGrid::activities_t activities;
    for (int layer=0; layer<5; ++layer) {
        const auto& m = measures[layer];
        activities.push_back(RayGrid::Activity(layer, {m.begin(), m.end()}));
    }

    auto blobs = RayGrid::make_blobs(geom.coords, activities, nudge);

    auto top = RaySvg::svg_full(geom, activities, blobs);

    std::string svgname;
    if (argc == 3) {
        svgname = argv[2];
    }
    else {
        svgname = iname + ".svg";
    }
    std::ofstream ofstr(svgname);
    ofstr << svggpp::dumps(top);

    return 0;
}
