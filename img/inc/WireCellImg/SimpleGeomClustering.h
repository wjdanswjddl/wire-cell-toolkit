
/**
 * make b-b edges using simple geom. based RayGrid::overlap
 */

#ifndef WIRECELLIMG_SIMPLEGEOMCLUSTERING
#define WIRECELLIMG_SIMPLEGEOMCLUSTERING

#include "WireCellUtil/IndexedGraph.h"
#include "WireCellIface/IBlobSet.h"
#include "WireCellIface/IClustering.h"

namespace WireCell {
    namespace Img {
        void simple_geom_clustering(cluster_indexed_graph_t& grind, IBlobSet::vector::iterator beg,
                                    IBlobSet::vector::iterator end, std::string policy);
    }
}  // namespace WireCell

#endif