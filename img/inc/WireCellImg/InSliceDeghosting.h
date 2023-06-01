/**
 * InSliceDeghosting
 * 1, Ident good/bad blob groups. in: ICluster out: blob_quality_tags {blob_desc -> quality_tag} TODO: map or multi-map?
 * 2, Local (in-slice) de-ghosting. in: ICluster + blob_quality_tags out: updated blob_quality_tags (remove or not)
 * 3, delete some blobs. in: ICluster + blob_quality_tags out: filtered ICluster
 * 4, in-group clustering. in: ICluster + blob_quality_tags out: updated ICluster
 */
#ifndef WIRECELL_INSLICEDEGHOSTING_H
#define WIRECELL_INSLICEDEGHOSTING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class InSliceDeghosting : public Aux::Logger, public IClusterFilter, public IConfigurable {
           public:
            /// TODO: what is needed here
            /// FIXME: bit operation would be better
            enum BLOB_QUALITY_BITPOS { GOOD, BAD, POTENTIAL_GOOD, POTENTIAL_BAD, TO_BE_REMOVED };

            using vertex_tags_t = std::unordered_map<cluster_vertex_t, int>;
            // using vertex_tagging_t = std::function<void(const cluster_graph_t&, vertex_tags_t&)>;

            InSliceDeghosting();
            virtual ~InSliceDeghosting();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:
            void blob_quality_ident(const cluster_graph_t& cg, vertex_tags_t& blob_tags);
            void local_deghosting(const cluster_graph_t& cg, vertex_tags_t& blob_tags);
            void local_deghosting1(const cluster_graph_t& cg, vertex_tags_t& blob_tags);

            /// TODO: DEBUGONLY
            bool m_dryrun{false};
            double m_good_blob_charge_th{300.};
            std::string m_clustering_policy{"uboone"};
            int m_config_round{1};
            float m_deghost_th{0.75};
            float m_deghost_th1{0.5};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_INSLICEDEGHOSTING_H */
