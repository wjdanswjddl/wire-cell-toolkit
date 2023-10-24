/**
 * GlobalGeomClustering
 * 1, remove old b-b edges
 * 2, new b-b edges geom_clustering with no filters
 */
#ifndef WIRECELL_GLOBALGEOMCLUSTERING_H
#define WIRECELL_GLOBALGEOMCLUSTERING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class GlobalGeomClustering : public Aux::Logger, public IClusterFilter, public IConfigurable {
           public:
            GlobalGeomClustering();
            virtual ~GlobalGeomClustering();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:
            std::string m_clustering_policy{"uboone"};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_GLOBALGEOMCLUSTERING_H */
