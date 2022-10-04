
#include "WireCellImg/TestClusterShadow.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/Projection2D.h"

#include "WireCellAux/ClusterShadow.h"
#include "WireCellAux/SimpleCluster.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Rectangles.h"

#include <iterator>
#include <chrono>

WIRECELL_FACTORY(TestClusterShadow, WireCell::Img::TestClusterShadow,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::Img;
using namespace WireCell::Img::CS;
using namespace WireCell::Img::Projection2D;

Img::TestClusterShadow::TestClusterShadow()
    : Aux::Logger("TestClusterShadow", "img")
{
}

Img::TestClusterShadow::~TestClusterShadow() {}

WireCell::Configuration Img::TestClusterShadow::default_configuration() const
{
    WireCell::Configuration cfg;

    return cfg;
}

void Img::TestClusterShadow::configure(const WireCell::Configuration& cfg)
{
}

static void dump_cg(const cluster_graph_t& cg, Log::logptr_t& log)
{
    size_t mcount{0}, bcount{0};
    value_t bval;
    for (const auto& vtx : mir(boost::vertices(cg))) {
        const auto& node = cg[vtx];
        if (node.code() == 'b') {
            const auto iblob = get<blob_t>(node.ptr);
            bval += value_t(iblob->value(), iblob->uncertainty());
            ++bcount;
            continue;
        }
        if (node.code() == 'm') {
            const auto imeas = get<meas_t>(node.ptr);
            ++mcount;
        }
    }
    log->debug("cluster graph: vertices={} edges={} $blob={} bval={} #meas={}",
               boost::num_vertices(cg), boost::num_edges(cg),
               bcount, bval, mcount);
}

bool Img::TestClusterShadow::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);
    auto& out_graph = in_graph;
    BlobShadow::graph_t bsgraph = BlobShadow::shadow(out_graph, 'w'); // or 'c'
    ClusterShadow::blob_cluster_map_t clusters;
    auto cs_graph = ClusterShadow::shadow(out_graph, bsgraph, clusters);
    dump_cg(out_graph, log);
    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    return true;
}


