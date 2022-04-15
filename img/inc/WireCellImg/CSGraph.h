/**
   CSGraph - charge solving graph

   This is used internally by ChargeSolving.

 */

#ifndef WIRECELLIMG_CSGRAPH
#define WIRECELLIMG_CSGRAPH

#include "WireCellIface/ICluster.h"
#include "WireCellUtil/IndexedSet.h"
#include "WireCellUtil/Ress.h"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <iterator>

namespace WireCell::Img::CS {

    
    // A value holds (central,uncertainty) for measures and blobs.
    // For blobs, uncertainty is interpeted as the weight.
    using value_t = ISlice::value_t;

    // For matrix representation of the graphs.
    using double_vector_t = Eigen::VectorXd;
    using double_matrix_t = Eigen::MatrixXd;

    // Here, "node" implies a cluster graph vertex payload object.
    using meas_node_t = IChannel::shared_vector;
    using blob_node_t = IBlob::pointer;
    using slice_node_t = ISlice::pointer;

    // Return an inherent ordering value for a measure.  It does not
    // matter what ordering results but it must be stable on
    // re-running.  We return the smallest channel ident in the
    // measure on the assumption that each measure has a unique set
    // of channels w/in its slice.
    int ordering(const meas_node_t& m);

    // Return an arbitrary, inherent and stable value on which a
    // collection of IBlobs may be ordered.
    int ordering(const blob_node_t& blob);

    // The type associated with a graph_t as a whole
    struct graphprop_t {
        // The input islice from which this graph was derived
        slice_node_t islice;

        // An arbitrary index counting the original connected subgraph
        // from the slice from which this graph derives
        size_t index{0};

        // The two chi2 components from a solution.
        double chi2_base{0};
        double chi2_l1{0};
    };

    // The vertex payload type associated to a charge solving graph
    // vertex.
    struct node_t {
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

    //
    // The grpah type.  Just b's and m's holding ready to use info
    //
    using graph_t = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                          node_t, boost::no_property, graphprop_t>;
    using vdesc_t = boost::graph_traits<graph_t>::vertex_descriptor;
    using edesc_t = boost::graph_traits<graph_t>::edge_descriptor;
    using vertex_iter_t = boost::graph_traits<graph_t>::vertex_iterator;
    
    // make easier iteration.
    // for (auto v : mir(boost::vertices(graph))) {
    //    for (auto edge : mir(boost::out_edges(v, graph)) { ... }
    // }
    template <typename It> boost::iterator_range<It> mir(std::pair<It, It> const& p) {
        return boost::make_iterator_range(p.first, p.second);
    }
    template <typename It> boost::iterator_range<It> mir(It b, It e) {
        return boost::make_iterator_range(b, e);
    }
    template <typename Gr> 
    boost::iterator_range<typename boost::graph_traits<Gr>::vertex_iterator> vertex_range(Gr& g) {
        return mir(boost::vertices(g));
    }

    // Used, eg when forming subgraphs.
    using graph_vector_t = std::vector<graph_t>;

    // Return selection of vertex descriptions of kind ordered by
    // .ordering and as an indexed set.
    using indexed_vdescs_t = IndexedSet<CS::vdesc_t>;
    indexed_vdescs_t select_ordered(const graph_t& csg,
                                    node_t::Kind kind);
                           
    // Convert IChannel::shared_vector into a value.
    value_t measure_sum(const meas_node_t& imeas, const slice_node_t& islice);

    // Break up input CS graph into "connected subgraphs".
    void connected_subgraphs(const graph_t& slice_graph,
                             std::back_insert_iterator<graph_vector_t> subgraphs_out);

    // Break a cluster_graph into a vector of connected subgraphs
    // represented by a CS graph_t.
    void unpack(const cluster_graph_t& cgraph,
                std::back_insert_iterator<graph_vector_t> subgraphs,
                const value_t& meas_thresh);

    // Reform a cluster graph from a collection of CS graphs.
    cluster_graph_t repack(const cluster_graph_t& cgraph,
                           const std::vector<graph_t>& csgs);

    // Solve the graph, returning a new one holding solution in blob nodes
    struct SolveParams {
        Ress::Params ress;
        double scale{1000};
        // If true, apply matrix transform of the cholesky
        // decomposition on the measure covariance matrix.  If False,
        // measurement uncertainties are not considered.
        bool whiten{true};
    };
    graph_t solve(const graph_t& csg, const SolveParams& params);

    // Return a copy of the input retaining only blobs with charge
    // above threshold.  Any blobless measures are also removed.
    graph_t prune(const graph_t& csg, float threshold=0);
}

namespace std {
    template <>
    struct hash<WireCell::Img::CS::node_t> {
        std::size_t operator()(const WireCell::Img::CS::node_t& n) const {
            return n.ordering;
        }
    };
}
#endif
