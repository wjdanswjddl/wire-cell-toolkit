/** BlobSolving takes in a cluster graph and produces another with values assigned to blobs.
 */
#ifndef WIRECELL_BLOBSOLVING_H
#define WIRECELL_BLOBSOLVING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class BlobSolving : public Aux::Logger, public IClusterFilter, public IConfigurable {
           public:
            BlobSolving();
            virtual ~BlobSolving();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_BLOBSOLVING_H */
