/**
   Modify the Channel Mask Map and pass through everything else
 */

#ifndef WIRECELLIMG_CMMMODIFIER
#define WIRECELLIMG_CMMMODIFIER

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellIface/IAnodePlane.h"

#include <string>

namespace WireCell {
    namespace Img {

        class CMMModifier : public Aux::Logger,
                                public IFrameFilter,
                                public IConfigurable
        {
           public:
            CMMModifier();
            virtual ~CMMModifier();

            /// IFrameFilter interface.
            virtual bool operator()(const input_pointer& in, output_pointer& out);

            /// IConfigurable interface.
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

           private:
            // Needed for channel lookups
            IAnodePlane::pointer m_anode{nullptr};

            // ChannelMast to be processed
            std::string m_cm_tag{"bad"};

            // Traces that would be used in the CM processing
            std::string m_trace_tag{"gauss"};
        };
    }  // namespace Img
}  // namespace WireCell

#endif
