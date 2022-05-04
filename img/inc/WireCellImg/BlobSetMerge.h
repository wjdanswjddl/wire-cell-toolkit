/**
 * different from BlobSetSync
 * currently only works on Img::Data::Slice
*/

#ifndef WIRECELL_IMG_BLOBSETMERGE
#define WIRECELL_IMG_BLOBSETMERGE

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IBlobSetFanin.h"
#include "WireCellAux/Logger.h"

namespace WireCell {
    namespace Img {

        class BlobSetMerge : public Aux::Logger, public IBlobSetFanin, public IConfigurable {
           public:
            BlobSetMerge();
            virtual ~BlobSetMerge();

            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual std::vector<std::string> input_types();
            virtual bool operator()(const input_vector& invec, output_pointer& out);

           private:
            size_t m_multiplicity;
        };
    }  // namespace Img
}  // namespace WireCell

#endif
