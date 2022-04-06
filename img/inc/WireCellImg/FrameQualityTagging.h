/**
   Tagging the quality of a frame
 */

#ifndef WIRECELLIMG_FRAMEQUALITYTAGGING
#define WIRECELLIMG_FRAMEQUALITYTAGGING

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellIface/IAnodePlane.h"

#include <string>

namespace WireCell {
    namespace Img {

        class FrameQualityTagging : public Aux::Logger,
                                public IFrameFilter,
                                public IConfigurable
        {
           public:
            FrameQualityTagging();
            virtual ~FrameQualityTagging();

            /// IFrameFilter interface.
            virtual bool operator()(const input_pointer& in, output_pointer& out);

            /// IConfigurable interface.
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

           private:
            // Needed for channel lookups
            IAnodePlane::pointer m_anode{nullptr};

            // Traces that would be used in the CM processing
            std::string m_trace_tag{"gauss"};
        };
    }  // namespace Img
}  // namespace WireCell

#endif
