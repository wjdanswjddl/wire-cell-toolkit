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

            // Used to hold measurement and blob values
            // (central+uncertainty).
            using value_t = ISlice::value_t;
            // Config: blob_{value,error}_threshold. A blob with
            // central value less than this is dropped.  The
            // uncertainty is not currently considered.
            value_t m_blob_thresh{0,1};
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_TESTPROJECTION2D_H */
