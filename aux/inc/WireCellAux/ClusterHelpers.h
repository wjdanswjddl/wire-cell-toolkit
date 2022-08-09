/** Provide some helper functions for working with ICluster.

 */

#ifndef WIRECELLAUX_CLUSTERJSONIFY
#define WIRECELLAUX_CLUSTERJSONIFY

#include "WireCellUtil/Configuration.h"
#include "WireCellIface/IFrame.h"
#include "WireCellIface/ISlice.h"
#include "WireCellIface/ICluster.h"

#include <string>

namespace WireCell::Aux {

    /// Return JSON representation of the cluster.
    Json::Value jsonify(const cluster_graph_t& cgraph);

    /// Return GraphViz dot representation of the cluster.  The "types"
    /// is a list (string) of cluster node type codes to include.  If
    /// empty, all node types will be included.
    std::string dotify(const cluster_graph_t& cgraph, const std::string& types="bsm");

    /// Return the slices in the cluster.
    ISlice::vector find_slices(const cluster_graph_t& cgraph);

    /// Return the frame of the first slice in the cluster.  Note, in
    /// principle, clusters can span frames.
    IFrame::pointer find_frame(const cluster_graph_t& cgraph);

    /// Return the blobs for each slice
    using blobs_by_slice_t = std::unordered_map<ISlice::pointer, IBlob::vector>;
    blobs_by_slice_t blobs_by_slice(const cluster_graph_t& cgraph);

    /// Return number of vertices of each type by type code
    using code_counts_t = std::unordered_map<char, size_t>;
    code_counts_t counts_by_type(const cluster_graph_t& cgraph);

    // Return the vertex descriptors for the given type code (in "cwsbm")
    // std::vector<cluster_vertex_t> vdesc_by_code(const cluster_graph_t& cgraph, char code);

    // Return a flat vector of vertices in cgraph by their type code.
    // Vectors have no particular ordering.
    using cluster_vertices_t = std::vector<cluster_vertex_t>;
    using cluster_vertices_type_map_t = std::unordered_map<char, cluster_vertices_t>;
    cluster_vertices_type_map_t vertices_type_map(const cluster_graph_t& cgraph);

    namespace cluster::directed {
        using graph_t  = boost::adjacency_list<boost::setS, boost::vecS, boost::directedS, cluster_node_t>;
        using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
        using edge_t = boost::graph_traits<graph_t>::edge_descriptor;

        /// Return a directed version of the cgraph with direction
        /// determined by a vertex type code order.  The vertices and
        /// edges and their order within their sets are unchanged in
        /// the returned graph.  However, the order of an individual
        /// edge edpoints (which is "source" vs "target") is subject
        /// change based on the "order" string which provides cluster
        /// node type codes in ascending value.  Eg, with the default
        /// "order" string, an edge formed with an s-type and a b-type
        /// will produce an s-b edge.  Edges between like types (eg
        /// b-b edge) or edges with the type code of one or both
        /// endpoints that are not represented in the "order" string
        /// will be output in their original order.
        graph_t type_directed(const cluster_graph_t& cgraph,
                              const std::string& order = "sbmwc");
    
    }
    /// Return an s-b-w graph given an s-b-w-c cluster graph.  The
    /// c-nodes (and any m-nodes) are removed.
    cluster_graph_t extract_sbw(const cluster_graph_t& cgraph);

    /// Return an sbc graph given an sbwc cluster graph.  The w-nodes
    /// are removed and b-c edges made.  Any m-nodes are also removed.
    cluster_graph_t extract_sbc(const cluster_graph_t& cgraph);

    /// Simply return an sbw or sbc depending on if code is 'w' or 'c'
    /// or return an empty graph.
    cluster_graph_t extract_sbX(const cluster_graph_t& cgraph, char code);

}

#endif

