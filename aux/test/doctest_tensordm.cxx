#include "WireCellAux/TensorDMcommon.h"

#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Testing.h"

using namespace WireCell;
using namespace WireCell::Aux::TensorDM;

using spdlog::debug;

ITensor::pointer make_tensor(const std::string& datatype, const std::string datapath)
{
    debug("doctest_tensordm: make tensor type={} path={}", datatype, datapath);
    return make_metadata_tensor(datatype, datapath);
}

TEST_CASE("tensordm top tensor")
{
    // Log::add_stderr(true, "debug");

    debug("doctest_tensordm: chirp");
    ITensor::vector tens = {
        make_tensor("type1","path1"),
        make_tensor("type2","path2"),
        make_tensor("type1","path3") };
    CHECK(tens[0] == top_tensor(tens, "type1", "path1"));
    CHECK(tens[1] == top_tensor(tens, "type2", "path2"));
    CHECK(tens[2] == top_tensor(tens, "type1", "path3"));
    CHECK(tens[0] == top_tensor(tens, "type1"));
}
