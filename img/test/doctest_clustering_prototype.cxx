#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/PointTesting.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

#include <unordered_map>

using namespace WireCell;
using namespace WireCell::PointTesting;
using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
// WireCell::PointCloud::Tree::scoped_pointcloud_t
using spdlog::debug;

using node_ptr = std::unique_ptr<Points::node_t>;

static void print_dds(const scoped_pointcloud_t& dds) {
    for (size_t idx=0; idx<dds.size(); ++idx) {
        const Dataset& ds = dds[idx];
        std::stringstream ss;
        ss << "ds: " << idx << std::endl;
        const size_t len = ds.size_major();
        for (const auto& key : ds.keys()) {
            auto arr = ds.get(key)->elements<double>();
            ss << key << ": ";
            for(auto elem : arr) {
                ss << elem << " ";
            }
            ss << std::endl;
        }
        std::cout << ss.str() << std::endl;
    }
}

static
Points::node_ptr make_simple_pctree()
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug

    // empty root node
    Points::node_ptr root = std::make_unique<Points::node_t>();

    // Insert a child with a set of named points clouds with one point
    // cloud from a track.
    /// QUESTION: can only do this on construction?
    /// QUESTION: units?
    auto* n1 = root->insert(Points({
        {"center", make_janky_track(Ray(Point(0.5, 0, 0), Point(0.7, 0, 0)))},
        {"3d", make_janky_track(Ray(Point(0, 0, 0), Point(1, 0, 0)))}
        }));

    const Dataset& pc1 = n1->value.local_pcs().at("3d");
    debug("pc1: {}", pc1.size_major());
    

    // Ibid from a different track
    auto* n2 = root->insert(Points({
        {"center", make_janky_track(Ray(Point(1.5, 0, 0), Point(1.7, 0, 0)))},
        {"3d", make_janky_track(Ray(Point(1, 0, 0), Point(2, 0, 0)))}
        }));

    const Dataset& pc2 = n2->value.local_pcs().at("3d");
    debug("pc2: {}", pc2.size_major());

    REQUIRE(pc1 != pc2);
    REQUIRE_FALSE(pc1 == pc2);

    return root;
}

TEST_CASE("PointCloudFacade test")
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    auto root = make_simple_pctree();
    CHECK(root.get());

    // from WireCell::NaryTree::Node to WireCell::PointCloud::Tree::Points
    auto& rval = root->value;

    CHECK(root->children().size() == 2);
    CHECK(root.get() == rval.node()); // node raw pointer == point's node
    CHECK(rval.local_pcs().empty());
    
    {
        auto& cval = *(root->child_values().begin());
        const auto& pcs = cval.local_pcs();
        CHECK(pcs.size() > 0);
        // using pointcloud_t = Dataset;
        // key: std::string, val: Dataset
        for (const auto& [key,val] : pcs) {
            debug("child has pc named \"{}\" with {} points", key, val.size_major());
        }
        const auto& pc3d = pcs.at("3d");
        debug("got child PC named \"3d\" with {} points", pc3d.size_major());
        CHECK(pc3d.size_major() > 0);
        CHECK(pc3d.has("x"));
        CHECK(pc3d.has("y"));
        CHECK(pc3d.has("z"));
        CHECK(pc3d.has("q"));
    }

    // name, coords, [depth]
    Scope scope{ "3d", {"x","y","z"}};
    const scoped_pointcloud_t& pc3d = rval.scoped_pc(scope);
    CHECK(pc3d.size() == 2);
    print_dds(pc3d);

    const scoped_pointcloud_t& pccenter = rval.scoped_pc({ "center", {"x","y","z"}});
    print_dds(pccenter);

    const auto& skd = rval.scoped_kd<double>(scope);
    // CHECK(&kd.pointclouds() == &pc3d);

    /// QUESTION: how to get it -> node?
    const std::vector<double> origin = {1,0,0};
    auto knn = skd.knn(2, origin);
    for (auto [it,dist] : knn) {
        auto& pt = *it;
        debug("knn: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(knn.size() == 2);

    const auto& all_points = skd.points();
    for (size_t pt_ind = 0; pt_ind<knn.size(); ++pt_ind) {
        auto& [pit,dist] = knn[pt_ind];
        const size_t maj_ind = all_points.major_index(pit);
        const size_t min_ind = all_points.minor_index(pit);
        debug("knn point {} at distance {} from query is in local point cloud {} at index {}",
              pt_ind, dist, maj_ind, min_ind);
        const Dataset& pc = pc3d[maj_ind];
        for (const auto& name : scope.coords) {
            debug("\t{} = {}", name, pc.get(name)->element<double>(min_ind));
        }
    }

    auto rad = skd.radius(.01, origin);
    for (auto [it,dist] : rad) {
        auto& pt = *it;
        debug("rad: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(rad.size() == 2);

}