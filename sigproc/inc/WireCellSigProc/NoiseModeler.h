/** Model noise found in dense voltage input frames.

    Write WCT noise file at termination time.

    See Undigitizer for conversion of ADC to voltage.
 */

#ifndef WIRECELL_SIGPROC_NOISEMODELER
#define WIRECELL_SIGPROC_NOISEMODELER

#include "WireCellAux/Logger.h"
#include "WireCellAux/NoiseTools.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSink.h"
#include "WireCellIface/ITerminal.h"
#include "WireCellIface/IDFT.h"

#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

namespace WireCell::SigProc {

    class NoiseModeler : public Aux::Logger,
                         virtual public IFrameSink,
                         virtual public ITerminal,
                         virtual public IConfigurable
    {
      public:
        NoiseModeler();
        virtual ~NoiseModeler();

        // IConfigurable
        virtual WireCell::Configuration default_configuration() const;
        virtual void configure(const WireCell::Configuration& cfg);

        // IFrameSink
        virtual bool operator()(const input_pointer& vframe);

        // ITerminal
        virtual void finalize();

      private:

        std::function<bool(const ITrace::pointer&)> m_isnoise;
        IDFT::pointer m_dft;

        size_t m_count{0}, m_nhalfout{0}, m_nfft{0};
        double m_tick{0};
        std::unordered_map<int,int> m_chgrp;
        std::string m_outname;

        using Collector = WireCell::Aux::NoiseTools::Collector;
        std::unordered_map<int, Collector> m_gcol;
        Collector& nc(int chid);

    };
}

#endif
