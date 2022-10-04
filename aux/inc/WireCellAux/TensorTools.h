#ifndef WIRECELL_AUX_TENSORTOOLS
#define WIRECELL_AUX_TENSORTOOLS

#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/IDFT.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/PointCloud.h"


#include <Eigen/Core>
#include <complex>

namespace WireCell::Aux {

    inline
    bool is_row_major(const ITensor::pointer& ten)
    {
        if (ten->order().empty() or ten->order()[0] == 1) {
            return true;
        }
        return false;
    }

    template<typename scalar_t>
    bool is_type(const ITensor::pointer& ten) {
        return (ten->element_type() == typeid(scalar_t));
    }


    // Extract the underlying data array from the tensor as a vector.
    // Caution: this ignores storage order hints and 1D or 2D will be
    // flattened assuming C-ordering, aka row-major (if 2D).  It
    // throws ValueError on type mismatch.
    template<typename element_type>
    std::vector<element_type> asvec(const ITensor::pointer& ten)
    {
        if (ten->element_type() != typeid(element_type)) {
            THROW(ValueError() << errmsg{"element type mismatch"});
        }
        const element_type* data = (const element_type*)ten->data();
        const size_t nelems = ten->size()/sizeof(element_type);
        return std::vector<element_type>(data, data+nelems);
    }
    
    // Extract the tensor data as an Eigen array.
    template<typename element_type>
    Eigen::Array<element_type, Eigen::Dynamic, Eigen::Dynamic> // this default is column-wise
    asarray(const ITensor::pointer& tens)
    {
        if (tens->element_type() != typeid(element_type)) {
            THROW(ValueError() << errmsg{"element type mismatch"});
        }
        using ROWM = Eigen::Array<element_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        using COLM = Eigen::Array<element_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

        auto shape = tens->shape();
        int nrows, ncols;
        if (shape.size() == 1) {
            nrows = 1;
            ncols = shape[0];
        }
        else {
            nrows = shape[0];
            ncols = shape[1];
        }

        // Eigen::Map is a non-const view of data but a copy happens
        // on return.  We need to temporarily break const correctness.
        const element_type* cdata = reinterpret_cast<const element_type*>(tens->data());
        element_type* mdata = const_cast<element_type*>(cdata);

        if (is_row_major(tens)) {
            return Eigen::Map<ROWM>(mdata, nrows, ncols);
        }
        // column-major
        return Eigen::Map<COLM>(mdata, nrows, ncols);
    }

    // PointCloud support

    /// Convert Array to ITensor.  Additional md may be provided
    ITensor::pointer as_itensor(const PointCloud::Array& array);

    /// Convert a Dataset to an ITensorSet.  The dataset metadata is
    /// checked for an "ident" attribute to set on the ITensorSet and
    /// if not found, 0 is used.
    ITensorSet::pointer as_itensorset(const PointCloud::Dataset& dataset);

    /// Convert an ITensor to an Array.  A default array is returned
    /// if the element type of the tensor is not supported.  Setting
    /// the share as true is a means of optimizing memory usage and
    /// must be used with care.  It will allow the memory held by the
    /// ITensor to be directly shared with the Array (no copy).  The
    /// user must keep the ITensor alive or call
    /// Array::assure_mutable() in order for this memory to remain
    /// valid.
    PointCloud::Array as_array(const ITensor::pointer& ten, bool share=false);

    /// Convert an ITensorSet to a Dataset.  The "ident" of the
    /// ITensorSet will be stored as the "ident" attribute of the
    /// Dataset metadata.  If a "_dataset_arrays" key is found in the
    /// ITensorSet metadata it shall provide an array of string giving
    /// names matching order and size of the array of ITensors.
    /// Otherwise, each ITensor metadata shall provide an "name"
    /// attribute.  Otherwise, an array name if invented.  The share
    /// argument is passed to as_array().
    PointCloud::Dataset as_dataset(const ITensorSet::pointer& itsptr, bool share=false);


}

#endif
