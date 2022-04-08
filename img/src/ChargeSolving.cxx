// This reimplements the ideas from:
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/ChargeSolving.cxx
// and
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/MatrixSolver.cxx


#include "WireCellImg/ChargeSolving.h"
#include "WireCellImg/CSGraph.h"

#include "WireCellIface/SimpleCluster.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"

#include <iterator>

WIRECELL_FACTORY(ChargeSolving, WireCell::Img::ChargeSolving,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Img::CS;



Img::ChargeSolving::ChargeSolving()
    : Aux::Logger("ChargeSolving", "img")
{
}

Img::ChargeSolving::~ChargeSolving() {}

WireCell::Configuration Img::ChargeSolving::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["meas_value_threshold"] = m_meas_thresh.value();
    cfg["meas_error_threshold"] = m_meas_thresh.uncertainty();
    cfg["blob_value_threshold"] = m_blob_thresh.value();
    cfg["blob_error_threshold"] = m_blob_thresh.uncertainty();

    cfg["lasso_tolerance"] = m_lasso_tolerance;
    cfg["lasso_minnorm"] = m_lasso_minnorm;
    for (const auto& one : m_weighting_strategies) {
        cfg["weighting_strategies"].append(one);
    }

    return cfg;
}


void blob_weight_uniform(const cluster_graph_t& /*cgraph*/, graph_t& csg)
{
    for (auto desc : vertex_range(csg)) {
        auto& vtx = csg[desc];
        if (vtx.kind != node_t::blob) {
            continue;
        }
        vtx.value.uncertainty(1.0);
    }
}
void blob_weight_simple(const cluster_graph_t& cgraph, graph_t& csg)
{
    for (auto desc : vertex_range(csg)) {
        auto& vtx = csg[desc];
        if (vtx.kind != node_t::blob) {
            continue;
        }
        // get neighbors in original cluster graph
        // count how many unique slices have connected blobs
        // add 1.0.  that's the simple weight.
        std::unordered_set<int> slice_idents;
        slice_idents.insert(csg[boost::graph_bundle].islice->ident());
        for (auto edge : mir(boost::out_edges(vtx.orig_desc, cgraph)))
        {
            vdesc_t ndesc = boost::target(edge, cgraph);
            const auto& nnode = cgraph[ndesc];
            if (nnode.code() == 'b') {
                slice_idents.insert(get<blob_node_t>(nnode.ptr)->slice()->ident());
            }
        }
        vtx.value.uncertainty((float) slice_idents.size());
    }
}
// fixme: implement distance.

// Weighting function lookup
using blob_weighting_f = std::function<void(const cluster_graph_t& cgraph, graph_t& csg)>;
using blob_weighting_lut = std::unordered_map<std::string, blob_weighting_f>;
static blob_weighting_lut gStrategies{
    {"uniform", blob_weight_uniform},
    {"simple", blob_weight_simple},
    // distance....
};


void Img::ChargeSolving::configure(const WireCell::Configuration& cfg)
{
    float mt_val = get(cfg,"meas_value_threshold", m_meas_thresh.value());
    float mt_err = get(cfg,"meas_error_threshold", m_meas_thresh.uncertainty());
    m_meas_thresh = value_t(mt_val, mt_err);

    float bt_val = get(cfg,"blob_value_threshold", m_blob_thresh.value());
    float bt_err = get(cfg,"blob_error_threshold", m_blob_thresh.uncertainty());
    m_blob_thresh = value_t(bt_val, bt_err);

    m_lasso_tolerance = get(cfg, "lasso_tolerance", m_lasso_tolerance);
    m_lasso_minnorm = get(cfg, "lasso_minnorm", m_lasso_minnorm);

    m_weighting_strategies =
        get<std::vector<std::string>>(cfg, "weighting_strategies",
                                      m_weighting_strategies);
    for (const auto& strat : m_weighting_strategies) {
        if (gStrategies.find(strat) == gStrategies.end()) {
            THROW(ValueError() << errmsg{"Unknown weighting strategy: " + strat});
        }
    }
}


bool Img::ChargeSolving::operator()(const input_pointer& in, output_pointer& out)
{
    if (!in) {
        out = nullptr;
        log->debug("EOS");
        return true;
    }

    // Separate the big graph spanning the whole frame into connected
    // b-m subgraphs with all the info needed for solving each round.
    graph_vector_t sgs;
    const auto in_graph = in->graph();
    unpack(in_graph, std::back_inserter(sgs), m_meas_thresh);

    log->debug("cluster: {} input nvertices={} nedges={} found nsubgraphs={}",
               in->ident(),
               boost::num_vertices(in_graph),
               boost::num_edges(in_graph),
               sgs.size()
        );


    const size_t nstrats = m_weighting_strategies.size();

    SolveParams sparams{Ress::Params{Ress::lasso}};
    for (size_t ind = 0; ind < nstrats; ++ind) {
        const auto& strategy = m_weighting_strategies[ind];
        log->debug("cluster: {} strategy={}",
                   in->ident(), strategy);

        auto& blob_weighter = gStrategies[strategy];

        std::transform(sgs.begin(), sgs.end(), sgs.begin(),
                       [&](graph_t& sg) {
                           blob_weighter(in_graph, sg);
                           auto new_csg = solve(sg, sparams);
                           SPDLOG_LOGGER_TRACE(
                               log,
                               "cluster={} slice={} subcluster={} solved nvertices={} nedges={}",
                               in->ident(),
                               new_csg[boost::graph_bundle].islice->ident(),
                               new_csg[boost::graph_bundle].index,
                               boost::num_vertices(new_csg),
                               boost::num_edges(new_csg));

                           // fixme: add blob threshold pruning

                           return new_csg;
                       });
    }
    log->debug("cluster={} solved nsubclusters={} over nstrategies={}",
               in->ident(), sgs.size(), m_weighting_strategies.size());

    auto packed = repack(in_graph, sgs);

    log->debug("cluster={} nvertices={} nedges={}",
               in->ident(),
               boost::num_vertices(packed),
               boost::num_edges(packed));

    out = std::make_shared<SimpleCluster>(packed, in->ident());
    return true;
}


