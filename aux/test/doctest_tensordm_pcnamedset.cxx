#include "WireCellAux/TensorDMpointtree.h"

#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

#include <map>

using namespace WireCell;
using namespace WireCell::PointCloud;
using namespace WireCell::Aux::TensorDM;

using spdlog::debug;


TEST_CASE("tensordm pcnamedset")
{
    const std::string datapath = "pcns";
    const std::string store = "pcns/the-named-point-clouds";
    named_pointclouds_t pcns;
    pcns["one"] = Dataset();
    pcns["two"] = Dataset();
    auto tens = pcnamedset_as_tensors(pcns.begin(), pcns.end(), datapath, store);
    CHECK(tens.size() == 3);

    CHECK(tens[0]->metadata()["datatype"] == "pcnamedset");
    CHECK(tens[0]->metadata()["datapath"] == datapath);
    CHECK(tens[0]->size() == 0);
    
    CHECK(tens[1]->metadata()["datatype"] == "pcdataset");
    CHECK(tens[1]->metadata()["datapath"] == store + "/one");
    CHECK(tens[1]->size() == 0);
    
    CHECK(tens[2]->metadata()["datatype"] == "pcdataset");
    CHECK(tens[2]->metadata()["datapath"] == store + "/two");
    CHECK(tens[2]->size() == 0);

    auto pcns2 = as_pcnamedset(tens, datapath);
    CHECK(pcns2.size() == 2);
    CHECK(pcns2.find("one") != pcns2.end());
    CHECK(pcns2.find("two") != pcns2.end());

}

