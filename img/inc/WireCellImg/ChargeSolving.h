/** ChargeSolving solves for charge in blobs.

    This attempts to faithfully implement the algorithms from the
    original Wire-Cell Prototype version:
    
    https://github.com/BNLIF/wire-cell-2dtoy/blob/master/src/ChargeSolving.cxx

    It includes these features:

    - uses channel charge and its uncertainty.

    - configurable threshold on the amount of charge for a measurement to be considered.

    - uses b-m connectivity to separate the solving to smaller problems.

    - solving uses b-weights based on b-b connectivity.

    - a Cholsky decomposition "whitening" of the matrices to accomodate LassoModel solver issues.

    https://en.wikipedia.org/wiki/Whitening_transformation

 */
#ifndef WIRECELL_CHARGESOLVING_H
#define WIRECELL_CHARGESOLVING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class ChargeSolving : public Aux::Logger, public IClusterFilter, public IConfigurable {
           public:
            ChargeSolving();
            virtual ~ChargeSolving();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

           private:

            // config.  only measures with more than this much charge
            // are considered.
            double m_minimum_measure{0.0};
            // config.  the tolerance passed to LassoModel solver.
            double m_lasso_tolerance{1e-3};
            // config.  the minimum norm passed to LassoModel solver.
            double m_lasso_minnorm{1e-6};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_CHARGESOLVING_H */
