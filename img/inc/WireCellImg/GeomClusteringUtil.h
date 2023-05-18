

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

        // ICluster based one
        void geom_clustering(cluster_graph_t& cg, std::string policy);
    }  // namespace Img
}  // namespace WireCell

#endif