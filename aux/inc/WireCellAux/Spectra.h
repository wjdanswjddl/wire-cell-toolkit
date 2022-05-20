/**
   Common utilities for handling spectra.

 */
#ifndef WIRECELL_AUX_SPECTRA
#define WIRECELL_AUX_SPECTRA

#include "WireCellAux/DftTools.h"
#include "WireCellUtil/Math.h"
#include <vector>
#include <functional>

namespace WireCell::Aux {

    /** Wave Collector
         
        This is a small helper to which you may progressively of
        waveforms of a common size and produce a mean spectrum.
    */
    class WaveCollector {
      public:

        using real_vector_t = std::vector<float>;


        WaveCollector(IDFT::pointer dft);

        /// Add a waveform's contribution to the spectrum.
        void add(const real_vector_t& wave);

        // Return the full domain, real-valued mean spectral amplitude
        // so far collected.
        real_vector_t mean();

      private:
        IDFT::pointer dft;
        real_vector_t sum;
        size_t count{0};
        
    };
     

    /** Wave Generator

        From a mean spectra, generate waves
    */
    class WaveGenerator {
      public:

        using real_vector_t = std::vector<float>;
        using complex_vector_t = std::vector< std::complex<float> >;

        // Something that returns n random samplings from normal
        // Gaussian distribution N(mu=0,sigma=1).  See RandTools.h for
        // some examples to use.
        using normal_f = std::function<real_vector_t(size_t n)>;

        /// The meanspec provides the mean spectral amplitude spanning
        /// the full frequency range from 0 to Fmax=2*FNyquist.  Note,
        /// this sampling determines that of the generated waves.  See
        /// linterp/irrterp functions if your spectrum's sampling
        /// differs from what wave sampling you want.
        WaveGenerator(IDFT::pointer dft, normal_f normal,
                      const real_vector_t& meanspec);

        // Generate a complex, Hermitian-symmetric spectrum from the
        // mean spectrum
        complex_vector_t spec();

      private:
        IDFT::pointer dft;
        const real_vector_t& meanspec;
        normal_f normal;
    };
}

#endif
