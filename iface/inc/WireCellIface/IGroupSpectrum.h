/**
   A Group Spectrum provides a discrete Fourier amplitude as a
   function of frequency for a given group ID and a mapping from
   channel ID to group ID.

 */

#ifndef WIRECELL_IGROUPSPECTRUM
#define WIRECELL_IGROUPSPECTRUM

#include "WireCellUtil/IComponent.h"

namespace WireCell {

    class IGroupSpectrum : virtual public IComponent<IGroupSpectrum> {
       public:
        virtual ~IGroupSpectrum();

        /// The spectrum type.
        typedef std::vector<float> amplitude_t;

        /// Return a regularly sampled spectrum for the channel ID.
        virtual const amplitude_t& group_spectrum(int groupid) const = 0;

        /// Return the group ID for the given channel ID.
        virtual int groupid(int chid) const = 0;
    };
}  // namespace WireCell

#endif
