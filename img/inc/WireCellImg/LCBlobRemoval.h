/**
 * remove blobs with low charge
 */
#ifndef WIRECELL_LCBLOBREMOVING_H
#define WIRECELL_LCBLOBREMOVING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class LCBlobRemoval : public Aux::Logger, public IClusterFilter, public IConfigurable {
          public:
            LCBlobRemoval();
            virtual ~LCBlobRemoval();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

          private:

            // Used to hold measurement and blob values
            // (central+uncertainty).
            using value_t = ISlice::value_t;
            // Config: blob_{value,error}_threshold. A blob with
            // central value less than this is dropped.  The
            // uncertainty is not currently considered.
            value_t m_blob_thresh{0,1};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_LCBLOBREMOVING_H */
