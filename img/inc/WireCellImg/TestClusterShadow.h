/**
 * Test ClusterShadow
 * https://github.com/WireCell/wire-cell-toolkit/blob/master/aux/docs/cluster-shadow.org
 */
#ifndef WIRECELL_TESTCLUSTERSHADOW_H
#define WIRECELL_TESTCLUSTERSHADOW_H

#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"

namespace WireCell {

    namespace Img {

        class TestClusterShadow : public Aux::Logger, public IClusterFilter, public IConfigurable {
          public:
            TestClusterShadow();
            virtual ~TestClusterShadow();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

          private:
        };

    }  // namespace Img

}  // namespace WireCell

#endif /* WIRECELL_TESTCLUSTERSHADOW_H */