/**
 * Projection2D
 * Project a group of blobs (cluster) to a certain view
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
        namespace Projection2D {
            // Here, "node" implies a cluster graph vertex payload object.
            using channel_t = cluster_node_t::channel_t;
            using wire_t = cluster_node_t::wire_t;
            using blob_t = cluster_node_t::blob_t;
            using slice_t = cluster_node_t::slice_t;
            using meas_t = cluster_node_t::meas_t;

            // For matrix representation of the graphs.
            using scaler_t = float;
            using sparse_mat_t = Eigen::SparseMatrix<scaler_t>;

            // chan_min, chan_max, tick_min, tick_max
            using projection_bound_t = std::tuple<int, int, int, int>;

            struct Projection2D {
                projection_bound_t m_bound {
                    std::numeric_limits<int>::max(),
                    std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max(),
                    std::numeric_limits<int>::min()
                };
                sparse_mat_t m_proj;
            };

            using vdesc_t = boost::graph_traits<cluster_graph_t>::vertex_descriptor;
            using edesc_t = boost::graph_traits<cluster_graph_t>::edge_descriptor;

            // return vertex descriptors connected to the given vertex descriptor.
            std::vector<vdesc_t> neighbors(const WireCell::cluster_graph_t& cg, const vdesc_t& vd);

            // 
            template <typename Type>
            std::vector<vdesc_t> neighbors_oftype(const WireCell::cluster_graph_t& cg, const vdesc_t& vd);

            // returns group id -> vdesc_t in each group
            std::unordered_map<int, std::vector<vdesc_t> > get_geom_clusters(const WireCell::cluster_graph_t& cg);

            // returns layer ID -> channel-tick-charge matrix
            using layer_projection_map_t = std::unordered_map<WirePlaneLayer_t, Projection2D>;
            layer_projection_map_t get_2D_projection(const WireCell::cluster_graph_t& cg, std::vector<vdesc_t>);

            std::string dump(const Projection2D& proj2d, bool verbose=false);
            bool write(const Projection2D& proj2d, const std::string& fname="proj2d.tar.gz");

            //
            // std::vector<int> calc_coverage(const Projection2D& ref, const Projection2D& tar);

            // 1: tar is part of ref; 2: tar is equal to ref; -1: ref is part of tar; ref and tar do not overlap
            int judge_coverage(const Projection2D& ref, const Projection2D& tar);
            // int judge_coverage_alt(const Projection2D& ref, const Projection2D& tar);

        }  // namespace Projection2D
    }  // namespace Img
}  // namespace WireCell
