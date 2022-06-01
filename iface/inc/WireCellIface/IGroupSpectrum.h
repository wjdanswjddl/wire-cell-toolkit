/**
   A Group Spectrum provides a discrete Fourier amplitude as a
   function of frequency for a given group ID and a mapping from
   channel ID to group ID.

   An example implementation is a model of the noise on a given
   group.

 */

#ifndef WIRECELL_IGROUPSPECTRUM
#define WIRECELL_IGROUPSPECTRUM

#include "WireCellUtil/IComponent.h"

namespace WireCell {

    class IGroupSpectrum : virtual public IComponent<IGroupSpectrum> {
       public:
        virtual ~IGroupSpectrum();

        /// The data type for frequency space amplitude (not power).
        /// It should be in units of [X]/[frequency] (equivalently
        /// [X]*[time]) where [X] is the unit of the equivalent
        /// waveform expressed in the time domain.  For a noise model
        /// [X] is likely [voltage].
        typedef std::vector<float> amplitude_t;

        /// Return the spectrum associated with the given group ID
        /// in suitable binning.  In the implementing component, the
        /// Binning should likely be coordinated with the rest of the
        /// application via the configuration.
        virtual const amplitude_t& group_spectrum(int groupid) const = 0;

        /// Return the group ID for the given channel ID.
        virtual int groupid(int chid) const = 0;
    };
}  // namespace WireCell

#endif
