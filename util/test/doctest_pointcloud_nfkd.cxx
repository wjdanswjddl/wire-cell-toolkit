#include "WireCellUtil/NFKD.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Logging.h"

#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointCloudDisjoint.h"

#include "WireCellUtil/doctest.h"

#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::PointCloud;
using namespace WireCell;


TEST_CASE("nfkd dataset atomic")
{
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});
    std::vector<std::string> names = {"x","y","z"};
    
    using point_type = std::vector<double>;
    using coords_type = coordinates<point_type>;

    coords_type coords(ds.selection(names));
    
    
    using kdtree_type = NFKD::Tree<coords_type::iterator, NFKD::IndexStatic>;
    kdtree_type kd(names.size(), coords.begin(), coords.end());

    CHECK(kd.size() == 3);
    debug("nfkd: {} point calls", kd.point_calls());

    CHECK(kd.at(0)->at(0) == 1.0);
    CHECK(kd.at(0)->at(1) == 2.0);
    CHECK(kd.at(0)->at(2) == 1.0);

    CHECK(kd.at(1)->at(0) == 1.0);
    CHECK(kd.at(1)->at(1) == 1.0);
    CHECK(kd.at(1)->at(2) == 4.0);

    CHECK(kd.at(2)->at(0) == 1.0);
    CHECK(kd.at(2)->at(1) == 3.0);
    CHECK(kd.at(2)->at(2) == 1.0);

    CHECK_THROWS_AS(*kd.at(3), IndexError); // throws

    // Do three times and observe that calls to the point evaluation
    // method are repeated.
    for (size_t count = 0; count < 3; ++count)
    {
        debug("nfkd: radius query #{}", count);
        auto rad = kd.radius(0.01, {1.0, 2.0, 1.0});
        CHECK(rad.size() == 1);
        for (const auto& [it,dist] : rad) {
            debug("rad: #{} point=({},{},{}), dist={}, calls={}",
                  count, it->at(0), it->at(1), it->at(2), dist, kd.point_calls());
        }
    }
    for (size_t N = 1; N <= 3; ++N) {
        debug("nfkd: knn={} query", N);
        auto knn = kd.knn(N, {0,0,0});
        REQUIRE(knn.size() == N);
        for (const auto& [it,dist] : knn) {
            debug("knn: N={} point=({},{},{}), dist={}, calls={}",
                  N, it->at(0), it->at(1), it->at(2),
                  dist, kd.point_calls());
        }
    }

}
TEST_CASE("nfkd dataset disjoint")
{
    DisjointDataset dds;
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});
    dds.append(ds);
    REQUIRE(dds.size() == 3);
    dds.append(ds);
    REQUIRE(dds.size() == 6);

    std::vector<std::string> names = {"x","y","z"};
    using point_type = std::vector<double>;
    auto coords = dds.selection<point_type>(names);
    
    REQUIRE(std::distance(coords.begin(), coords.end()) == 6);

    using kdtree_type = NFKD::Tree<decltype(coords)::iterator, NFKD::IndexStatic>;
    kdtree_type kd(names.size(), coords.begin(), coords.end());
    
    for (size_t N = 1; N <= 6; ++N) {
        debug("nfkd: knn={} query", N);
        auto knn = kd.knn(N, {0,0,0});
        REQUIRE(knn.size() == N);
        for (const auto& [it,dist] : knn) {
            debug("knn: N={} point=({},{},{}), dist={}, calls={}",
                  N, it->at(0), it->at(1), it->at(2),
                  dist, kd.point_calls());
        }
    }
    {
        auto rad = kd.radius(0.01, {1.0, 2.0, 1.0});
        CHECK(rad.size() == 2);
        for (const auto& [it,dist] : rad) {
            debug("rad: point=({},{},{}), dist={}, calls={}",
                  it->at(0), it->at(1), it->at(2), dist, kd.point_calls());
        }
    }
}

#include <random>
#include <algorithm>

TEST_CASE("nfkd dataset big")
{
    std::random_device rnd_device;
    // Specify the engine and distribution.
    std::mt19937 mersenne_engine {rnd_device()};  // Generates random integers
    std::uniform_int_distribution<int> dist {1, 52};
    
    auto gen = [&dist, &mersenne_engine](){
                   return dist(mersenne_engine);
               };
    const size_t n = 1024;
    std::vector<double> x(n), y(n), z(n), q(n);
    generate(begin(x), end(x), gen);
    generate(begin(y), end(y), gen);
    generate(begin(z), end(z), gen);

    Dataset ds({
            {"x", Array(x)},
            {"y", Array(y)},
            {"z", Array(z)},
        });
    
    using point_type = std::vector<double>;
    using coords_type = coordinates<point_type>;

    coords_type coords(ds.selection(names));
    using kdtree_type = NFKD::Tree<coords_type::iterator, NFKD::IndexStatic>;
    std::vector<std::string> names = {"x","y","z"};
    kdtree_type kd(names.size(), coords.begin(), coords.end());

    CHECK(kd.size() == n);
}
