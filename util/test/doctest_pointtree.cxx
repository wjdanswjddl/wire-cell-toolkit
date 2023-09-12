#include "WireCellUtil/PointTree.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
using spdlog::debug;

TEST_CASE("point tree key")
{
    // NodeValue nv0 = {"pcname", {"x","y","z"}, 0};
    // NodeValue nv1 = {"pcname", {"x","y","z"}, 1};
    // NodeValue nv2 = {"PCNAME", {"x","y","z"}, 0};
    // NodeValue nv3 = {"pcname", {"X","Y","Z"}, 0};
    // NodeValue nv4 = {"pcname", {"x","y","z"}, 0};

    // CHECK(nv0 != nv1);
    // CHECK(nv0 != nv2);
    // CHECK(nv0 != nv3);
    // CHECK(nv0 == nv4);

}
