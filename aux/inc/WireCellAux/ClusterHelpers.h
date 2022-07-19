// Provide some helper functions for working with ICluster

#ifndef WIRECELLAUX_CLUSTERJSONIFY
#define WIRECELLAUX_CLUSTERJSONIFY

#include "WireCellUtil/Configuration.h"
#include "WireCellIface/IFrame.h"
#include "WireCellIface/ISlice.h"
#include "WireCellIface/ICluster.h"

#include <string>

namespace WireCell::Aux {

    /// Return JSON representation of the cluster.
    Json::Value jsonify(const ICluster& cluster);

    /// Return GraphViz dot representation of the cluster.  The nodes
    /// is a list (string) of cluster node type codes to include.  If
    /// empty, all node types will be included.
    std::string dotify(const ICluster& cluster, const std::string& nodes="bsm");

    /// Return name for the cluster in a canonical form suitable for
    /// use as a file name.  It only includes info related directly to
    /// the cluster (the ident).
    std::string name(const ICluster& cluster);

    /// Return the slices in the cluster.
    ISlice::vector find_slices(const ICluster& cluster);

    /// Return the frame of the first slice in the cluster.  Note, in
    /// principle, clusters can span frames.
    IFrame::pointer find_frame(const ICluster& cluster);

    /// Return the blobs for each slice
    using blobs_by_slice_t = std::unordered_map<ISlice::pointer, IBlob::vector>;
    blobs_by_slice_t blobs_by_slice(const ICluster& cluster);

    /// Return number of vertices of each type by type code
    using code_counts_t = std::unordered_map<char, size_t>;
    code_counts_t counts_by_type(const ICluster& cluster);

    /// Return the vertex descriptors for the given type code (in "cwsbm")
    std::vector<cluster_vertex_t> vdesc_by_code(const ICluster& cluster, char code);

}

#endif

