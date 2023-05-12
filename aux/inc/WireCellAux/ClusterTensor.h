#ifndef WIRECELL_AUX_CLUSTERTENSOR
#define WIRECELL_AUX_CLUSTERTENSOR

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IClusterTensorSet.h"
#include "WireCellAux/TensorDM.h"
#include "WireCellAux/Logger.h"

namespace WireCell::Aux {
    class ClusterTensor : public Aux::Logger,
                        public WireCell::IConfigurable,
                        public WireCell::IClusterTensorSet
    {
      public:
        ClusterTensor();
        virtual ~ClusterTensor();
        
        virtual bool operator()(const input_pointer& in, output_pointer& out);
        
        virtual WireCell::Configuration default_configuration() const;
        virtual void configure(const WireCell::Configuration& cfg);
        
        /** Config: "datapath"

            Set the datapath for the tensor representing the cluster.
            The string may provide a %d format code which will be
            interpolated with the cluster's ident number.
         */
        std::string m_datapath = "clusters/%d";

        size_t m_count{0};

    };
}

#endif
