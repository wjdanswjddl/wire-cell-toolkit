#ifndef WIRECELL_GEN_GROUPNOISEMODEL
#define WIRECELL_GEN_GROUPNOISEMODEL

#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IConfigurable.h"
#include <unordered_map>

namespace WireCell::Gen {

    class GroupNoiseModel : public IChannelSpectrum, public IConfigurable
    {

      public:
        virtual ~GroupNoiseModel();

        /// IChannelSpectrum
        virtual const amplitude_t& operator()(int chid) const;

        /// IConfigurable
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

      private:

        std::unordered_map<int, int> m_ch2grp;
        std::unordered_map<int, amplitude_t> m_grp2amp;
    };

}

#endif
