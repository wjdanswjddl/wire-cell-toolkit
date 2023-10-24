#ifndef WIRECELL_ITENSORSETCLUSTER
#define WIRECELL_ITENSORSETCLUSTER

#include "WireCellIface/ICluster.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/IFunctionNode.h"

namespace WireCell {

    /*! Interface which converts from a set of tensors to a cluster. */
    class ITensorSetCluster : public IFunctionNode<ITensorSet, ICluster> {
       public:
        typedef std::shared_ptr<ITensorSetCluster> pointer;

        virtual ~ITensorSetCluster() {}

        virtual std::string signature() { return typeid(ITensorSetCluster).name(); }

        // supply:
        // virtual bool operator()(const input_pointer& in, output_pointer& out);
    };
}  // namespace WireCell

#endif
