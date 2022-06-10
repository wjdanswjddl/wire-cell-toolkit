/**
   Provide BOTH group and channel noise model intefaces by reading two
   types files: a "grouped spectra file" and a "channel group file".

   Typically only one or the other (group or channel) interface makes
   sense to use for a given "grouped spectra file".
 */

#ifndef WIRECELL_GEN_GROUPNOISEMODEL
#define WIRECELL_GEN_GROUPNOISEMODEL

#include "WireCellAux/Logger.h"

#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IGroupSpectrum.h"
#include "WireCellIface/IConfigurable.h"

#include <unordered_map>

namespace WireCell::Gen {

    class GroupNoiseModel : public Aux::Logger,
                            virtual public IConfigurable,
                            virtual public IChannelSpectrum,
                            virtual public IGroupSpectrum {
      public:
        using amplitude_t = std::vector<float>;

        GroupNoiseModel();
        virtual ~GroupNoiseModel();

        /// IChannelSpectrum
        virtual const amplitude_t& channel_spectrum(int chid) const;

        /// IGroupSpectrum
        virtual const amplitude_t& group_spectrum(int groupid) const;
        virtual int groupid(int chid) const;

        /// IConfigurable
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

      private:
        std::unordered_map<int, int> m_ch2grp;
        std::unordered_map<int, amplitude_t> m_grp2amp;
    };

}

#endif
