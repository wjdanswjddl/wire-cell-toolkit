#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Type.h"
#include "WireCellUtil/doctest.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range.hpp>

#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::PointCloud;

TEST_CASE("point cloud coordinate iterator")
{
    using point_type = coordinate_point<double>;
    using cid = coordinate_iterator<point_type>;
    Dataset::selection_t empty;
    cid c1(&empty, 0);
    cid c2 = c1;
    cid c3(c2);
    cid c4(std::move(c3));
    c1 = c4;
    c1 = std::move(c4);
}

TEST_CASE("point cloud coordinate range")
{
    Dataset ds({
            {"x", Array({1.0, 1.0, 1.0})},
            {"y", Array({2.0, 1.0, 3.0})},
            {"z", Array({1.0, 4.0, 1.0})},
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})}});
    auto sel = ds.selection({"x","y","z"});

    using point_type = coordinate_point<double>;
    using coords_type = coordinate_range<point_type>;

    {
        point_type cp(&sel, 0);
        CHECK(cp[0] == 1.0);
        CHECK(cp[1] == 2.0);
        CHECK(cp[2] == 1.0);
        point_type cp2(&sel,0);
        CHECK(cp == cp2);
    }
    {
        point_type cp(&sel,1);
        CHECK(cp[0] == 1.0);
        CHECK(cp[1] == 1.0);
        CHECK(cp[2] == 4.0);
        point_type cp2(&sel, 0);
        ++cp2.index();
        CHECK(cp == cp2);
    }

    {
        coords_type ccr(sel);
        debug("coordinate_iterator is type {}", type(ccr.begin()));
        debug("coordinate_iterator value is type {}", type(*ccr.begin()));

        for (const auto& cp : ccr) {
            CHECK(cp.size() == sel.size());
        }

        boost::sub_range<coords_type> sr(ccr);
        for (const auto& cp : sr) {
            CHECK(cp.size() == sel.size());
        }

    }
    
}

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
    using point_type = coordinate_point<double>;
    using coords_type = coordinate_range<point_type>;

    coords_type coords(sel);
    CHECK(coords.size() == 3); 

    auto beg = coords.begin();
    auto end = coords.end();

    CHECK(beg != end);

    REQUIRE(std::distance(beg,end) == 3);
    CHECK(std::distance(beg,beg) == 0);
    CHECK(std::distance(end,end) == 0);

    debug("doctest_pointcloud_iterator: copy iterator");
    coords_type coords2 = coords;

    CHECK(coords2.size() == 3); 

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

}
