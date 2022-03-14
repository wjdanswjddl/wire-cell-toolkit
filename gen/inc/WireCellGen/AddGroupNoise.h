// Add noise by group to the waveform 
// mailto:smartynen@bnl.gov

#ifndef WIRECELL_GEN_ADDGROUPNOISE
#define WIRECELL_GEN_ADDGROUPNOISE

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Logging.h"

#include <string>

namespace WireCell {
    namespace Gen {

        class AddGroupNoise : public IFrameFilter, public IConfigurable {
           public:
            AddGroupNoise(const std::string& model = "", const std::string& map = "", const std::string& rng = "Random");

            virtual ~AddGroupNoise();

            /// IFrameFilter
            virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            void gen_elec_resp_default();
            size_t argclosest(std::vector<float> const& vec, float value);

           private:
            IRandom::pointer m_rng;

            std::string m_spectra_file, m_map_file, m_rng_tn;
            int m_nsamples;
            double m_fluctuation;
	    double m_shift;
            double m_period;
            double m_normalization;

            int m_fft_length;

	    std::unordered_map<int,int> m_ch2grp;
	    std::unordered_map<int,std::vector<float>> m_grp2spec;
	    std::unordered_map<int,std::vector<float>> m_grp2rnd_amp;
            std::unordered_map<int,std::vector<float>> m_grp2rnd_phs;
            Waveform::realseq_t m_elec_resp_freq;

            Log::logptr_t log;
        };
    }  // namespace Gen
}  // namespace WireCell

#endif
