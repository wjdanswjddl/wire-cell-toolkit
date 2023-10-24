#ifndef WIRECELL_SIGPROC_UNDIGITIZER
#define WIRECELL_SIGPROC_UNDIGITIZER

#include "WireCellAux/Logger.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/ITrace.h"

namespace WireCell::SigProc {

    class Undigitizer : public Aux::Logger,
                        virtual public IFrameFilter,
                        virtual public IConfigurable
    {
      public:
        Undigitizer();
        virtual ~Undigitizer();

        // IConfigurable
        virtual WireCell::Configuration default_configuration() const;
        virtual void configure(const WireCell::Configuration& cfg);

        // IFrameFilter
        virtual bool operator()(const input_pointer& in, output_pointer& out);

      private:

        std::function<ITrace::pointer(const ITrace::pointer&)> m_dac;

        size_t m_count{0};
    };
}

#endif
