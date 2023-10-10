/**
   Utilities support the tensor data model for ICluster.
 */
#ifndef WIRECELLAUX_TENSORDMCLUSTER
#define WIRECELLAUX_TENSORDMCLUSTER

#include "WireCellIface/ICluster.h"
#include "WireCellIface/ITensor.h"
#include "WireCellIface/IAnodePlane.h"

namespace WireCell::Aux::TensorDM {


    /*
     * ICluster
     */

    /**
       Convert an ICluster to vector of ITensor.

       First ITensor will be the "cluster" then a sequence of
       "clnodeset" and then sequence of "cledgeset".
     */
    ITensor::vector as_tensors(ICluster::pointer cluster,
                               const std::string& datapath);

    /**
       Convert sequence of ITensor to ICluster.

       This is the inverse for as_tensors().

       The datapath specifies the cluster tensor which aggregates the
       others.  If empty then the first tensor of type "cluster" is
       assumed.

       The vector of anodes must be supplied in order to resolve ident
       values to their IData object representation.
    */
    ICluster::pointer as_cluster(const ITensor::vector& tens,
                                 const IAnodePlane::vector& anodes,
                                 const std::string& datapath="");
}

#endif
