/** IWaveformMap - a service-like interface providing IWaveform instances
    
    It provides a method which returns an IWaveform associated to some
    name.
*/

#ifndef WIRECELL_IFACE_IWAVEFORMMAP
#define WIRECELL_IFACE_IWAVEFORMMAP

#include "WireCellIface/IWaveform.h"
#include "WireCellUtil/IComponent.h"

#include <string>

namespace WireCell {
    class IWaveformMap : public IComponent<IWaveformMap> {
      public:
        virtual ~IWaveformMap();

        // Return known names which may be empty if implementation
        // does not know them a'priori.
        virtual std::vector<std::string> waveform_names() const {
            std::vector<std::string> empty;
            return empty;
        };

        // Return the named waveform or a nullptr if name is unknown.
        virtual IWaveform::pointer waveform_lookup(const std::string& name) const = 0;
    };
}

#endif
