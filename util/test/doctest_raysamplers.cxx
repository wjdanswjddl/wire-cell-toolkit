#include "WireCellUtil/RaySamplers.h"
#include "WireCellUtil/RayHelpers.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/doctest.h"

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::RayGrid;

TEST_CASE("sample blobs")
{
    auto raypairs = symmetric_raypairs();
    CHECK(! raypairs.empty());
    Coordinates coords(raypairs);

    std::vector<Point> pts = {
        Point(0, 100*units::mm, 100*units::mm),
        Point(0, 101*units::mm, 100*units::mm),
        Point(0, 100*units::mm, 101*units::mm),
        Point(0, 101*units::mm, 101*units::mm),
    };
    auto meas = make_measures(coords, pts);
    CHECK(! meas.empty());

    auto acts = make_activities(coords, meas);
    CHECK(! acts.empty());

    auto blobs = make_blobs(coords, acts);
    CHECK(! blobs.empty());
    CHECK(blobs.size() == 1);


    spdlog::debug("checking blobs");    
    const auto& blob = blobs[0];
    spdlog::debug(blob);
    for (const auto& strip : blob.strips()) {
        spdlog::debug(strip);
    }

    spdlog::debug("making BS");    
    BlobSamplers bs(coords);

    std::vector<std::string> strategies = {
        "center", "corners", "edges", "stepped"
    };
    for (const auto& strategy : strategies) {
        spdlog::debug("strategy {}", strategy);
        auto pts = bs.sample(blob, strategy);
        //CHECK(! pts.empty());
        for (const auto& pt : pts) {
            spdlog::debug("{}", pt/units::cm);
        }
    }
}

