#include "WireCellUtil/NFKD.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Logging.h"

#include "WireCellUtil/doctest.h"

#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::NFKD;

TEST_CASE("nfkd dynamic")
{
    const size_t dimension = 3;
    using points_t = std::vector<Point>;
    points_t points = {
        // x    y    z
        {1.0, 2.0, 1.0},
        {1.0, 1.0, 4.0},
        {1.0, 3.0, 1.0}
    };
        
    debug("nfkd: making k-d tree");
    Tree<points_t::iterator> kd(dimension, points.begin(), points.end());

    CHECK(kd.size() == 3);
    debug("nfkd: {} point calls", kd.point_calls());

    CHECK(*kd.at(0) == points[0]);
    CHECK(*kd.at(1) == points[1]);
    CHECK(*kd.at(2) == points[2]);
    CHECK_THROWS_AS(*kd.at(3) == points[2], IndexError); // throws

    for (size_t count = 0; count < 3; ++count)
    {
        debug("nfkd: radius query #{}", count);
        auto rad = kd.radius(0.01, Point{1.0, 2.0, 1.0});
        CHECK(rad.size() == 1);
        for (const auto& [it,dist] : rad) {
            const Point& pt = *it;
            debug("rad: #{} point=({},{},{}), dist={}, calls={}",
                  count, pt.x(), pt.y(), pt.z(), dist, kd.point_calls());
        }
    }

    debug("nfkd: appending");
    kd.append(points.begin(), points.end());
    CHECK(kd.size() == 6);

    CHECK(*kd.at(0) == points[0]);
    CHECK(*kd.at(1) == points[1]);
    CHECK(*kd.at(2) == points[2]);
    CHECK(*kd.at(3) == points[0]);
    CHECK(*kd.at(4) == points[1]);
    CHECK(*kd.at(5) == points[2]);

    for (size_t N = 1; N <= 6; ++N) {
        debug("nfkd: knn={} query", N);
        auto knn = kd.knn(N, Point{0,0,0});
        CHECK(knn.size() == N);
        for (const auto& [it,dist] : knn) {
            const Point& pt = *it;
            debug("knn: N={} point=({},{},{}), dist={}, calls={}",
                  N, pt.x(), pt.y(), pt.z(), dist, kd.point_calls());
        }
    }

}
TEST_CASE("nfkd static")
{
    const size_t dimension = 3;
    using points_t = std::vector<Point>;
    points_t points = {
        // x    y    z
        {1.0, 2.0, 1.0},
        {1.0, 1.0, 4.0},
        {1.0, 3.0, 1.0}
    };
        
    debug("nfkd: making k-d tree");
    Tree<points_t::iterator, DistanceL2Simple, IndexStatic> kd(dimension, points.begin(), points.end());

    CHECK(kd.size() == 3);
    debug("nfkd: {} point calls", kd.point_calls());

    CHECK(*kd.at(0) == points[0]);
    CHECK(*kd.at(1) == points[1]);
    CHECK(*kd.at(2) == points[2]);
    CHECK_THROWS_AS(*kd.at(3) == points[2], IndexError); // throws

    for (size_t count = 0; count < 3; ++count)
    {
        debug("nfkd: radius query #{}", count);
        auto rad = kd.radius(0.01, Point{1.0, 2.0, 1.0});
        CHECK(rad.size() == 1);
        for (const auto& [it,dist] : rad) {
            const Point& pt = *it;
            debug("rad: #{} point=({},{},{}), dist={}, calls={}",
                  count, pt.x(), pt.y(), pt.z(), dist, kd.point_calls());
        }
    }

    for (size_t N = 1; N <= 3; ++N) {
        debug("nfkd: knn={} query", N);
        auto knn = kd.knn(N, Point{0,0,0});
        CHECK(knn.size() == N);
        for (const auto& [it,dist] : knn) {
            const Point& pt = *it;
            debug("knn: N={} point=({},{},{}), dist={}, calls={}",
                  N, pt.x(), pt.y(), pt.z(), dist, kd.point_calls());
        }
    }

}
