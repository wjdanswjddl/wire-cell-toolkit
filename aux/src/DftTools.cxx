#include "WireCellAux/DftTools.h"
#include <algorithm>

#include <iostream>             // debugging


using namespace WireCell;
using namespace WireCell::Aux;

/*** helpers, both vector/array types ***/

// 1D vector
void Aux::hermitian_symmetry_inplace(Aux::complex_vector_t& spec)
{
    const size_t fullsize = spec.size();
    const size_t halfsize = fullsize/2; // integer division
    size_t extra = 0;
    if (spec.size() % 2) {           // odd, no Nyquist bin
        extra = 1;
    }
    else { // even, has a Nyquist bin.
        // Force the Nyquist bin to be real.  When user gives us
        // complex it is likely due to them making a fluctuated
        // spectra and thus best to keep amplitude and remove phase
        // instead of taking real part.  Thus abs().
        spec[halfsize] = std::abs(spec[halfsize]);
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

// 2D array
// axis=1 means along columns, ie on a per-row basis
void Aux::hermitian_symmetry_inplace(Aux::complex_array_t& spec, int axis)
{
    const size_t fullsize = axis ? spec.cols() : spec.rows();
    const size_t halfsize = fullsize/2; // integer division
    size_t extra = 0;
    if (spec.size() % 2) {           // odd, no Nyquist bin
        extra = 1;
    }
    else {              // see comments in same block above for vector
        if (axis) {
            spec.col(halfsize) = spec.col(halfsize).abs();
        }
        else {
            spec.row(halfsize) = spec.row(halfsize).abs();
        }
    }

    if (axis) {
        for (size_t ind=halfsize+extra; ind<fullsize; ++ind) {
            spec.col(ind) = spec.col(fullsize-ind).conjugate();
        }
    }
    else {
        for (size_t ind=halfsize+extra; ind<fullsize; ++ind) {
            spec.row(ind) = spec.row(fullsize-ind).conjugate();
        }
    }
}
Aux::complex_array_t Aux::hermitian_symmetry(const Aux::complex_array_t& spec, int axis)
{
    Aux::complex_array_t ret = spec;
    hermitian_symmetry_inplace(ret, axis);
    return ret;
}


/*** vector ***/

Aux::complex_vector_t Aux::fwd(const IDFT::pointer& dft, const Aux::complex_vector_t& seq)
{
    complex_vector_t ret(seq.size());
    dft->fwd1d(seq.data(), ret.data(), ret.size());
    return ret;
}

Aux::complex_vector_t Aux::fwd_r2c(const IDFT::pointer& dft, const Aux::real_vector_t& vec)
{
    complex_vector_t cvec(vec.size());
    std::transform(vec.begin(), vec.end(), cvec.begin(),
                   [](float re) { return Aux::complex_t(re,0.0); } );
    return fwd(dft, cvec);
}

Aux::complex_vector_t Aux::inv(const IDFT::pointer& dft, const Aux::complex_vector_t& spec)
{
    complex_vector_t ret(spec.size());
    dft->inv1d(spec.data(), ret.data(), ret.size());
    return ret;
}

Aux::real_vector_t Aux::inv_c2r(const IDFT::pointer& dft, const Aux::complex_vector_t& spec)
{
    // fixme: in future, expand IDFT to have c2r 
    complex_vector_t symspec = hermitian_symmetry(spec);
    auto cvec = inv(dft, symspec);
    real_vector_t rvec(cvec.size());
    std::transform(cvec.begin(), cvec.end(), rvec.begin(),
                   [](const Aux::complex_t& c) { return std::real(c); });
    return rvec;
}

/*** array ***/

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


Aux::complex_array_t Aux::fwd_r2c(const IDFT::pointer& dft, const Aux::real_array_t& wave, int axis)
{
    complex_array_t cwave = wave.cast<complex_t>();
    return fwd(dft, cwave, axis);
}

Aux::real_array_t Aux::inv_c2r(const IDFT::pointer& dft, const Aux::complex_array_t& spec, int axis)
{
    complex_array_t symspec = hermitian_symmetry(spec, axis);
    complex_array_t cwave = inv(dft, symspec, axis);
    // Drops the small imaginary that is accrued due to round-off errors.
    return cwave.real();        
}



/*** high level functions ***/

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

