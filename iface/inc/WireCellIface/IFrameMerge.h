#ifndef WIRECELL_IFRAMEMERGE
#define WIRECELL_IFRAMEMERGE

#include "WireCellIface/IHydraNode.h"
#include "WireCellIface/IFrame.h"

namespace WireCell {

    class IFrameMerge : public IHydraNodeVT<IFrame, std::tuple<IFrame>> {
      public:
        virtual ~IFrameMerge();
    
        virtual std::string signature() {
            return typeid(IFrameMerge).name();
        }
    };

}

#endif
