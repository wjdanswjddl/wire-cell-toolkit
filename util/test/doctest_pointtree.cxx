#include "WireCellUtil/PointTree.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

#include <unordered_map>

using namespace WireCell::PointCloud;
using namespace WireCell::PointCloud::Tree;
using spdlog::debug;

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

TEST_CASE("point tree points empty")
{
    Points p;
    CHECK(p.node() == nullptr);
    auto& lpcs = p.local_pcs();
    CHECK(lpcs.empty());

    Scope s;
    const auto& dds = p.scoped_pc(s);
    CHECK(dds.datasets().empty());
    CHECK(dds.npoints() == 0);

    const auto& kd = p.scoped_kd(s);
    const auto& dds2 = kd.pointclouds();
    CHECK(dds2.datasets().empty());
    CHECK(dds2.npoints() == 0);
    CHECK(&dds == &dds2);
}
