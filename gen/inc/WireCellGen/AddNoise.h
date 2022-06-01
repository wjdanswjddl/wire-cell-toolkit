// This adds noise to traces of its input frame to make a new output
// frame.  It should be given voltage-level input.  The model
// determins the noise waveform length so should be made to coincide
// with the length of input waveforms.  Thus, this component works
// only on rectangular, dense frames

#ifndef WIRECELL_GEN_ADDNOISE
#define WIRECELL_GEN_ADDNOISE

#include "WireCellGen/Noise.h"

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IGroupSpectrum.h"

namespace WireCell::Gen {


    // Note, for historical reasons, this will also be known by the
    // "AddNoise" typename in configuration.
    class IncoherentAddNoise : public NoiseBaseT<IChannelSpectrum>,
                               public IFrameFilter
    {
      public:
        IncoherentAddNoise();
        virtual ~IncoherentAddNoise();

        /// IFrameFilter
        virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);
    };

    class CoherentAddNoise : public NoiseBaseT<IGroupSpectrum>,
                             public IFrameFilter
    {

      public:
        CoherentAddNoise();
        virtual ~CoherentAddNoise();

        /// IFrameFilter
        virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);
    };


}  // namespace WireCell::Gen

#endif
