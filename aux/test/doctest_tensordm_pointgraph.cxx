#include "WireCellAux/TensorDMpointgraph.h"

#include "WireCellUtil/doctest.h"
#include "WireCellUtil/Logging.h"

using namespace WireCell;
using namespace WireCell::PointCloud;
using namespace WireCell::Aux::TensorDM;



TEST_CASE("tensordm pointgraph empty")
{
    PointGraph pg;
    const std::string datapath = "blah";
    auto tens = as_tensors(pg, datapath);
    CHECK(tens.size() == 3);
    CHECK(tens[0]->metadata()["datatype"] == "pcgraph");
    CHECK(tens[0]->metadata()["datapath"] == datapath);
    CHECK(tens[0]->size() == 0);

    // nodes
    CHECK(tens[1]->metadata()["datatype"] == "pcdataset");
    CHECK(tens[1]->metadata()["datapath"] == datapath + "/nodes");
    CHECK(tens[1]->size() == 0);

    // edges
    CHECK(tens[2]->metadata()["datatype"] == "pcdataset");
    CHECK(tens[2]->metadata()["datapath"] == datapath + "/edges");
    CHECK(tens[2]->size() == 0);

    auto pg2 = as_pointgraph(tens, datapath);
    CHECK(pg2.nodes().size_major() == 0);
    CHECK(pg2.edges().size_major() == 0);
}

using graph_t = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS>;
TEST_CASE("tensordm pointgraph simple")
{
    Dataset nodes({
            {"one", Array({1  ,2  ,3  })},
            {"two", Array({1.1,2.2,3.3})},
        });
    Dataset edges({
            {"tails", Array({0, 1, 2})},
            {"heads", Array({1, 2, 0})},
        });

    PointGraph pg(nodes, edges);


    {
        auto g = pg.boost_graph<graph_t>();
        CHECK(boost::num_vertices(g) == 3);
        CHECK(boost::num_edges(g) == 3);
    }
    
    const std::string datapath = "blah";
    auto tens = as_tensors(pg, datapath);
    {
        CHECK(tens[0]->metadata()["datapath"].asString() == datapath);
        CHECK(tens.size() == 1 + 1+2 + 1+2);
    }

    auto pg2 = as_pointgraph(tens, datapath);
    {
        auto g = pg2.boost_graph<graph_t>();
        CHECK(boost::num_vertices(g) == 3);
        CHECK(boost::num_edges(g) == 3);
    }
    
}
