/** A ranker returning a rank in [0,1] with 1 indicating pure "noise".

    It expects voltage-level waveforms as may be input to ADC.  This
    voltage includes effects from electronics gain and shaping.

*/

#ifndef WIRECELL_SIGPROC_NOISERANKER
#define WIRECELL_SIGPROC_NOISERANKER

#include "WireCellIface/ITraceRanker.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell :: SigProc {

    class NoiseRanker : public virtual ITraceRanker,
                        public virtual IConfigurable
    {
      public:

        NoiseRanker();
        virtual ~NoiseRanker();

        // ITraceRanker
        virtual double rank(const ITrace::pointer& trace);

        // IConfigurable
        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;

      private:

        double m_maxdev;

    };
}

#endif
