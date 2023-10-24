/**
   Common utilities for handling spectra.

 */
#ifndef WIRECELL_AUX_SPECTRA
#define WIRECELL_AUX_SPECTRA

#include "WireCellAux/DftTools.h"
#include "WireCellUtil/Math.h"

#include <vector>
#include <functional>

#include <algorithm>

namespace WireCell::Aux::NoiseTools {


    using real_vector_t = std::vector<float>;
    using complex_vector_t = std::vector< std::complex<float> >;
    
    // Return size which optimizes for both spectral resolution and
    // stability given nwaves waveforms of size nticks.
    inline
    size_t optimum_size(size_t nwaves, size_t nticks) {
        return round(log2(sqrt(nwaves*nticks)));
    }

    /** Noise Collector
         
        A noise collector accepts a number of time-series waveforms.
        The phase information in a waveform is assumed to be
        unimportant (eg, it may be uniformly random from sample to
        sample).  That is, the waveforms represent "noise".  

        The collector will maintain growing estimates of various
        quantities relevant for digital signal processing analysis.

        The size (called "nticks") of the provided time-series
        waveforms may (and typically will) differ from the number of
        frequency samples (nsamples) of the estimates as directed by
        the user.  The number of waveforms (nwaves) that will be
        provided is also important relative to nsamples.  

        The user is recommended to adhere to the following guidelines
        in selecting nticks, nsamples and nwaves.

        - nsamples >= nticks :: An overy-long waveform will be
          truncated to a size no larger than nsamples.  If smaller, it
          will be zero-padded to a size of nsamples.

        - nsamples >= 2*nticks :: Choosing nsamples to be less than
          twice the provided waveform size will result in cyclic
          aliasing.  Such aliasing will affect all but the estimate of
          the (linear) mean spectral amptlitude

        - nsamples = good FFT number :: To gain FFT speedups in
          performing the DFT, nsamples must be chosen as a "good FFT
          number".  Ideally it is 2^p, a power "p" of two but if that
          size is substnatially larger than the nominally desired size
          then defining nsamples as a product of small prime factors
          may lead to somehat faster speed.  Large prime factors will
          suffer substantial slowdown.

        - nwaves = nticks :: A balanced, simultaneous optimization of
          the spectra resolution and statistical stability is achieved
          when these two numbers are identical.

        Given nticks != nsamples in general, the spectral estimates
        become naturally interpolated in frequency space in the course
        of the fwd-DFT.  A particular spectral estimate may require
        normalization related to this interpolation depending on how
        it is to be used.  Except where indicated in method comments
        below, generally no normalization realted to interpolation is
        applied by the collector.

        In order to calculate estimates related to autocorelation
        (bac, sac), including power spectral density (psd) additional
        per-waveform DFTs are required.  These calculations are
        omitted by default and can be activated by apssing acs=true to
        the constructor or init().

        All returned estimates are of size nsamples.  
    */
    class Collector {
      public:

        // The native type for frequency space arrays.
        using spectrum_t = std::vector<float>;
        // The native type for interval space arrays
        using sequence_t = std::vector<float>;

        // Default noise collector.  Must call init() later.
        Collector();
        ~Collector();

        // Construct a noise collector of given nsamples size.  See
        // comments above regarding arguments.
        Collector(IDFT::pointer dft, size_t nsamples, bool acs=false);

        // Initialize or reinitialize a constructed collector.
        void init(IDFT::pointer dft, size_t nsamples, bool acs=false);

        /// Add time series samples.  Truncation or zero-padding will
        /// be applied to assure that all waves are same size as the
        /// first and no larger than nsamples.  The user should see
        /// above comments for chosing waveform size (referred to as
        /// "nticks").  Internally, doubles are used for accumulating
        /// the sums but the DFT and the API uses single-precision.
        /// User may provide the wave as any FP'ish type sequence.
        template<typename FPIter>
        void add(const FPIter& beg, const FPIter& end) {
            if (!m_nticks) {
                const size_t given = std::abs(std::distance(beg,end));
                m_nticks = std::min(m_nsamples, given);
            }
            std::vector<float> fwave(m_nsamples, 0);
            std::copy(beg, beg+m_nticks, fwave.begin());
            add_safe(fwave);
        }

        /// Return estimate of the spectral amplitude.
        ///
        ///   sqrt(nsamples/nticks)*<|X_k|>/nwaves
        ///
        /// The normalization assures that the amplitude may be
        /// fluctuated following a Rayleigh distribution to produce a
        /// complex magnitude and along with a uniformly random phase
        /// and the spectrum made Hermitian-symmetric, the inv-DFT may
        /// be applied to produce a new waveform consistent with the
        /// ensemble of waveforms that have been added.
        spectrum_t amplitude() const;

        /// Returns the spectrum mean Rayleigh parameters sigma_k for
        /// each frequency k.
        ///
        /// The sigmas spectrum can be directly fluctuated per
        /// Rayleigh and a uniform complex phase, inv-DFT'ed to get a
        /// properly normalized noise waveform of size nsamples.
        spectrum_t sigmas() const;

        /// Return <|X_k|> (sum/count) the (linear) mean spectral
        /// amplitude.  No normalization.
        spectrum_t linear() const;

        /// Return <|X_k|^2> (sum2/count), the "mean square amplitude
        /// spectral density" aka the power spectrum aka the DFT of
        /// the "biased auto correlation function", (x * x)(l).  No
        /// normalization.
        spectrum_t square() const;

        /// Return the element-wise square root of above.
        spectrum_t rms() const;

        /// Return the "mean periodogram": <|X_k|^2>/nticks.
        spectrum_t periodogram() const;

        /// Return the "mean biased autocorrelation" ie <bac> where
        /// for each wave, bac = inv-DFT(|DFT(x)|^2).  This returns
        /// size nsamples but only the first nticks are meaningful.
        /// Subsequent values are filled with round-off error or if
        /// nsamples was chosen small enough they will hold values due
        /// to cyclic aliasing.
        sequence_t bac() const;

        /// Return the "mean (unbiased) sample autocorrelation" ie
        /// <sac> where for each wave, sac(l) = bac(l)/(N-l).  This
        /// returns sequence of nsamples but only the first nticks are
        /// meaningful and the remaining values are zero.
        sequence_t sac() const;

        /// Return the "mean sample power spectral density ie <psd>
        /// where for each wave psd = DFT(sac).  It returns a spectrum
        /// of size nsamples.
        spectrum_t psd() const;

        /// Return the size of added waveforms (set on first add())
        size_t nticks() const { return m_nticks; }

        /// Return the size of spectrum_t return values.
        size_t nsamples() const { return m_nsamples; }

        /// Return the number of waves added.
        size_t nwaves() const { return m_nwaves; }

      private:
        void add_safe(const std::vector<float>& fwave);

        IDFT::pointer m_dft;

        size_t m_nwaves{0};
        size_t m_nticks{0};
        size_t m_nsamples{0};

        // Collect sums as doubles
        std::vector<double> m_sum, m_sum2, m_bac, m_sac, m_psd;
    };
     

    /** Noise Generator

        Geneate a fluctuated spectrum or equivalently its time series
        given a spectrum of Rayleigh parameter sigma (NOT amplitude).

        The sigma spectrum may be estimated as 

            sigma_k = sqrt(2/pi)*<|X_k|>

        or

            sigma_k = sqrt(0.5*<|X_k|^2>)

        Ie, estimated as the first or second raw momemnt of the k'th
        frequency bin of the DFT over some set of existing noise
        waveforms.

        This is an abstract base class.  See "N" and "U" variants for
        concrete implementations.
    */
    struct Generator {

        virtual ~Generator();

        // Return a fluctuated, complex X_k spectrum
        virtual complex_vector_t spec(const real_vector_t& sigma) = 0;

        // Return a fluctuated, real x_n time series
        virtual real_vector_t wave(const real_vector_t& sigma) = 0;
    };

    /** Generate via normal randoms:

        real(X_k) ~ N(0,sigma_k)
        imag(X_k) ~ N(0,sigma_k)

     */
    class GeneratorN : virtual public Generator {
      public:

        // Something that returns n random numbers from the normal
        // Gaussian distribution N(mu=0,sigma=1).  See RandTools.h for
        // some examples to use.
        using normal_f = std::function<real_vector_t(size_t n)>;

        GeneratorN(IDFT::pointer dft, normal_f normal);
        virtual ~GeneratorN();
        virtual complex_vector_t spec(const real_vector_t& sigma);
        virtual real_vector_t wave(const real_vector_t& sigma);

      private:
        IDFT::pointer dft;
        normal_f normal;
    };

    /** Generate via uniform randoms:

        |X_k| ~ R(sigma_k) ~ sigma_k*sqrt(-2ln(U(0, 1)))
        ang(X_k) ~ U(0, 2pi)

    */
    class GeneratorU : virtual public Generator {
      public:
        // Something that returns n random numbers from the uniform
        // distribution U(0,1).  See RandTools.h for some examples to
        // use.
        using uniform_f = std::function<real_vector_t(size_t n)>;

        GeneratorU(IDFT::pointer dft, uniform_f uniform);
        virtual ~GeneratorU();
        virtual complex_vector_t spec(const real_vector_t& sigma);
        virtual real_vector_t wave(const real_vector_t& sigma);

      private:
        IDFT::pointer dft;
        uniform_f uniform;
    };

}

#endif
