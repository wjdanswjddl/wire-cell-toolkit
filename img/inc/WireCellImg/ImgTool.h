/**

   Some common tools

*/

#include "WireCellIface/IFrame.h"
#include "WireCellIface/IWire.h"
#include "WireCellIface/IChannel.h"
#include "WireCellIface/IStripe.h"
#include "WireCellIface/IStripeSet.h"
#include "WireCellIface/ISlice.h"
#include "WireCellIface/ISliceFrame.h"
#include "WireCellIface/ICluster.h"
#include <Eigen/Core>
#include <Eigen/Sparse>

namespace WireCell {
    namespace Img {
        namespace Tool {
            // Here, "node" implies a cluster graph vertex payload object.
            using channel_t = cluster_node_t::channel_t;
            using wire_t = cluster_node_t::wire_t;
            using blob_t = cluster_node_t::blob_t;
            using slice_t = cluster_node_t::slice_t;
            using meas_t = cluster_node_t::meas_t;

            // For matrix representation of the graphs.
            using sparse_dmat_t = Eigen::SparseMatrix<double>;

            using vdesc_t = boost::graph_traits<cluster_graph_t>::vertex_descriptor;
            using edesc_t = boost::graph_traits<cluster_graph_t>::edge_descriptor;

            // 
            std::unordered_map<int, std::vector<vdesc_t> > get_geom_clusters(const WireCell::cluster_graph_t& cg);

            //
            sparse_dmat_t get_2D_projection(const WireCell::cluster_graph_t& cg, std::vector<vdesc_t>);

        }  // namespace Tool
    }  // namespace Img
}  // namespace WireCell
