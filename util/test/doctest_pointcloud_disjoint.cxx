#include "WireCellUtil/PointCloudDisjoint.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Exceptions.h"
using namespace WireCell;
using namespace WireCell::PointCloud;
using spdlog::debug;

TEST_CASE("point cloud disjoint dataset")
{
    using dsindex_t = DisjointDataset::address_t;

    DisjointDataset dds;
    CHECK(dds.size() == 0);

    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});

    dds.append(ds);
    debug("dds append 1: has {} datasets and {} points",
          dds.values().size(), dds.size());

    CHECK(dds.values().size() == 1);
    CHECK(dds.size() == 3);
    CHECK(dds.address(0) == dsindex_t(0,0));
    CHECK(dds.address(1) == dsindex_t(0,1));
    CHECK(dds.address(2) == dsindex_t(0,2));
    CHECK_THROWS_AS(dds.address(3), IndexError);

    dds.append(ds);
    debug("dds append 2: has {} datasets and {} points",
          dds.values().size(), dds.size());

    CHECK(dds.values().size() == 2);
    CHECK(dds.size() == 6);
    CHECK(dds.address(3) == dsindex_t(1,0));
    CHECK(dds.address(4) == dsindex_t(1,1));
    CHECK(dds.address(5) == dsindex_t(1,2));
    CHECK_THROWS_AS(dds.address(6), IndexError);

    using point_type = std::vector<double>;
    using coordinates_type = coordinates<point_type>;
    using coordinates_vector = std::vector<coordinates_type>;
    coordinates_vector cov;

    std::vector<std::string> names = {"x","y","z"};
    for (const Dataset& ds : dds.values()) {
        auto sel = ds.selection(names);
        coordinates_type coords(sel);
        REQUIRE(coords.size() == 3);
        REQUIRE(coords.ndim() == 3);
        cov.push_back(coords);
        REQUIRE(cov.back().begin().size() == 3);
        REQUIRE(cov.back().begin().ndim() == 3);
    }

    for (const auto& coords : cov) {
        for (const auto& vec : coords) {
            REQUIRE(vec.size() == 3);
        }
    }

    {
        auto coords = cov.front();
        REQUIRE(coords.size() == 3);
    }

    {
        disjoint_selection<point_type> djs(dds, names);
        REQUIRE(std::distance(djs.begin(), djs.end()) == 6);

        for (auto pt : djs) {
            debug("x={} y={} z={}", pt[0], pt[1], pt[2]);
        }

        auto djit = djs.begin();
        REQUIRE(djit != djs.end());

        point_type v;
        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 2.0);
        CHECK(v.at(2) == 1.0);

        ++djit;

        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 1.0);
        CHECK(v.at(2) == 4.0);

        ++djit;

        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 3.0);
        CHECK(v.at(2) == 1.0);

        ++djit;

        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 2.0);
        CHECK(v.at(2) == 1.0);

        ++djit;

        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 1.0);
        CHECK(v.at(2) == 4.0);

        ++djit;

        v = (*djit);
        CHECK(v.at(0) == 1.0);
        CHECK(v.at(1) == 3.0);
        CHECK(v.at(2) == 1.0);

        ++djit;                 // end

        CHECK(djit == djs.end());
    }

}

