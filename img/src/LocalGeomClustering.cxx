
#include "WireCellImg/LocalGeomClustering.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/GeomClusteringUtil.h"
#include "WireCellAux/SimpleCluster.h"
#include "WireCellUtil/GraphTools.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Exceptions.h"

#include <iterator>
#include <chrono>
#include <fstream>

WIRECELL_FACTORY(LocalGeomClustering, WireCell::Img::LocalGeomClustering, WireCell::INamed, WireCell::IClusterFilter,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Aux;

Img::LocalGeomClustering::LocalGeomClustering()
  : Aux::Logger("LocalGeomClustering", "img")
{
}

Img::LocalGeomClustering::~LocalGeomClustering() {}

WireCell::Configuration Img::LocalGeomClustering::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["dryrun"] = m_dryrun;
    return cfg;
}

void Img::LocalGeomClustering::configure(const WireCell::Configuration& cfg)
{
    m_dryrun = get<bool>(cfg, "dryrun", m_dryrun);
    Json::FastWriter jwriter;
    log->debug("{}", jwriter.write(cfg));
}

namespace {
    void dump_cg(const cluster_graph_t& cg, Log::logptr_t& log)
    {
        size_t mcount{0}, bcount{0};
        CS::value_t bval;
        for (const auto& vtx : GraphTools::mir(boost::vertices(cg))) {
            const auto& node = cg[vtx];
            if (node.code() == 'b') {
                const auto iblob = get<cluster_node_t::blob_t>(node.ptr);
                bval += CS::value_t(iblob->value(), iblob->uncertainty());
                ++bcount;
                continue;
            }
            if (node.code() == 'm') {
                const auto imeas = get<cluster_node_t::meas_t>(node.ptr);
                ++mcount;
            }
        }
        log->debug("cluster graph: vertices={} edges={} #blob={} bval={} #meas={}", boost::num_vertices(cg),
                   boost::num_edges(cg), bcount, bval, mcount);
    }
}

bool LocalGeomClustering::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);

    /// output
    auto& out_graph = in_graph;
    // debug info
    log->debug("in_graph:");
    dump_cg(in_graph, log);
    log->debug("out_graph:");
    dump_cg(out_graph, log);

    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    if (m_dryrun) {
        out = std::make_shared<Aux::SimpleCluster>(in_graph, in->ident());
    }
    return true;
}