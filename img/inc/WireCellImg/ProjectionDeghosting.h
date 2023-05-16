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
            bool m_verbose{false};
            size_t m_nchan{8256};
            size_t m_nslice{9592};

            int m_global_deghosting_cut_nparas{3};
            std::vector<double> m_global_deghosting_cut_values{3., 3000., 2000., 8., 8000., 4000., 8., 8000., 6000.};
            double m_uncer_cut{1e11};
            double m_dead_default_charge{-1e12};
            std::vector<double> m_judge_alt_cut_values{0.05, 0.33, 0.15, 0.33};

            // DEBUGONLY
            bool m_dryrun{false};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_PROJECTIONDEGHOSTING_H */
