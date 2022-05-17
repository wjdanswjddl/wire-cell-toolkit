#include "WireCellAux/DftTools.h"
#include <algorithm>

#include <iostream>             // debugging


using namespace WireCell;
using namespace WireCell::Aux;


void Aux::hermitian_symmetry_inplace(Aux::complex_vector_t& spec)
{
    const size_t fullsize = spec.size();
    const size_t halfsize = fullsize/2; // integer division
    size_t extra = 0;
    if (spec.size() % 2) {           // odd, no Nyquist bin
        extra = 1;
    }
    for (size_t ind=halfsize+extra; ind<fullsize; ++ind) {
        spec[ind] = std::conj(spec[fullsize-ind]);
    }
}
         
Aux::complex_vector_t Aux::hermitian_symmetry(const Aux::complex_vector_t& spec)
{
    Aux::complex_vector_t ret(spec.begin(), spec.end());
    hermitian_symmetry_inplace(ret);
    return ret;
}



/*
  Big fat warning to future me: Passing by reference means the input
  array may carry the .IsRowMajor optimization for implementing
  transpose().  An extra copy would remove that complication but this
  interface tries to keep it.
 */

using ROWM = Eigen::Array<Aux::complex_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using COLM = Eigen::Array<Aux::complex_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;


template<typename trans>
Aux::complex_array_t doit(const Aux::complex_array_t& arr, trans func)
{
    const Aux::complex_t* in_data = arr.data();
    Aux::complex_vector_t out_vec(arr.rows()*arr.cols());

    // std::cerr << "dft::doit: (" << arr.rows() << "," << arr.cols() << ") IsRowMajor:" << arr.IsRowMajor << std::endl;

    if (arr.IsRowMajor) {
        func(in_data, out_vec.data(), arr.cols(), arr.rows());
        return Eigen::Map<ROWM>(out_vec.data(), arr.rows(), arr.cols());
    }

    func(in_data, out_vec.data(), arr.rows(), arr.cols());
    return Eigen::Map<COLM>(out_vec.data(), arr.rows(), arr.cols());
}

Aux::complex_array_t Aux::fwd(const IDFT::pointer& dft, const Aux::complex_array_t& arr)
{
    return doit(arr, [&](const complex_t* in_data,
                         complex_t* out_data,
                         int nrows, int ncols) {
        dft->fwd2d(in_data, out_data, nrows, ncols);
    });
}

Aux::complex_array_t Aux::inv(const IDFT::pointer& dft, const Aux::complex_array_t& arr)
{
    return doit(arr, [&](const complex_t* in_data,
                         complex_t* out_data,
                         int nrows, int ncols) {
        dft->inv2d(in_data, out_data, nrows, ncols);
    });
}


// template<typename trans>
// Aux::complex_array_t doit1b(const Aux::complex_array_t& arr, int axis, trans func)
// {
//     // We must provide a flat array with storage order such with
//     // logical axis-major ordering.
//     const Aux::complex_t* in_data = arr.data();
//     const int nrows = arr.rows(); // "logical"
//     const int ncols = arr.cols(); // shape

//     // If storage order matches "axis-major"
//     if ( (axis == 1 and arr.IsRowMajor)
//          or
//          (axis == 0 and not arr.IsRowMajor) ) {
//         Aux::complex_vector_t out_vec(nrows*ncols);
//         func(in_data, out_vec.data(), ncols, nrows);
//         if (arr.IsRowMajor) {
//             // note, returning makes a copy and will perform an actual
//             // storage order transpose.
//             return Eigen::Map<ROWM>(out_vec.data(), nrows, ncols);
//         }
//         return Eigen::Map<COLM>(out_vec.data(), nrows, ncols);
//     }
    
//     // Either we have row-major and want column-major storage order or
//     // vice versa.

//     // Here, we must copy and not use "auto" to get actual storage
//     // order transpose and avoid the IsRowMajor flip optimization.
//     COLM flipped = arr.transpose();
//     COLM got = doit1b(flipped, (axis+1)%2, func);
//     return got.transpose();
// }

// Implementation notes for fwd()/inv():
//
// - We make an initial copy to get rid of any potential IsRowMajor
//   optimization/confusion over storage order.  This suffers a copy
//   but we need to allocate return anyways.
//
// - We then have column-wise storage order but IDFT assumes row-wise
// - so we reverse (nrows, ncols) and meaning of axis.

Aux::complex_array_t Aux::fwd(const IDFT::pointer& dft, 
                              const Aux::complex_array_t& arr, 
                              int axis)
{
    Aux::complex_array_t ret = arr; 
    dft->fwd1b(ret.data(), ret.data(), ret.cols(), ret.rows(), !axis);
    return ret;
}

Aux::complex_array_t Aux::inv(const IDFT::pointer& dft,
                              const Aux::complex_array_t& arr,
                              int axis)
{
    Aux::complex_array_t ret = arr; 
    dft->inv1b(ret.data(), ret.data(), ret.cols(), ret.rows(), !axis);
    return ret;
}


Aux::real_vector_t Aux::convolve(const IDFT::pointer& dft,
                                 const Aux::real_vector_t& in1,
                                 const Aux::real_vector_t& in2)
{
    size_t size = in1.size() + in2.size() - 1;
    Aux::complex_vector_t cin1(size,0), cin2(size,0);

    std::transform(in1.begin(), in1.end(), cin1.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );
    std::transform(in2.begin(), in2.end(), cin2.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );

    dft->fwd1d(cin1.data(), cin1.data(), size);
    dft->fwd1d(cin2.data(), cin2.data(), size);

    for (size_t ind=0; ind<size; ++ind) {
        cin1[ind] *= cin2[ind];
    }

    Aux::real_vector_t ret(size);
    std::transform(cin1.begin(), cin1.end(), ret.begin(),
                   [](const complex_t& c) { return std::real(c); });
    return ret;
}

Aux::real_vector_t Aux::replace(const IDFT::pointer& dft,
                                const Aux::real_vector_t& meas,
                                const Aux::real_vector_t& res1,
                                const Aux::real_vector_t& res2)
{
    size_t sizes[3] = {meas.size(), res1.size(), res2.size()};
    size_t size = sizes[0] + sizes[1] + sizes[2] - *std::min_element(sizes, sizes + 3) - 1;

    Aux::complex_vector_t cmeas(size,0), cres1(size,0), cres2(size,0);
    std::transform(meas.begin(), meas.end(), cmeas.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );
    std::transform(res1.begin(), res1.end(), cres1.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );
    std::transform(res2.begin(), res2.end(), cres2.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );

    dft->fwd1d(cmeas.data(), cmeas.data(), size);
    dft->fwd1d(cres1.data(), cres1.data(), size);
    dft->fwd1d(cres2.data(), cres2.data(), size);

    for (size_t ind=0; ind<size; ++ind) {
        cmeas[ind] *= res2[ind]/res1[ind];
    }
    Aux::real_vector_t ret(size);
    std::transform(cmeas.begin(), cmeas.end(), ret.begin(),
                   [](const complex_t& c) { return std::real(c); });

    return ret;
}
