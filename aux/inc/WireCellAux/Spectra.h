/**
   Common utilities for handling spectra.

 */
#ifndef WIRECELL_AUX_SPECTRA
#define WIRECELL_AUX_SPECTRA

#include "WireCellAux/DftTools.h"
#include "WireCellUtil/Math.h"
#include <vector>
#include <functional>

namespace WireCell::Aux::Spectra {


    using real_vector_t = std::vector<float>;
    using complex_vector_t = std::vector< std::complex<float> >;
    

    /** Wave Collector
         
        This is a small helper to which you may progressively of
        waveforms of a common size and produce a mean spectrum.
    */
    class WaveCollector {
      public:

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

        The mean spectral amplitude must span the full, regularly
        sampled frequency range from 0 to F_max=2*F_Nyquist.  The
        spectral sampling will determine the waveform sampling.  If
        your spectrum sampling differs from your desired waveform
        sampling see the linterp/irrterp functions from Interpolate.h.

        See WireCell::Aux::Normals::Recycling or FreshNormals from
        RandTools.h for examples of what can be provided for the
        normal_f function object.
    */
    class WaveGenerator {
      public:

        // Something that returns n random samplings from normal
        // Gaussian distribution N(mu=0,sigma=1).  See RandTools.h for
        // some examples to use.
        using normal_f = std::function<real_vector_t(size_t n)>;

        WaveGenerator(IDFT::pointer dft, normal_f normal);                      

        // Generate a complex, Hermitian-symmetric spectrum from the
        // mean spectrum.
        complex_vector_t spec(const real_vector_t& meanspec);

        // Generate a real, interval-space waveform.
        real_vector_t wave(const real_vector_t& meanspec);

      private:
        IDFT::pointer dft;
        normal_f normal;
    };
}

#endif
