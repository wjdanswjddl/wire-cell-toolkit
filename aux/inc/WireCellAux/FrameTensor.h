#ifndef WIRECELL_AUX_FRAMETENSOR
#define WIRECELL_AUX_FRAMETENSOR

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameTensorSet.h"
#include "WireCellAux/FrameTools.h"

namespace WireCell::Aux {
    class FrameTensor : public WireCell::IConfigurable, public WireCell::IFrameTensorSet {
      public:
        FrameTensor();
        virtual ~FrameTensor();
        
        virtual bool operator()(const input_pointer& in, output_pointer& out);
        
        virtual WireCell::Configuration default_configuration() const;
        virtual void configure(const WireCell::Configuration& config);
        
        // Config: "mode".  Set the mode to map frame to tensor to one
        // of "sparse", "monolithic" or "tagged".
        FrameTensorMode m_mode;

        // Config: "truncate".  If true, store trace as short ints,
        // otherwise as floats.
        bool m_truncate{false};


    };
}

#endif
