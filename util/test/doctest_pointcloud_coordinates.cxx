#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/doctest.h"


#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::PointCloud;

TEST_CASE("point cloud coordinates")
{
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});
    auto sel = ds.selection({"x","y","z"});

    debug("doctest_pointcloud_iterator: make iterators");
    // both a container-like and an iterator
    coordinates<std::vector<double>> coords(sel);
    CHECK(coords.size() == 3); 
    CHECK(coords.ndim() == 3); 

    auto beg = coords.begin();
    auto end = coords.end();

    CHECK(coords == beg);       // the container is itself an iterator
    CHECK(coords != end);

    REQUIRE(std::distance(beg,end) == 3);
    CHECK(std::distance(beg,beg) == 0);
    CHECK(std::distance(end,end) == 0);

    debug("doctest_pointcloud_iterator: copy iterator");
    coordinates<std::vector<double>> coords2 = coords;

    CHECK(coords2.size() == 3); 
    CHECK(coords2.ndim() == 3); 

    // operator[] returns convertable-to-value (NOT reference)
    // https://www.boost.org/doc/libs/1_83_0/libs/iterator/doc/facade-and-adaptor.html#operator
    std::vector<double> vec = coords[0];
    CHECK(vec[0] == 1.0);
    CHECK(vec[1] == 2.0);
    CHECK(vec[2] == 1.0);
    
    /// Compiles but holds garbage.
    // const std::vector<double>& cvecref = coords[0];
    // CHECK(cvecref[0] == 1.0);
    // CHECK(cvecref[1] == 2.0);
    // CHECK(cvecref[2] == 1.0);

    /// does not compile as a operator_brackets_proxy is preturned
    // auto avec = coords[0];
    // CHECK(avec[0] == 1.0);
    // CHECK(avec[1] == 2.0);
    // CHECK(avec[2] == 1.0);

    debug("doctest_pointcloud_iterator: iterate");

    auto cit = coords.begin();
    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 2.0);
    CHECK((*cit)[2] == 1.0);

    ++cit;                        // column 2

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 1.0);
    CHECK((*cit)[2] == 4.0);

    ++cit;                        // column 3

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 3.0);
    CHECK((*cit)[2] == 1.0);

    ++cit;                        // end

    CHECK (cit == end);

    --cit;                      // back to column 3

    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 3.0);
    CHECK((*cit)[2] == 1.0);

    cit -= 2;                   // back to start
    CHECK((*cit)[0] == 1.0);
    CHECK((*cit)[1] == 2.0);
    CHECK((*cit)[2] == 1.0);

    for (auto p : coordinates<Point>(sel, Point())) {
        debug("x={} y={} z={}", p.x(), p.y(), p.z());
    }
}
