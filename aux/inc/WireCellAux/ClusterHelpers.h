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

}

#endif

