/**
 * InSliceDeghosting
 * 1, Ident good/bad blob groups. in: ICluster out: blob_quality_tags {blob_desc -> quality_tag} TODO: map or multi-map?
 * 2, Local (in-slice) de-ghosting. in: ICluster + blob_quality_tags out: updated blob_quality_tags (remove or not)
 * 3, delete some blobs. in: ICluster + blob_quality_tags out: filtered ICluster
 * 4, in-group clustering. in: ICluster + blob_quality_tags out: filtered ICluster
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
            enum BLOB_QUALITY { GOOD, BAD, POTENTIAL_GOOD, POTENTIAL_BAD, TO_BE_REMOVED };

            InSliceDeghosting();
            virtual ~InSliceDeghosting();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:
            /// TODO: DEBUGONLY
            bool m_dryrun{false};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_INSLICEDEGHOSTING_H */
