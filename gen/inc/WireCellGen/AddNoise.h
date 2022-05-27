// This adds noise to traces of its input frame to make a new output
// frame.  It should be given voltage-level input.  The model
// determins the noise waveform length so should be made to coincide
// with the length of input waveforms.  Thus, this component works
// only on rectangular, dense frames

#ifndef WIRECELL_GEN_ADDNOISE
#define WIRECELL_GEN_ADDNOISE

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IDFT.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellAux/Logger.h"

#include <string>

namespace WireCell::Gen {


    class AddNoise : public Aux::Logger,
                     public IFrameFilter, public IConfigurable {
      public:
        AddNoise();

        virtual ~AddNoise();

        /// IFrameFilter
        virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

        /// IConfigurable
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

      private:
        IRandom::pointer m_rng;
        IDFT::pointer m_dft;
        std::map<std::string, IChannelSpectrum::pointer> m_models;

        size_t m_nsamples{9600};
        double m_rep_percent{0.02};

        size_t m_count{0};
    };

}  // namespace WireCell::Gen

#endif
