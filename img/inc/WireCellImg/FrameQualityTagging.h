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

            // Gauss
            std::string m_gauss_trace_tag{"gauss"};
            // Wiener
            std::string m_wiener_trace_tag{"wiener"};
            // CMM tag
            std::string m_cm_tag{"bad"};
            // tick0
            int m_tick0{0};
            // nticks
            int m_nticks{9592};
        };
    }  // namespace Img
}  // namespace WireCell

#endif
