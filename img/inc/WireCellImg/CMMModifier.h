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

	    int m_start;
	    int m_end;
	    
	    //continuity veto
	    int m_ncount_cont_ch{1};
	    std::vector<int> m_cont_ch_llimit{0};
	    std::vector<int> m_cont_ch_hlimit{0};
	    
	    // directly veto
	    int m_ncount_veto_ch{1};
	    std::vector<int> m_veto_ch_llimit{0};
	    std::vector<int> m_veto_ch_hlimit{0};

	    // dead level veto
	    int m_dead_ch_ncount{0};
	    float m_dead_ch_charge{1000};
	    int m_ncount_dead_ch{1};
	    std::vector<int> m_dead_ch_llimit{0};
	    std::vector<int> m_dead_ch_hlimit{0};

	    // organize boundary for dead channels
	    int m_dead_org{0};
	    std::vector<int> m_dead_org_llimit{0};
	    std::vector<int> m_dead_org_hlimit{0};

            size_t m_count{0};
        };
    }  // namespace Img
}  // namespace WireCell

#endif
