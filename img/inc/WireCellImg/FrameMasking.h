/**
   Apply a ChannelMask to a set of trace
 */

#ifndef WIRECELLIMG_FRAMEMASKING
#define WIRECELLIMG_FRAMEMASKING

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellIface/IAnodePlane.h"

#include <string>

namespace WireCell {
    namespace Img {

        class FrameMasking : public Aux::Logger,
                                public IFrameFilter,
                                public IConfigurable
        {
           public:
            FrameMasking();
            virtual ~FrameMasking();

            /// IFrameFilter interface.
            virtual bool operator()(const input_pointer& in, output_pointer& out);

            /// IConfigurable interface.
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

           private:
            // Needed for channel lookups
            IAnodePlane::pointer m_anode{nullptr};

            // ChannelMask to be processed
            std::string m_cm_tag{"bad"};

            // Traces that would be used in the CM processing
            std::vector<std::string> m_trace_tags{{"gauss"}};
        };
    }  // namespace Img
}  // namespace WireCell

#endif
