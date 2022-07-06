/** IBlobDepoFill is something taking in an IBlobSet and an
 * IDepoSet and producing an IBlobSet.
 */

#ifndef WIRECELL_IBLOBDEPOFILL
#define WIRECELL_IBLOBDEPOFILL

#include "WireCellUtil/IComponent.h"
#include "WireCellIface/IJoinNode.h"
#include "WireCellIface/IBlobSet.h"
#include "WireCellIface/IDepoSet.h"

namespace WireCell {

    class IBlobDepoFill : public IJoinNode<std::tuple<IBlobSet, IDepoSet>, IBlobSet> {
       public:
        typedef std::shared_ptr<IBlobDepoFill> pointer;

        virtual ~IBlobDepoFill();

        virtual std::string signature() { return typeid(IBlobDepoFill).name(); }

        // subclass supply:
        // virtual bool operator()(const input_tuple_type& intup,
        //                         output_pointer& out);
    };

}  // namespace WireCell

#endif
