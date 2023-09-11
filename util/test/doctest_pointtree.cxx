#include "WireCellUtil/PointTree.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

using namespace WireCell::PointCloud;
using spdlog::debug;

TEST_CASE("point tree key")
{
    size_t k0 = NodeValue::key("pcname", {"x","y","z"}, 0);
    size_t k1 = NodeValue::key("pcname", {"x","y","z"}, 1);
    size_t k2 = NodeValue::key("PCNAME", {"x","y","z"}, 0);
    size_t k3 = NodeValue::key("pcname", {"X","Y","Z"}, 0);
    size_t k4 = NodeValue::key("pcname", {"x","y","z"}, 0);

    CHECK(k0 != k1);
    CHECK(k0 != k2);
    CHECK(k0 != k3);
    CHECK(k0 == k4);

}
