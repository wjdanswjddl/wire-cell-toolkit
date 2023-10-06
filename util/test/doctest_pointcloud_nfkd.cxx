#include "WireCellUtil/NFKD.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/ExecMon.h"
#include "WireCellUtil/String.h"

#include "WireCellUtil/PointCloudCoordinates.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointCloudDisjoint.h"

#include "WireCellUtil/doctest.h"

#include <vector>

using spdlog::debug;
using namespace WireCell;
using namespace WireCell::PointCloud;
using WireCell::String::format;


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


// Smallest point cloud to consider
static const size_t start_size = 1024;

// How many times we double the point cloud size in tests.  Want to
// keep this somewhat small to allow the test to run quickly.  Feel
// free to increase it for a local build and test but keep it at ~3
// for commits.  see util/docs/nary-tree.org for some benchmarks.
static const size_t max_doubling = 3;

// A list of coordinate names and thus dimension of the k-d space.
static const std::vector<std::string> coord_names = {"x","y","z"};

Dataset make_dataset(size_t npts,
                     const std::vector<std::string>& names = coord_names)
{
    std::random_device rnd_device;
    // Specify the engine and distribution.
    std::mt19937 mersenne_engine {rnd_device()};  // Generates random integers
    std::uniform_real_distribution<double> dist {-1.0, 1.0};
    
    auto gen = [&dist, &mersenne_engine](){
                   return dist(mersenne_engine);
               };

    std::vector<double> arr(npts);

    Dataset ds;

    for (const auto& name: names) {
        generate(begin(arr), end(arr), gen);
        ds.add(name, Array(arr));
    }
    return ds;
}

TEST_CASE("point cloud disjoint iteration")
{
    ExecMon em("point cloud disjoint iteration");

    using point_type = std::vector<double>;

    for (size_t npts = start_size; npts <= (1<<max_doubling)*start_size; npts *= 2) {

        for (size_t nper = 16; nper <= npts; nper *= 8) {
            const size_t ndses = npts/nper;

            debug("\n{}", em(format("%d points, %d datasets: start test", npts, ndses)));

            std::vector<Dataset> ds_store;
            for (size_t count=0; count<ndses; ++count) {
                ds_store.emplace_back(make_dataset(nper, coord_names));
            }

            debug("\n{}", em(format("%d points, %d datasets: built datasets", npts, ndses)));

            DisjointDataset dds;
            for (size_t count=0; count<ndses; ++count) {
                dds.append(ds_store.back());
            }

            debug("\n{}", em(format("%d points, %d datasets: built disjoint dataset", npts, ndses)));

            auto coords = dds.selection<point_type>(coord_names);

            debug("\n{}", em(format("%d points, %d datasets: built coords", npts, ndses)));

            REQUIRE(coords.size() == dds.size());

            for (const auto& c : coords) {
                CHECK (c.size());
            }

            debug("\n{}", em(format("%d points, %d datasets: iterated coords", npts, ndses)));
            for (size_t ind=0; ind < dds.size(); ++ind) {
                const auto& a = dds.address(ind);
                CHECK(a.first >=0);
                CHECK(a.second >=0);
            }

            debug("\n{}", em(format("%d points, %d datasets: scan disjoint addresses", npts, ndses)));
        }
    }
    debug("Time and memory usage:\n{}", em.summary());
}


template<typename IndexType>
void do_monolithic(const std::string& index_name)
{

    ExecMon em("nfkd dataset monolithic " + index_name);

    using point_type = std::vector<double>;
    using coords_type = coordinates<point_type>;
    using kdtree_type = NFKD::Tree<coords_type::iterator, IndexType>;

    for (size_t npts = start_size; npts <= (1<<max_doubling)*start_size; npts *= 2) {

        em(format("%d points: start test", npts));

        auto ds = make_dataset(npts, coord_names);

        em(format("%d points: built dataset", npts));

        coords_type coords(ds.selection(coord_names));
        kdtree_type kd(coord_names.size(), coords.begin(), coords.end());
        CHECK(kd.size() == npts);

        em(format("%d points: built k-d tree", npts));

        for (size_t N = 1; N <= npts; N *= 2) {
            auto knn = kd.knn(N, {0,0,0});
            REQUIRE(knn.size() == N);

            em(format("%d points: knn query returns %d (expected %d)", npts, knn.size(), N));
        }

        for (double radius = 0.001; radius <= 1; radius *= 10) {
            auto rad = kd.radius(radius, {0,0,0});

            em(format("%d points: rad query returns %d (radius %f)", npts, rad.size(), radius));
        }
    }
    debug("Time and memory usage:\n{}", em.summary());
}


TEST_CASE("nfkd dataset performance monolithic static")
{
    do_monolithic<NFKD::IndexStatic>("static");
}
TEST_CASE("nfkd dataset performance monolithic dynamic")
{
    do_monolithic<NFKD::IndexDynamic>("dynamic");
}

TEST_CASE("nfkd dataset performance disjoint")
{
    ExecMon em("nfkd dataset disjoint");

    using point_type = std::vector<double>;
    using coords_type = coordinates<point_type>;
    using kdtree_type = NFKD::Tree<coords_type::iterator, NFKD::IndexDynamic>;

    for (size_t npts = start_size; npts <= (1<<max_doubling)*start_size; npts *= 2) {

        for (size_t nper = 16; nper <= npts; nper *= 8) {
            const size_t ndses = npts/nper;

            em(format("%d points, %d datasets: start test", npts, ndses));

            std::vector<Dataset> ds_store;
            for (size_t count=0; count<ndses; ++count) {
                ds_store.emplace_back(make_dataset(nper, coord_names));
            }

            em(format("%d points, %d datasets: built datasets", npts, ndses));

            std::vector<coords_type> co_store;
            for (size_t count=0; count<ndses; ++count) {
                co_store.emplace_back(ds_store[count].selection(coord_names));
            }

            em(format("%d points, %d datasets: built coords", npts, ndses));

            kdtree_type kd(coord_names.size());
            for (size_t count=0; count<ndses; ++count) {
                auto& co = co_store.back();
                kd.append(co.begin(), co.end());
            }
            REQUIRE(kd.size() == npts);

            em(format("%d points, %d datasets: built k-d tree", npts, ndses));

            for (size_t N = 1; N <= npts; N *= 2) {
                auto knn = kd.knn(N, {0,0,0});
                CHECK(knn.size() == N);

                em(format("%d points, %d datasets: knn query returns %d (expected %d)",
                          npts, ndses, knn.size(), N));
            }

            for (double radius = 0.001; radius <= 1; radius *= 10) {
                auto rad = kd.radius(radius, {0,0,0});
                CHECK(rad.size() <= npts);

                em(format("%d points, %d datasets: rad query returns %d (radius %f)",
                          npts, ndses, rad.size(), radius));
            }
        }

    }
    debug("Time and memory usage:\n{}", em.summary());
}
