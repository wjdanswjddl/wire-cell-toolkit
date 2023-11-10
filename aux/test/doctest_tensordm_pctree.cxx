#include "WireCellAux/TensorDMpointtree.h"
#include "WireCellUtil/PointTree.h"
#include "WireCellUtil/PointTesting.h"

#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

using namespace WireCell;
using namespace WireCell::PointTesting;
using namespace WireCell::PointCloud;
using namespace WireCell::Aux::TensorDM;
using namespace WireCell::PointCloud::Tree;

using spdlog::debug;

static
Points::node_ptr make_simple_pctree()
{
    Points::node_ptr root = std::make_unique<Points::node_t>();

    // Insert a child with a set of named points clouds with one point
    // cloud from a track.
    auto* n1 = root->insert(Points({ {"3d", make_janky_track()} }));

    const auto& pc1 = n1->value.local_pcs().at("3d");

    // Ibid from a different track
    auto* n2 = root->insert(Points({ {"3d",
                    make_janky_track(
                        Ray(Point(-1, 2, 3), Point(1, -2, -3)))}}));


    const auto& pc2 = n2->value.local_pcs().at("3d");

    REQUIRE(pc1 != pc2);
    REQUIRE_FALSE(pc1 == pc2);

    return root;
}

TEST_CASE("tensordm pctree")
{
    auto root = make_simple_pctree();
    const std::string datapath = "root";
    auto tens = as_tensors(*root.get(), datapath);
    CHECK(tens.size() > 0);

    debug("{:20} {}", "datatype", "datapath");
    for (auto ten : tens) {
        auto md = ten->metadata();
        debug("{:20} {}", md["datatype"].asString(), md["datapath"].asString());
    }

    auto root2 = as_pctree(tens, datapath);
    CHECK(root2);
}

