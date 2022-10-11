#ifndef WIRECELL_AUX_TENSORFRAME
#define WIRECELL_AUX_TENSORFRAME

#include "WireCellIface/ITensorSetFrame.h"
#include "WireCellAux/FrameTools.h"

namespace WireCell::Aux {
    class TensorFrame : public WireCell::ITensorSetFrame {
      public:
        TensorFrame();
        virtual ~TensorFrame();
        
        virtual bool operator()(const input_pointer& in, output_pointer& out);
        
    };
}

#endif
