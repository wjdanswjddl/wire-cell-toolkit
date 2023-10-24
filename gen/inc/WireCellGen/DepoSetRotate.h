//  This modifies coordinate of depos, in particular, when depo positions
//  from upstream simulation (eg, LarSoft) has a non-X drift direction,
//  one can rotate the coordinate and still has a X-drift internally in
//  wire-cell-toolkit, and later rotate back to the original coordinate
//  convention after the "drifter".
//
//  The current implementation allows a transpose of three axes as well as
//  a scaling to mimic a rotation, eg, a transpose of [1,0,2] switches x-
//  and y-axis, a scale of [1,-1, 1] maps y to minus-y. A combination of
//  above two operations is equivalent to a 90-deg rotation around z-axis.
//
//  A more general rotation can be described in matrix, while the
//  computation is expensive and can be implemented if needed.
//

#ifndef WIRECELLGEN_DEPOSETROTATE
#define WIRECELLGEN_DEPOSETROTATE

#include "WireCellIface/IDepoSetFilter.h"
#include "WireCellIface/INamed.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell::Gen {

    class DepoSetRotate : public Aux::Logger,
                           public IDepoSetFilter, public IConfigurable {     
      public:

        DepoSetRotate();
        virtual ~DepoSetRotate();

        // IDepoSetFilter
        virtual bool operator()(const input_pointer& in, output_pointer& out);

        /// WireCell::IConfigurable interface.
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

      private:

        // Define how to transpose and scale coordinates
        bool m_rotate{false};
        std::tuple<int, int, int> m_transpose{0,1,2};
        std::tuple<double, double, double> m_scale{1.,1.,1.};

        size_t m_count{0};

    };

}



#endif
