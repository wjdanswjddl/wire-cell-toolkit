// This reimplements the ideas from:
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/ChargeSolving.cxx
// and
// https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/MatrixSolver.cxx

#include "WireCellImg/ChargeSolving.h"
#include "WireCellIface/SimpleCluster.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/IndexedSet.h"
#include "WireCellUtil/Ress.h"
#include "WireCellUtil/Logging.h"
#include <iterator>
#include <Eigen/Core>
#include <Eigen/Dense>

WIRECELL_FACTORY(ChargeSolving, WireCell::Img::ChargeSolving,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;
using value_t = ISlice::value_t;

Img::ChargeSolving::ChargeSolving()
    : Aux::Logger("ChargeSolving", "img")
{
}

Img::ChargeSolving::~ChargeSolving() {}

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
}

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
        cfg["strategies"].append(one);
    }

    return cfg;
}


    
// Return an inherent ordering value for a measure.  It does not
// matter what ordering results but it must be stable on re-running.
// We return the smallest channel ident in the measure on the
// assumption that each measure has a unique set of channels w/in
// its slice.
using meas_node_t = IChannel::shared_vector;
inline int ordering(const meas_node_t& m)
{
    int order = 0x7fffffff;
    for (const auto& one : *m) {
        int ident = one->ident();
        if (ident < order) {
            order = ident;
        }
    }
    return order;
}

// Return an arbitrary, inherent and stable value on which a
// collection of IBlobs may be ordered.
using blob_node_t = IBlob::pointer;
inline int ordering(const blob_node_t& blob)
{
    // This assumes that a context sets ident uniquely.
    return blob->ident();
}

using slice_node_t = ISlice::pointer;
static value_t measure_sum(const meas_node_t& imeas, const slice_node_t& islice)
{
    // Use double for the sum for a little extra precision.
    WireCell::Measurement::float64 value = 0;

    const auto& activity = islice->activity();
    for (const auto& ich : *(imeas.get())) {
        const auto& it = activity.find(ich);
        if (it == activity.end()) {
            continue;
        }
        value += it->second;
    }
    return value_t(value);
}


// The type associated with a cs_graph_t as a whole
struct cs_graphprop_t {
    // The input islice from which this graph was derived
    slice_node_t islice;

    // An arbitrary index counting the original connected subgraph
    // from the slice from which this graph derives
    size_t index{0};

    // The two chi2 components from a solution.
    double chi2_base{0};
    double chi2_l1{0};
};
// The type associated to a cs_graph_t vertex
struct cs_node_t {
    // Remember where we came from in the original cluster graph.
    cluster_vertex_t orig_desc;

    // Mark what kind of the vertex this is.  
    enum Kind { unknown=0, meas, blob };
    Kind kind;

    // A number by which a collection of vertices of the same kind may
    // be ordered
    int ordering;

    // Both m-kind and b-kind vertex has a value as
    // (central,uncertainty) pairs.  For blob type, the central value
    // may be used on input to the solver as a starting value and
    // holds the result of the solving on output.  The uncertainty is
    // interpreted as a weight and is untouched by solving.  For meas
    // type, the value is the sum of the per channel values.
    value_t value;

};
namespace std {
    template <>
    struct hash<cs_node_t> {
        std::size_t operator()(const cs_node_t& n) const {
            return n.ordering;
        }
    };
}
// New type of graph with just b's and m's holding ready to use info
using cs_graph_t = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                         cs_node_t, boost::no_property, cs_graphprop_t>;
using cs_vdesc_t = boost::graph_traits<cs_graph_t>::vertex_descriptor;
using cs_edesc_t = boost::graph_traits<cs_graph_t>::edge_descriptor;
using cs_vertex_iter_t = boost::graph_traits<cs_graph_t>::vertex_iterator;
    

// Return selection of vertex descriptions of kind ordered by
// .ordering and as an indexed set.
IndexedSet<cs_vdesc_t> cs_select_ordered(const cs_graph_t& csg,
                                         cs_node_t::Kind kind)
{
    std::vector<cs_vdesc_t> ret;
    for (const auto& v : boost::make_iterator_range(boost::vertices(csg))) {
        const auto& vp = csg[v];
        if (vp.kind == kind) {
            ret.push_back(v);
        }
    }
    std::sort(ret.begin(), ret.end(), [&](const cs_vdesc_t& a, const cs_vdesc_t& b) {
        return csg[a].ordering < csg[b].ordering;
    });
    return IndexedSet<cs_vdesc_t>(ret);
}

using double_vector_t = Eigen::VectorXd;
using double_matrix_t = Eigen::MatrixXd;

struct SolveParams {
    Ress::Params ress;
    double scale{1000};
};
static cs_graph_t solve(const cs_graph_t& csg, const SolveParams& params)
{
    cs_graph_t csg_out;

    IndexedSet<cs_vdesc_t> blob_desc_out;
    auto blob_desc = cs_select_ordered(csg, cs_node_t::blob);

    const size_t nblob = blob_desc.size();
    double_vector_t source = double_vector_t::Zero(nblob);
    double_vector_t weight = double_vector_t::Zero(nblob);
    for (size_t bind=0; bind<nblob; ++bind) {
        const auto& blob_in = csg[blob_desc.collection[bind]];

        const auto valerr = blob_in.value;
        source(bind) = valerr.value();
        weight(bind) = valerr.uncertainty();

        auto desc_out = boost::add_vertex(blob_in, csg_out);
        blob_desc_out(desc_out);        
    }

    IndexedSet<cs_vdesc_t> meas_desc_out;
    auto meas_desc = cs_select_ordered(csg, cs_node_t::meas);

    const size_t nmeas = meas_desc.size();
    double_vector_t measure = double_vector_t::Zero(nmeas);
    double_matrix_t mcov = double_matrix_t::Zero(nmeas, nmeas);
    for (size_t mind=0; mind<nmeas; ++mind) {
        const auto& meas_in = csg[meas_desc.collection[mind]];
        const auto valerr = meas_in.value;
        measure(mind) = valerr.value();
        mcov(mind, mind) = valerr.uncertainty();

        auto desc_out = boost::add_vertex(meas_in, csg_out);
        meas_desc_out(desc_out);        
    }

    double_matrix_t A = double_matrix_t::Zero(meas_desc.size(), blob_desc.size());

    for (auto [ei, ei_end] = boost::edges(csg); ei != ei_end; ++ei) {
        const cs_vdesc_t tail = boost::source(*ei, csg);
        const cs_vdesc_t head = boost::target(*ei, csg);

        const auto kind1 = csg[tail].kind;
        const auto kind2 = csg[head].kind;
        int bind{0}, mind{0};
        if (kind1 == cs_node_t::blob and kind2 == cs_node_t::meas) {
            bind = blob_desc.get(tail);
            mind = meas_desc.get(head);
        }        
        else if (kind2 == cs_node_t::blob and kind1 == cs_node_t::meas) {
            bind = blob_desc.get(head);
            mind = meas_desc.get(tail);
        }
        else {
            // someone has violated my requirements with this edge!
            continue;
        }
        A(mind, bind) = 1;

        boost::add_edge(blob_desc_out.collection[bind],
                        meas_desc_out.collection[mind],
                        csg_out);
    }

    Eigen::LLT<double_matrix_t> llt(mcov.inverse());
    double_matrix_t U = llt.matrixL().transpose();

    // The measure vector in a "whitened" basis
    double_vector_t m_white = params.scale*U*measure;

    // The blob-measure association in "whitened" basis (becomes
    // the "reasponse" matrix in ress solving).
    double_matrix_t R_white = U * A;

    auto solution = Ress::solve(R_white, m_white, params.ress,
                                source, weight);
    auto predicted = Ress::predict(R_white, solution);

    auto& gp_out = csg_out[boost::graph_bundle];
    gp_out.chi2_base = Ress::chi2_base(m_white, predicted);
    gp_out.chi2_l1 = Ress::chi2_l1(m_white, solution, params.ress.lambda);

    // Update outgoing blob nodes with their solution
    for (size_t ind=0; ind<nblob; ++ind) {
        auto& bvalue = csg_out[blob_desc_out.collection[ind]];
        bvalue.value.value(solution[ind]);
    }
    return csg_out;
}


cs_graph_t unpack_slice(const cluster_graph_t& cgraph,
                        cluster_vertex_t sd,
                        const value_t& meas_thresh)
{
    cs_graph_t slice_graph;

    const auto svtx = cgraph[sd];
    if (svtx.code() != 's') {
        return slice_graph;
    }
    slice_node_t islice = std::get<slice_node_t>(svtx.ptr);

    slice_graph[boost::graph_bundle].islice = islice;
    slice_graph[boost::graph_bundle].index = 0; // don't actually care yet

    // get the blobs's
    for (auto sb_edge : boost::make_iterator_range(boost::out_edges(sd, cgraph))) {
        cluster_vertex_t bd = boost::target(sb_edge, cgraph);
        const auto& bvtx = cgraph[bd];
        if (bvtx.code() != 'b') { continue; }
                
        auto bptr = std::get<blob_node_t>(bvtx.ptr);
        cs_node_t blob{bd, cs_node_t::blob, ordering(bptr), value_t(0,1)};
        auto blob_desc = boost::add_vertex(blob, slice_graph);

        // collect the blobs's and the measures's
        int nsaved = 0;
        for (auto bx_edge : boost::make_iterator_range(boost::out_edges(bd, cgraph))) {
            cluster_vertex_t xd = boost::target(bx_edge, cgraph);
            const auto& xvtx = cgraph[xd];
            char code = xvtx.code();
            if (code != 'm') {
                continue;
            }
            auto mptr = std::get<meas_node_t>(xvtx.ptr);
            const auto msum = measure_sum(mptr, islice);
            if (msum.value() < meas_thresh.value() or
                msum.uncertainty() > meas_thresh.uncertainty()) {
                continue;
            }
            cs_node_t meas{xd, cs_node_t::meas, ordering(mptr), msum};
                    
            auto meas_desc = boost::add_vertex(meas, slice_graph);
            boost::add_edge(blob_desc, meas_desc, slice_graph);
            ++nsaved;
        }

        // In principle, all measures supporting a blob can fail the
        // threshold.
        if (!nsaved) {
            boost::remove_vertex(blob_desc, slice_graph);
        }
    }
    return slice_graph;
}


using cs_graph_vector_t = std::vector<cs_graph_t>;

void unpack_slice_subgraphs(const cs_graph_t& slice_graph,
                            std::back_insert_iterator<cs_graph_vector_t> subgraphs_out)
{

    auto islice = slice_graph[boost::graph_bundle].islice;

    std::unordered_map<int, std::vector<cs_vdesc_t> > groups;
    std::unordered_map<cs_vdesc_t, int> desc2id;
    boost::connected_components(slice_graph, boost::make_assoc_property_map(desc2id));
    for (auto& [desc,id] : desc2id) {  // invert
        groups[id].push_back(desc);
    }
    for (const auto& [index, slice_vdescs] : groups) {
        cs_graph_t sub_graph;
        sub_graph[boost::graph_bundle].islice = islice;
        sub_graph[boost::graph_bundle].index = index;
        std::unordered_map<cs_vdesc_t, cs_vdesc_t> slice2sub;
        // Add map from slice to sub graph vdesc's
        for (const auto& slice_vdesc : slice_vdescs) {
            cs_vdesc_t sub_vdesc = boost::add_vertex(slice_graph[slice_vdesc], sub_graph);
            slice2sub[slice_vdesc] = sub_vdesc;
        }
        // Add edges
        for (const auto& slice_vdesc : slice_vdescs) {
            for (auto edge : boost::make_iterator_range(boost::out_edges(slice_vdesc, slice_graph))) {
                auto tail = slice2sub[boost::source(edge, slice_graph)];
                auto head = slice2sub[boost::target(edge, slice_graph)];
                boost::add_edge(tail, head, sub_graph);
            }
        }
        subgraphs_out = sub_graph;
    }
}

void unpack(const cluster_graph_t& cgraph,
            std::back_insert_iterator<cs_graph_vector_t> subgraphs,
            const value_t& meas_thresh)
{
    // get the slices's
    for (const auto& sd : boost::make_iterator_range(boost::vertices(cgraph))) {
        auto slice_graph = unpack_slice(cgraph, sd, meas_thresh);
        unpack_slice_subgraphs(slice_graph, subgraphs);
    }
}

static
cluster_graph_t pack(const std::vector<cs_graph_t>& csgs)
{
    cluster_graph_t tbd;
    return tbd;
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
    cs_graph_vector_t sgs;
    unpack(in->graph(), std::back_inserter(sgs), m_meas_thresh);

    const size_t nstrats = m_weighting_strategies.size();

    SolveParams sparams{Ress::Params{Ress::lasso}};
    for (size_t ind = 0; ind < nstrats; ++ind) {
        std::transform(sgs.begin(), sgs.end(), sgs.begin(),
                       [&](const cs_graph_t& sg) {
                           return solve(sg, sparams);
                       });
    }

    out = std::make_shared<SimpleCluster>(pack(sgs), in->ident());
    return true;
}
