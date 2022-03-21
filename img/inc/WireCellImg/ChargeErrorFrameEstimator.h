/**
   Add a set of traces for charge error, which is estimated based on the ROI length
   Pass on other parts of the frame
 */

#ifndef WIRECELLSIGPROC_CHARGEERRORFRAMEESTIMATOR
#define WIRECELLSIGPROC_CHARGEERRORFRAMEESTIMATOR

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

#include <string>

namespace WireCell {
    namespace Img {

        class ChargeErrorFrameEstimator : public Aux::Logger,
                                public IFrameFilter,
                                public IConfigurable
        {
           public:
            ChargeErrorFrameEstimator();
            virtual ~ChargeErrorFrameEstimator();

            /// IFrameFilter interface.
            virtual bool operator()(const input_pointer& in, output_pointer& out);

            /// IConfigurable interface.
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

	    
	    
           private:
            ITrace::vector error_traces(const ITrace::vector& intraces) const;
            std::string m_input_tag;
            std::string m_output_tag;

	    	    
        };
    }  // namespace Img
}  // namespace WireCell

#endif
