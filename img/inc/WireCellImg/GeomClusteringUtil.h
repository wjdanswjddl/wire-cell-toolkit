

/**
 * make b-b edges using simple geom. based RayGrid::overlap
 */

#ifndef WIRECELLIMG_GEOMCLUSTERINGUTIL
#define WIRECELLIMG_GEOMCLUSTERINGUTIL

#include "WireCellUtil/IndexedGraph.h"
#include "WireCellIface/IBlobSet.h"
#include "WireCellIface/IClustering.h"

namespace WireCell {
    namespace Img {

        // IBlobSet blobs are within one time slice
        // grind nodes can be found by IBlob object
        void geom_clustering(cluster_indexed_graph_t& grind, IBlobSet::vector::iterator beg,
                             IBlobSet::vector::iterator end, std::string policy);

        // blob filter function signature
        using gc_filter_t = std::function<bool(const cluster_vertex_t&)>;

        // ICluster based one
        void geom_clustering(cluster_graph_t& cg, std::string policy,
                             gc_filter_t filter = [](const cluster_vertex_t&) { return true; });
    }  // namespace Img
}  // namespace WireCell

#endif