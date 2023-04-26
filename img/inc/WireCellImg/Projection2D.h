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

            // For matrix representation of the graphs.
            using scaler_t = float;
            using sparse_mat_t = Eigen::SparseMatrix<scaler_t>;

            struct Projection2D {
                Projection2D(){}
                Projection2D(const size_t nchan, const size_t nslice) : m_nchan(nchan), m_nslice(nslice), m_proj({m_nchan, m_nslice}) {}
                // uboone: 8256,9592
                size_t m_nchan {0}; // bins in the channel direction
                size_t m_nslice {0}; // bins in the time direction
                sparse_mat_t m_proj{m_nchan, m_nslice};
            };

            // returns group id -> cluster_vertex_t in each group
            std::unordered_map<int, std::set<cluster_vertex_t> > get_geom_clusters(const WireCell::cluster_graph_t& cg);

            struct LayerProjection2DMap {
                using layer_projection2d_map_t = std::unordered_map<WirePlaneLayer_t, Projection2D>;
                layer_projection2d_map_t m_layer_proj;
                double m_estimated_charge {0};
            };
            // returns layer ID -> channel-tick-charge matrix
            LayerProjection2DMap get_projection(const WireCell::cluster_graph_t& cg, const std::set<cluster_vertex_t>&, const size_t nchan, const size_t nslice);

            std::string dump(const Projection2D& proj2d, bool verbose=false);
            bool write(const Projection2D& proj2d, const std::string& fname="proj2d.tar.gz");

            //
            // std::vector<int> calc_coverage(const Projection2D& ref, const Projection2D& tar);

            // see .cxx for more details
            int judge_coverage(const Projection2D& ref, const Projection2D& tar);
            int judge_coverage_alt(const Projection2D& ref, const Projection2D& tar);

        }  // namespace Projection2D
    }  // namespace Img
}  // namespace WireCell
