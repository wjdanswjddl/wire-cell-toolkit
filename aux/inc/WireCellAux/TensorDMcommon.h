/**
   Utilities support the tensor data model common API.
 */
#ifndef WIRECELLAUX_TENSORDMCOMMON
#define WIRECELLAUX_TENSORDMCOMMON

#include "WireCellIface/ITensorSet.h"



namespace WireCell::Aux::TensorDM {

    /** Build metadata-only (array-less) tensor in the DM.
     */
    ITensor::pointer make_metadata_tensor(const std::string& datatype,
                                          const std::string& datapath,
                                          Configuration metadata = {});

    /** Return first itensor of matching datatype.
     *
     *  If datapath is not empty, further require it to match.
     *
     *  Raises ValueError if no tensor found in the set tens.
     */
    ITensor::pointer top_tensor(const ITensor::vector& tens,
                                const std::string& datatype,
                                const std::string& datapath="");


    /// Build a tensor set from set of tensors.  The tensor data model
    /// makes no requirements on the ITensorSet itself.  The user may
    /// combine tensors representing multiple objects into one set.
    ITensorSet::pointer as_tensorset(const ITensor::vector& tens,
                                     int ident = 0,
                                     const Configuration& tsetmd = Json::objectValue);


    using located_t = std::map<std::string, ITensor::pointer>;

    /// Index the tensors by their datapaths.
    located_t index_datapaths(const ITensor::vector& tens);

    /// Return first of type
    ITensor::pointer first_of(const ITensor::vector& tens,
                              const std::string& datatype);

    /// Return all tensors with a datapath matching regex pattern.
    ITensor::vector match_at(const ITensor::vector& tens,
                             const std::string& datapath);

}

#endif
