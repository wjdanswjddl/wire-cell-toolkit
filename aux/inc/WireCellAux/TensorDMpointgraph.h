/**
   Utilities support the tensor data model for point cloud graphs
 */
#ifndef WIRECELLAUX_TENSORDMPOINTGRAPH
#define WIRECELLAUX_TENSORDMPOINTGRAPH

#include "WireCellUtil/PointGraph.h"
#include "WireCellIface/ITensor.h"

namespace WireCell::Aux::TensorDM {


    /// PointGraph support

    /**
       Convert tensors representing point cloud graph to PointGraph.

       The tensor found at datapath is converted.  If datapath is
       empty the first pcgraph found is used.
     */
    PointGraph as_pointgraph(const ITensor::vector& tens,
                             const std::string& datapath="");

    /** Convert a point graph to tensors.

        The vector first contains the conversion of the "nodes"
        dataset with datapaths <datapath>/nodes/ followed by the
        conversion of the "edges" dataset with datapaths
        <datapath>/edges/.
    */
    ITensor::vector as_tensors(const PointGraph& pcgraph,
                               const std::string& datapath);


}

#endif
