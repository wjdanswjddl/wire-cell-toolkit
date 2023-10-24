/* Something consuming a blobset and producing a frame.
 */

#ifndef WIRECELL_IBLOBSETFRAMER
#define WIRECELL_IBLOBSETFRAMER

#include "WireCellIface/IBlobSet.h"
#include "WireCellIface/IFrame.h"
#include "WireCellIface/IFunctionNode.h"

namespace WireCell {
    class IBlobSetFramer : public IFunctionNode<IBlobSet, IFrame> {
    public:
        virtual ~IBlobSetFramer() {}

        virtual std::string signature() { return typeid(IBlobSetFramer).name(); }

        // supply:
        // virtual bool operator()(const input_pointer& in, output_pointer& out);
    };

}  // namespace WireCell

#endif
