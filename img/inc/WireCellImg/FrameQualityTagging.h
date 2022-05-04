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

	    //
	    int m_nrebin{4};
	    int m_length_cut{12};
	    int m_time_cut{12};

	    //
	    int m_ch_threshold{100};

	    int m_n_cover_cut1{12};
	    int m_n_fire_cut1{14};
	    int m_n_cover_cut2{6};
	    int m_n_fire_cut2{6};
	    float m_fire_threshold{0.22};
	    
	    std::vector<int> m_n_cover_cut3{1200,1200,1800};
	    std::vector<float> m_percent_threshold{0.25,0.25,0.2};
	    std::vector<float> m_threshold1{300,300,360};
	    std::vector<float> m_threshold2{150,150,180};

	    int m_min_time{3180};
	    int m_max_time{7870};
	    int m_flag_corr{0};

	    float m_global_threshold{0.048};
	    
        };
    }  // namespace Img
}  // namespace WireCell

#endif
