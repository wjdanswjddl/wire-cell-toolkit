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

    using vpoint_t = std::vector<double>;
    struct ci_range {
        using iterator = coordinate_iterator<vpoint_t>;
        iterator it;
        iterator begin() const { return it; }
        iterator end() const { return iterator(); }
    };
    using ci_ranges = std::vector<ci_range>;
    ci_ranges cis;

    std::vector<std::string> names = {"x","y","z"};
    for (const Dataset& ds : dds.values()) {
        auto sel = ds.selection(names);
        ci_range::iterator cit(sel);
        REQUIRE(cit.size() == 3);
        REQUIRE(cit.ndim() == 3);
        cis.push_back(ci_range{cit});
        REQUIRE(cis.back().begin().size() == 3);
        REQUIRE(cis.back().begin().ndim() == 3);
        debug("doctest_pointcloud_iterator: load a {}-selection of size {}",
              cit.ndim(), cit.size());
    }

    for (const auto& cir : cis) {
        for (const auto& v : cir) {
            REQUIRE(v.size() == 3);
        }
    }

    {
        auto cir = cis.front();
        REQUIRE(cir.it.size() == 3);
    }

    
    using djit_t = disjoint_iterator<ci_ranges::iterator, ci_range::iterator, vpoint_t>;
    djit_t djit(cis.begin(), cis.end());
    // for (const auto& v : djit) {
    for (djit_t it = djit; it != djit_t(); ++it) {
        REQUIRE(it.size() == 3);

        const auto& v = *it;
        REQUIRE(v.size() == 3);
        debug("x={} y={} z={}", v[0], v[1], v[2]);
    }
    // for (const auto& pt : djit) {
    //     debug("x={} y={} z={}", pt.x(), pt.y(), pt.z());
    // }


    // See similar test narrowed to a singular data set in
    // doctest_pointcloud_iterator.cxx
    // auto points = dds.selection<std::vector<double>>({"x","y","z"});
    // CHECK(points.size() == 6);


    // auto djit = points.begin();
    
    // const auto& v = (*djit);
    // CHECK(v.at(0) == 1.0);
    // CHECK(v.at(1) == 2.0);
    // CHECK(v.at(2) == 1.0);

    // ++djit;

    // CHECK(djit->at(0) == 1.0);
    // CHECK(djit->at(1) == 1.0);
    // CHECK(djit->at(2) == 4.0);

    // ++djit;

    // CHECK(djit->at(0) == 1.0);
    // CHECK(djit->at(1) == 3.0);
    // CHECK(djit->at(2) == 1.0);

    // ++djit;                     // at start of 2nd copy

    // CHECK(djit->at(0) == 1.0);
    // CHECK(djit->at(1) == 2.0);
    // CHECK(djit->at(2) == 1.0);

    // ++djit;

    // CHECK(djit->at(0) == 1.0);
    // CHECK(djit->at(1) == 1.0);
    // CHECK(djit->at(2) == 4.0);

    // ++djit;

    // CHECK(djit->at(0) == 1.0);
    // CHECK(djit->at(1) == 3.0);
    // CHECK(djit->at(2) == 1.0);

    // ++djit;                     // at end
}

