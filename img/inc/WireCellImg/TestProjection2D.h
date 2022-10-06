/**
 * Test Projection2D
 */
#ifndef WIRECELL_TESTPROJECTION2D_H
#define WIRECELL_TESTPROJECTION2D_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class TestProjection2D : public Aux::Logger, public IClusterFilter, public IConfigurable {
          public:
            TestProjection2D();
            virtual ~TestProjection2D();

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

#endif /* WIRECELL_TESTPROJECTION2D_H */
