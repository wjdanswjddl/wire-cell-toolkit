#ifndef WIRECELL_ICLUSTERTENSORSET
#define WIRECELL_ICLUSTERTENSORSET

#include "WireCellIface/ICluster.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/IFunctionNode.h"

namespace WireCell {

    /*! An ICluster to ITensorSet converter interface. */
    class IClusterTensorSet : public IFunctionNode<ICluster, ITensorSet> {
       public:
        typedef std::shared_ptr<IClusterTensorSet> pointer;

        virtual ~IClusterTensorSet(){};

        virtual std::string signature() { return typeid(IClusterTensorSet).name(); }

        // supply:
        // virtual bool operator()(const input_pointer& in, output_pointer& out);
    };

}  // namespace WireCell

#endif
