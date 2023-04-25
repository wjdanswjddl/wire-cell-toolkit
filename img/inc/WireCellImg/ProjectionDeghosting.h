/**
 * ProjectionDeghosting
 */
#ifndef WIRECELL_PROJECTIONDEGHOSTING_H
#define WIRECELL_PROJECTIONDEGHOSTING_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class ProjectionDeghosting : public Aux::Logger, public IClusterFilter, public IConfigurable {
          public:
            ProjectionDeghosting();
            virtual ~ProjectionDeghosting();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

          private:
            bool m_compare_brute_force{false};
            bool m_compare_rectangle{true};
            bool m_verbose{false};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_PROJECTIONDEGHOSTING_H */
