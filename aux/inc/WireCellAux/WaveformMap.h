/* WaveformMap - named waveforms from file.
*/

#ifndef WIRECELLAUX_SIGNALROIUNCERTAINTY
#define WIRECELLAUX_SIGNALROIUNCERTAINTY

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IWaveformMap.h"

#include <unordered_map>

namespace WireCell::Aux {

    class WaveformMap : public IWaveformMap, public IConfigurable {
      public:
        WaveformMap();
        virtual ~WaveformMap();


        // IWaveformMap interface
        std::vector<std::string> waveform_names() const;
        virtual IWaveform::pointer waveform_lookup(const std::string& name) const;

        /// IConfigurable interface.
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;
        
      private:
          
        std::unordered_map<std::string, IWaveform::pointer> m_wfs;

    };
}

#endif
