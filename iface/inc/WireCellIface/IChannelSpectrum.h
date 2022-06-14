/**
   A Channel Spectrum provides a discrete Fourier amplitude as a
   function of frequency.
 */

#ifndef WIRECELL_ICHANNELSPECTRUM
#define WIRECELL_ICHANNELSPECTRUM

#include "WireCellUtil/IComponent.h"

namespace WireCell {

    class IChannelSpectrum : virtual public IComponent<IChannelSpectrum> {
       public:
        virtual ~IChannelSpectrum();

        /// The spectrum type.
        typedef std::vector<float> amplitude_t;

        /// Return a regularly sampled spectrum for the channel ID.
        virtual const amplitude_t& channel_spectrum(int chid) const = 0;

    };
}  // namespace WireCell

#endif
