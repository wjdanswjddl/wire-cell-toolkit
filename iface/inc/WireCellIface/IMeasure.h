/** 

    IMeasure is an identified "signal" as a (value, uncertainty) pair.

    Typically it is used to aggregate signal from a set of channels.
    See ICluster.

*/

#ifndef WIRECELL_IMEASURE
#define WIRECELL_IMEASURE

#include "WireCellIface/IChannel.h"
#include "WireCellUtil/Measurement.h"

namespace WireCell {
    
    class IMeasure : public IData<IMeasure> {
       public:
        virtual ~IMeasure();

        // An identifier for this measure.
        virtual int ident() const = 0;

        using value_t = WireCell::Measurement::float32;

        // Return "signal" measured as (central,uncertainty) value_t pair.
        virtual value_t signal() const = 0;

        /// The ID of the plane in which this measure resides
        virtual WirePlaneId planeid() const = 0;

    };

}


#endif
