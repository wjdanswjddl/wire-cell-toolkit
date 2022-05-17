/**
   This provides std::vector and Eigen::Array typed interface to an
   IDFT.
 */

#ifndef WIRECELL_AUX_DFTTOOLS
#define WIRECELL_AUX_DFTTOOLS

#include "WireCellIface/IDFT.h"
#include <vector>
#include <Eigen/Core>

namespace WireCell::Aux {

    using complex_t = IDFT::complex_t;

    // std::vector based functions

    using real_vector_t = std::vector<float>;
    using complex_vector_t = std::vector<complex_t>;

    /// Return a version of the input with the upper half becoming the
    /// mirrored complex conjugate of the lower half.  The lower half
    /// is unchanged.  The zeroth element is not mirrored and if the
    /// input is even the element corresponding to the Nyquist bin is
    /// not mirrored.
    complex_vector_t hermitian_symmetry(const complex_vector_t& spec);

    // As above but in-place.
    void hermitian_symmetry_inplace(complex_vector_t& spec);


    // 1D with std::vector.

    // Perform forward c2c transform on vector.
    inline complex_vector_t fwd(const IDFT::pointer& dft, const complex_vector_t& seq)
    {
        complex_vector_t ret(seq.size());
        dft->fwd1d(seq.data(), ret.data(), ret.size());
        return ret;
    }

    // Perform forward r2c transform on vector.
    inline complex_vector_t fwd_r2c(const IDFT::pointer& dft, const real_vector_t& vec)
    {
        complex_vector_t cvec(vec.size());
        std::transform(vec.begin(), vec.end(), cvec.begin(),
                       [](float re) { return Aux::complex_t(re,0.0); } );
        return fwd(dft, cvec);
    }

    // Perform inverse c2c transform on vector.
    inline complex_vector_t inv(const IDFT::pointer& dft, const complex_vector_t& spec)
    {
        complex_vector_t ret(spec.size());
        dft->inv1d(spec.data(), ret.data(), ret.size());
        return ret;
    }

    // Perform inverse c2r transform on vector.  Note: this ignores
    // all frequencies in the input spectrum which are above the
    // Nyquist frequency.
    inline real_vector_t inv_c2r(const IDFT::pointer& dft, const complex_vector_t& spec)
    {
        complex_vector_t symspec = hermitian_symmetry(spec);
        auto cvec = inv(dft, symspec);
        real_vector_t rvec(cvec.size());
        std::transform(cvec.begin(), cvec.end(), rvec.begin(),
                       [](const Aux::complex_t& c) { return std::real(c); });
        return rvec;
    }

    // 1D high-level interface

    /// Convovle in1 and in2.  Returned vecgtor has size sum of sizes
    /// of in1 and in2 less one element in order to assure no periodic
    /// aliasing.  Caller need not (should not) pad either input.
    /// Caller is free to truncate result as required.
    real_vector_t convolve(const IDFT::pointer& dft,
                           const real_vector_t& in1,
                           const real_vector_t& in2);


    /// Replace response res1 in meas with response res2.
    ///
    /// This will compute the FFT of all three, in frequency space will form:
    ///
    ///     meas * resp2 / resp1
    ///
    /// apply the inverse FFT and return its real part.
    ///
    /// The output vector is long enough to assure no periodic
    /// aliasing.  In general, caller should NOT pre-pad any input.
    /// Any subsequent truncation of result is up to caller.
    real_vector_t replace(const IDFT::pointer& dft,
                          const real_vector_t& meas,
                          const real_vector_t& res1,
                          const real_vector_t& res2);


    // Eigen array based functions

    /// 2D array types.  Note, use Array::cast<complex_t>() if you
    /// need to convert rom real or arr.real() to convert to real.
    using real_array_t = Eigen::ArrayXXf;
    using complex_array_t = Eigen::ArrayXXcf;
    
    // 2D with Eigen arrays.  Use eg arr.cast<complex_>() to provde
    // from real or arr.real()() to convert result to real.

    // Full complex to complex forward transform.
    complex_array_t fwd(const IDFT::pointer& dft, const complex_array_t& arr);
    // Full complex to complex inverse transform.
    complex_array_t inv(const IDFT::pointer& dft, const complex_array_t& arr);

    // Forward transform of real to complex.
    // complex_array_t fwd_r2c(const IDFT::pointer& dft, const real_array_t& arr);

    // Perform inverse c2r transform on array. 
    // real_array_t inv_c2r(const IDFT::pointer& dft, const complex_array_t& arr, int axis=0);

    // Transform a 2D array along one axis.
    //
    // The axis identifies the logical array "dimension" over which
    // the transform is applied.  For example, axis=1 means the
    // transforms are applied along columns (ie, on a per-row basis).
    // Note: this is the same convention as held by numpy.fft.
    //
    // The axis is interpreted in the "logical" sense Eigen arrays
    // indexed as array(irow, icol).  Ie, the dimension traversing
    // rows is axis 0 and the dimension traversing columns is axis 1.
    // Note: internal storage order of an Eigen array may differ from
    // the logical order and indeed that of the array template type
    // order.  Neither is pertinent in setting the axis.
    complex_array_t fwd(const IDFT::pointer& dft, const complex_array_t& arr, int axis);
    complex_array_t inv(const IDFT::pointer& dft, const complex_array_t& arr, int axis);


    // Fixme: possible additions
    // - superposition of 2 reals for 2x speedup
    // - r2c / c2r for 1b

}

#endif
