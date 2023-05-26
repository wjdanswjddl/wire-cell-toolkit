/**
 * LocalGeomClustering
 * 1, divide blobs into old geom. cluster groups
 * 2, remove old b-b edges
 * 3, new b-b edges within groups. geom_clustering with filters for each group
 */
#ifndef WIRECELL_LOCALGEOMCLUSTERING_H
#define WIRECELL_LOCALGEOMCLUSTERING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class LocalGeomClustering : public Aux::Logger, public IClusterFilter, public IConfigurable {
           public:
            LocalGeomClustering();
            virtual ~LocalGeomClustering();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:
            /// TODO: DEBUGONLY
            bool m_dryrun{false};
            std::string m_clustering_policy{"uboone_local"};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_LOCALGEOMCLUSTERING_H */
