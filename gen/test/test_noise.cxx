// A toy noise sim that makes a round trip.

#include "WireCellAux/DftTools.h"
#include "WireCellAux/FftwDFT.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Interpolate.h"

#include <map>
#include <random>

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::String;

// An ordered map from abscissa to ordinate
using points_t = std::map<float,float>;

// Given a spectrum in the form of irregularly sampled points return a
// spectrum of "nsamples" regularly sampled points.  Only frequencies
// up to the central "Fnyquist" Nyquist frequency are filled, the
// remaining are zero.  Linear interpolation and constant
// extrapolation will be performed on the input spectrum.
real_vector_t sample_spectrum(const points_t& half_spec,
                              size_t nsamples, float Fnyquist)
{
    irrterp<float> terp(half_spec.begin(), half_spec.end());
    real_vector_t ret(nsamples, 0);

    float df = 2*Fnyquist / nsamples;

    size_t nhalf = nsamples/2;
    if (nsamples % 2 == 0) {
        ++nhalf;
    }
    terp(ret.begin(), nhalf, 0, df);
    return ret;
}

// Model a made up noise spectrum
float fictional_spectrum(float freq, float Fnyquist = 1.0)
{
    // fold frequency to be between 0 and Fnyquist
    while (freq > Fnyquist) {
        freq -= 2*Fnyquist;
    }
    freq = std::abs(freq);
    return 4000*freq*freq * exp(-20*freq);
}

// Component code should NEVER have such a struct.  Once Random is
// moved to Aux we can use it instead of this quick and dirty stand
// replica.
struct RNG {
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> _freq;
    std::normal_distribution<float> _normal;

    RNG(float Fnyquist) : gen(rd()), _freq(0,Fnyquist) { }

    float frequency() { return _freq(gen); }
    float normal() { return _normal(gen); }
};

// Pretend to "digitize" a spectrum from some printed plot by
// selecting values at random points.  Number of points presumably ifs
// far smaller than eventually required to make waveforms.
points_t digitize_spectrum(RNG& rng, size_t npoints=128, float Fnyquist=1.0)
{
    points_t ret;
    for (size_t count=0; count<npoints; ++count) {
        float freq = rng.frequency();
        float amp = fictional_spectrum(freq, Fnyquist);
        ret[freq] = amp;
    }
    return ret;
}

// Generate a fluctuated spectrum with Hermitian symmetry the input
// average spectrum.  The input is assumed to span the entire
// frequency domain however only the first half is considered.
complex_vector_t generate_hermitian_spectrum(RNG& rng, const real_vector_t& avgspec)
{
    const size_t nsamples = avgspec.size();
    complex_vector_t spec(nsamples);
    size_t ndo = nsamples/2;
    if (nsamples%2 == 0) ++ndo;

    for (size_t ind=0; ind<ndo; ++ind) {
        spec[ind] = std::complex(avgspec[ind]*rng.normal(), avgspec[ind]*rng.normal());
    }
    hermitian_symmetry_inplace(spec);
    return spec;
}


// Generate a noise waveform from a fluctuated spectrum.
real_vector_t spec_to_wave(IDFT::pointer& dft, const complex_vector_t& spec)
{
    // "real" code would inject an IDFT.
    auto wave = inv_c2r(dft, spec);

    size_t nsamples = spec.size();
    const float mean_to_mode = sqrt(2/3.141592);
    for (size_t ind=0; ind<nsamples; ++ind) {
        wave[ind] *= mean_to_mode;
    }
    return wave;
}


// A progressive collector of average so we need not load all waves
// into memory.
class SpectralAverage {
    IDFT::pointer dft;
    real_vector_t total;
    size_t count{0};
  public:
    SpectralAverage(IDFT::pointer dft, size_t nsamples) : dft(dft), total(nsamples, 0) { }
    void operator()(const real_vector_t& wave) {
        // "real" code would inject an IDFT.
        auto spec = fwd_r2c(dft, wave);
        const size_t nsamples = total.size(); 
        for (size_t ind=0; ind<nsamples; ++ind) {
            total[ind] += std::abs(spec[ind]); // ortho/nsamples-norm?
        }
        ++count;
    }
    real_vector_t mean() {
        real_vector_t avg(total.begin(), total.end());
        const size_t nsamples = total.size(); 
        for (size_t ind=0; ind<nsamples; ++ind) {
            avg[ind] /= count;
        }
        return avg;
    }
};


std::ostream& doit(std::ostream& os,     // dump results
                   RNG& rng, IDFT::pointer dft,
                   size_t nsamples=1024, // frequency bins / ticks
                   size_t npoints=128,   // points in input spectrum
                   size_t nwaves = 1000, // number of wavesforms to generate
                   float Fnyquist=1.0)   // Nyquist frequency
{
    real_vector_t freqs(nsamples);
    float dfreq = 2*Fnyquist/nsamples;
    for (size_t ind=0; ind<nsamples; ++ind) {
        freqs[ind] = dfreq * ind;
    }
    Stream::write(os, "freqs.npy", freqs);

    auto sampled_spectrum = digitize_spectrum(rng, npoints, Fnyquist);
    std::vector<float> ss_freq, ss_spec;
    for (const auto& [f,a] : sampled_spectrum) {
        ss_freq.push_back(f);
        ss_spec.push_back(a);
    }
    Stream::write(os, "ss_freq.npy", ss_freq);
    Stream::write(os, "ss_spec.npy", ss_spec);

    auto true_spectrum = sample_spectrum(sampled_spectrum, nsamples, Fnyquist);
    assert(true_spectrum.size() == nsamples);
    Stream::write(os, "true_spectrum.npy", true_spectrum);

    for (size_t iwave=0; iwave<5; ++iwave) {
        auto sampled_spectrum = generate_hermitian_spectrum(rng, true_spectrum);
        auto wave = spec_to_wave(dft, sampled_spectrum);
        Stream::write(os, format("example_wave%d.npy", iwave), wave);
    }
    SpectralAverage sa(dft, nsamples);
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        auto sampled_spectrum = generate_hermitian_spectrum(rng, true_spectrum);
        auto wave = spec_to_wave(dft, sampled_spectrum);
        sa(wave);
    }
    Stream::write(os, "average_spectrum.npy", sa.mean());
    
    return os;
}


int main(int argc, char* argv[])
{

    std::string outfile = argv[0];
    outfile += ".tar";          // fixme: need PR #163 merged to save as .npz
    if (argc > 1) {
        outfile = argv[1];
    }

    Stream::filtering_ostream out;
    Stream::output_filters(out, outfile);
    
    const size_t nsamples = 1024;
    const size_t npoints = 128;
    const size_t nwaves = 1000;
    const float Fnyquist = 1.0;

    // Component code should NEVER explicitly make these kind of
    // things.  Use IDFT and IRandom instead.
    RNG rng(Fnyquist);
    auto dft = std::make_shared<FftwDFT>();

    doit(out, rng, dft, nsamples, npoints, nwaves, Fnyquist);

    out.pop();
    std::cerr << outfile << std::endl;
    return 0;
}
