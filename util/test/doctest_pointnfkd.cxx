#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/doctest.h"

#include <unordered_map>

using namespace WireCell;
using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
using namespace WireCell::KDTree;
using spdlog::debug;

TEST_CASE("point nfkd empty")
{
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});

    DisjointDataset dds;
    CHECK(dds.values().size() == 0);
    CHECK(dds.size() == 0);

    name_list_t coords = {"x","y","z"};

    NFKDT<double, DistanceL2Simple, IndexDynamic> nfkd(dds, coords);

    CHECK(&dds == &nfkd.pointclouds());

    // part of the nanoflann API
    CHECK(nfkd.kdtree_get_point_count() == 0);
    CHECK(&dds == &nfkd.pointclouds());

    // Should not crash nor do something stupid.
    nfkd.addpoints(0,0);
    CHECK(nfkd.kdtree_get_point_count() == 0);
    CHECK(&dds == &nfkd.pointclouds());

    dds.append(ds);
    CHECK(&dds == &nfkd.pointclouds());
    CHECK(dds.values().size() == 1);
    CHECK(dds.size() == 3);
    CHECK(&dds == &nfkd.pointclouds());

    debug("doctest_pointnfkd: addpoints: {}", (void*)&dds);
    nfkd.addpoints(0,3);
    debug("doctest_pointnfkd: addpoints done.");

    CHECK(&dds == &nfkd.pointclouds());

    CHECK(nfkd.kdtree_get_point_count() == 3);

    {
        debug("doctest_pointnfkd: calling knn(3).");
        auto knn = nfkd.knn(3, {0,0,0});
        CHECK(knn.size() == 3);
    }

    {
        debug("doctest_pointnfkd: calling knn(2).");
        auto knn = nfkd.knn(2, {0,0,0});
        CHECK(knn.size() == 2);
    }

    {
        debug("doctest_pointnfkd: calling radius.");
        double rad = 0.1;
        auto res = nfkd.radius(rad*rad, {1.,2.,1.});
        CHECK(res.size() == 1);
        CHECK(res.begin()->first == 0);
        CHECK(res.begin()->second == 0.0);
    }
}
TEST_CASE("point nfkd static is static")
{
    DisjointDataset dds;
    name_list_t coords = {"x","y","z"};
    NFKDT<double, DistanceL2Simple, IndexStatic> nfkd(dds, coords);
    Dataset ds({{"x", Array({1.0, 1.0, 1.0})},
                {"y", Array({2.0, 1.0, 3.0})},
                {"z", Array({1.0, 4.0, 1.0})},
                {"one", Array({1  ,2  ,3  })},
                {"two", Array({1.1,2.2,3.3})}});
    dds.append(ds);
    CHECK_THROWS_AS(nfkd.addpoints(0,3), LogicError);
}
