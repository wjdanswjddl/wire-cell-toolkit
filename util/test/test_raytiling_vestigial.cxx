/* https://github.com/WireCell/wire-cell-toolkit/issues/196
 */

#include "WireCellUtil/RayClustering.h"
#include "WireCellUtil/RayTiling.h"
#include <WireCellUtil/DetectorWires.h>
#include <WireCellUtil/Testing.h>
#include <WireCellUtil/RaySvg.h>
#include <WireCellUtil/Exceptions.h>
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
        AssertMsg(got == nwires[ind], "wrong number of wires in plane");
    }
    DetectorWires::flat_channels(storedb);

    WireSchema::Store store(std::make_shared<WireSchema::StoreDB>(storedb));

    std::vector<std::vector<WireSchema::Wire>> wireobjs(3);
    std::vector<Vector> pitches(3);
    for (auto plane : store.planes()) {
        wireobjs[plane.ident] = store.wires(plane);
        pitches[plane.ident] = store.mean_pitch(plane);
    }

    std::string fname = argv[0];
    fname += ".json.bz2";
    WireSchema::dump(fname.c_str(), store);

    auto raypairs = WireSchema::ray_pairs(store, store.faces()[0]);
    // const int nlayers = raypairs.size();

    RayGrid::Coordinates coords(raypairs);
    const double hit=1.0;
    std::vector<std::pair<int,int>> tries = {
        {1,1727},               // single blob
        {3,1727},               // 3 vestigial
    };

    auto span_size = [](const auto& s) -> int {
        int siz = s.bounds.second-s.bounds.first;
        std::cerr << "size="<<siz<<" [" << s.bounds.first << "," << s.bounds.second << "]\n";
        return siz;
    };

    auto doit = [&](int span, int iwire) -> RayGrid::blobs_t {
        RayGrid::activities_t activities;
        activities.emplace_back(0, 1, hit);
        activities.emplace_back(1, 1, hit);
        activities.emplace_back(2, span+1, hit, nwires[0]/2);
        activities.emplace_back(3, span+2, hit, nwires[1]/2);
        activities.emplace_back(4, span, hit, iwire);

        auto blobs = RayGrid::make_blobs(coords, activities);

        RaySvg::Scene scene(coords, store);
        scene(blobs);
        scene(activities);
        std::stringstream ss;
        ss << argv[0] << "-s" << span << "-w" << iwire << ".svg";
        scene.blob_view(ss.str());
        std::cerr << ss.str() << "\n";

        for (const auto& blob : blobs) {
            for (const auto& strip : blob.strips()) {
                int size = span_size(strip);
                std::cerr << strip.layer << " " << size << "\n";
            }
        }
        return blobs;
    };

    // vestigial wires should produce 3 corners and 2 strips per plane
    {                          
        auto blobs = doit(3, 1727);

        if (blobs.size() != 1) {
            THROW(ValueError() << errmsg{"Unexpected number of blobs"});
        }
        const auto& blob = blobs[0];
        if (blob.corners().size() != 3) {
            THROW(ValueError() << errmsg{"Unexpected number of blob corners"});
        }
        const auto& strips = blob.strips();
        if (strips.size() != 5) {
            THROW(ValueError() << errmsg{"Unexpected number of blob strips"});
        }
        for (const auto& strip : strips) {
            int size = span_size(strip);
            std::cerr << strip.layer << " " << size << "\n";
            if (strip.layer <=1 ) {
                AssertMsg(size == 1, "Active area strip wrong size");
            }
            else {
                AssertMsg(size == 2, "Active wire strip wrong size");
            }
        }
    }

    // single-corner blob

    {
        auto blobs = doit(1, 1727);
        if (blobs.size()) {
            THROW(ValueError() << errmsg{"No blobs expected with single-corner blob"});
        }
    }
    {
        auto blobs = doit(3, 1725);
        if (blobs.size()) {
            THROW(ValueError() << errmsg{"No blobs expected with single-corner blob"});
        }
    }

    return 0;
}
