/** Convert from tensor data model to ICluster.
 */

#ifndef WIRECELL_AUX_TENSORCLUSTER
#define WIRECELL_AUX_TENSORCLUSTER

#include "WireCellIface/ITensorSetCluster.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellAux/Logger.h"

#include <regex>

namespace WireCell::Aux {
    class TensorCluster : public Aux::Logger,
                        public WireCell::ITensorSetCluster,
                        public WireCell::IConfigurable
    {
    public:
        TensorCluster();
        virtual ~TensorCluster();
        
        virtual WireCell::Configuration default_configuration() const;
        virtual void configure(const WireCell::Configuration& cfg);

        virtual bool operator()(const input_pointer& in, output_pointer& out);
        
      private:

        /** Config: "dpre"

            The datapath at which the tensor representing a cluster can
            be found.

            The string is interpreted as a regular expression (regex)
            and matched against the datapaths of the tensors.  This
            allows finding the cluster tensor given a path that depends
            on the cluster ident.
         */
        std::string m_dpre;
        /** Config: "anodes"

            An array of anode type-names.
        */
        IAnodePlane::vector m_anodes;

    
        size_t m_count{0};


    };
}

#endif
