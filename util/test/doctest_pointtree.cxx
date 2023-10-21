#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/PointTesting.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

#include <unordered_map>

using namespace WireCell;
using namespace WireCell::PointTesting;
using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
using spdlog::debug;

using node_ptr = std::unique_ptr<Points::node_t>;


TEST_CASE("point tree scope")
{
    Scope s;
    CHECK(s.pcname == "");
    CHECK(s.coords.empty());
    CHECK(s.depth == 0);

    Scope s0{ "pcname", {"x","y","z"}, 0};
    Scope s1{ "pcname", {"x","y","z"}, 1};
    Scope s2{ "PCNAME", {"x","y","z"}, 0};
    Scope s3{ "pcname", {"X","Y","Z"}, 0};
    Scope sc{ "pcname", {"x","y","z"}, 0};

    CHECK(s0 == s0);
    CHECK(s0 == sc);
    CHECK(s0 != s1);
    CHECK(s0 != s2);
    CHECK(s0 != s3);

    CHECK(s0.hash() == sc.hash());
    CHECK(s0.hash() != s1.hash());
    CHECK(s0.hash() != s2.hash());
    CHECK(s0.hash() != s3.hash());

    std::unordered_map<Scope, size_t> m;
    m[s1] = 1;
    m[s2] = 2;
    CHECK(m[s0] == 0);          // spans default
    CHECK(m[s1] == 1);
    CHECK(m[s2] == 2);

}

TEST_CASE("point tree no points")
{
    Points p;
    CHECK(p.node() == nullptr);
    auto& lpcs = p.local_pcs();
    CHECK(lpcs.empty());

    Scope s;
    auto pcr = p.scoped_pc(s);
    CHECK(pcr.size() == 0);

    const auto& kd = p.scoped_kd<double>(s);
    CHECK(kd.points().size() == 0);
}

static
Points::node_ptr make_simple_pctree()
{
    Points::node_ptr root = std::make_unique<Points::node_t>();

    // Insert a child with a set of named points clouds with one point
    // cloud from a track.
    auto* n1 = root->insert(Points({ {"3d", make_janky_track()} }));

    const Dataset& pc1 = n1->value.local_pcs().at("3d");

    // Ibid from a different track
    auto* n2 = root->insert(Points({ {"3d", make_janky_track(
                        Ray(Point(-1, 2, 3), Point(1, -2, -3)))} }));

    const Dataset& pc2 = n2->value.local_pcs().at("3d");

    REQUIRE(pc1 != pc2);
    REQUIRE_FALSE(pc1 == pc2);

    return root;
}

TEST_CASE("point tree with points")
{
    auto root = make_simple_pctree();
    CHECK(root.get());

    auto& rval = root->value;

    CHECK(root->children().size() == 2);
    CHECK(root.get() == rval.node());
    CHECK(rval.local_pcs().empty());
    
    {
        auto& cval = *(root->child_values().begin());
        const auto& pcs = cval.local_pcs();
        CHECK(pcs.size() > 0);
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

    Scope scope{ "3d", {"x","y","z"}};

    const auto& pc3d = rval.scoped_pc(scope);
    CHECK(pc3d.size() == 2);

    auto& kd = rval.scoped_kd(scope);
    // CHECK(&kd.pointclouds() == &pc3d);

    std::vector<double> origin = {0,0,0};
    auto knn = kd.knn(6, origin);
    for (auto [it,dist] : knn) {
        auto& pt = *it;
        debug("knn: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(knn.size() == 6);


    auto rad = kd.radius(.001, origin);
    for (auto [it,dist] : rad) {
        auto& pt = *it;
        debug("rad: pt=({},{},{}) dist={}",
              pt[0], pt[1], pt[2], dist);
    }
    CHECK(rad.size() == 6);

}

TEST_CASE("point tree remove node")
{
    auto root = make_simple_pctree();
    Scope scope{ "3d", {"x","y","z"}};
    auto& rval = root->value;

    // NOTE: it is the nodes that hold the Datasets so we get a collection of std::ref.

    const auto& pc3d_orig = rval.scoped_pc(scope);
    const Dataset& pc3d_one = pc3d_orig.at(0).get();
    const Dataset& pc3d_two = pc3d_orig.at(1).get();
    
    SUBCASE("remove child one") {
        const size_t nleft = pc3d_two.size_major();
        auto it = root->children().begin();
        // NOTE: receiving the dead node keeps its datasets alive.
        auto dead = root->remove(it);
        CHECK(dead);
        CHECK(root->children().size() == 1);
        const auto& pc3d = rval.scoped_pc(scope);
        CHECK(pc3d.size() == 1);
        CHECK(pc3d[0].get() == pc3d_two);

        const auto& kd = rval.scoped_kd(scope);
        CHECK(kd.points().size() == nleft);
    }
    SUBCASE("remove child two") {
        const size_t nleft = pc3d_one.size_major();
        auto it = root->children().begin();
        ++it;
        // NOTE: receiving the dead node keeps its datasets alive.
        auto dead = root->remove(it);
        CHECK(dead);
        CHECK(root->children().size() == 1);
        const auto& pc3d = rval.scoped_pc(scope);
        CHECK(pc3d.size() == 1);
        CHECK(pc3d[0].get() == pc3d_one);

        const auto& kd = rval.scoped_kd(scope);
        CHECK(kd.points().size() == nleft);
    }
    SUBCASE("shared cached point cloud") {
        Scope sxy{ "3d", {"x","y"}};
        Scope syz{ "3d", {"y","z"}};
        auto& pcxy = rval.scoped_pc(sxy);
        auto& pcyz = rval.scoped_pc(syz);
        CHECK(pcxy.size() == pcyz.size());

    }
}
TEST_CASE("point tree merge trees")
{
    auto r1 = make_simple_pctree();
    auto r2 = make_simple_pctree();

    Points::node_t root;
    root.insert(std::move(r1));
    root.insert(std::move(r2));
    CHECK(!r1);
    CHECK(!r2);


    Scope scope{ "3d", {"x","y","z"}};
    auto& rval = root.value;
    const auto& pc3d = rval.scoped_pc(scope);
    CHECK(pc3d.size() == 4);
}

