/**
   Add a set of traces for charge error, which is estimated based on the ROI length
   Pass on other parts of the frame
 */

#ifndef WIRECELLSIGPROC_CHARGEERRORFRAMEESTIMATOR
#define WIRECELLSIGPROC_CHARGEERRORFRAMEESTIMATOR

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellIface/IAnodePlane.h"

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

            // This will map from plane index to a vector of ROI errors indexed by
            // ROI duration according to the binning.
            std::vector<std::vector<float>> m_errs;

            // The binning that the above is using.  This will be subject to user
            // configuration and used to interpolate the IWaveforms.
            Binning m_bins;

            // We will only process one set of input tagged traces and produce one
            // set of output tagged traces.  User may create a small subgraph
            // bound by FrameFanout and FrameFanin to operate on other tagged
            // trace sets as well as pass-through the input tagged traces.
            std::string m_intag{"gauss"};
            std::string m_outtag{"gauss_error"};

	    int m_rebin{4};
	    std::vector<float> m_fudge_factors{1,1,1};
	    std::pair<float,float> m_time_limits{12,800};
	    
            // Needed for channel lookups
            IAnodePlane::pointer m_anode{nullptr};

        };
    }  // namespace Img
}  // namespace WireCell

#endif
