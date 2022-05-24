// A toy noise sim that makes a round trip.

#include "WireCellAux/DftTools.h"
#include "WireCellAux/RandTools.h"
#include "WireCellAux/Spectra.h"

#include "WireCellIface/IConfigurable.h"

#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/TimeKeeper.h"

#include <map>
#include <random>

using namespace WireCell;
using namespace WireCell::Aux;
using namespace WireCell::Aux::RandTools;
using namespace WireCell::Aux::Spectra;
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

// Pretend to "digitize" a spectrum from some printed plot by
// selecting values at random points.  Number of points presumably ifs
// far smaller than eventually required to make waveforms.
points_t digitize_spectrum(IRandom::pointer rng, size_t npoints=128, float Fnyquist=1.0)
{
    points_t ret;
    for (size_t count=0; count<npoints; ++count) {
        float freq = rng->uniform(0, Fnyquist);
        float amp = fictional_spectrum(freq, Fnyquist);
        ret[freq] = amp;
    }
    return ret;
}



real_vector_t make_true(IRandom::pointer rng,
                        size_t nsamples=1024, size_t npoints=128, float Fnyquist=1.0)
{
    real_vector_t freqs(nsamples);
    float dfreq = 2*Fnyquist/nsamples;
    for (size_t ind=0; ind<nsamples; ++ind) {
        freqs[ind] = dfreq * ind;
    }
    auto sampled_spectrum = digitize_spectrum(rng, npoints, Fnyquist);
    auto true_spectrum = sample_spectrum(sampled_spectrum, nsamples, Fnyquist);
    return true_spectrum;
}

// This is a copy of what was once in gen/src/Noise.cxx
static complex_vector_t
old_generate_spectrum(const std::vector<float>& spec, IRandom::pointer rng, double replace=0.02)
{
    // reuse randomes a bit to optimize speed.
    static std::vector<double> random_real_part;
    static std::vector<double> random_imag_part;

    if (random_real_part.size() != spec.size()) {
        random_real_part.resize(spec.size(), 0);
        random_imag_part.resize(spec.size(), 0);
        for (unsigned int i = 0; i < spec.size(); i++) {
            random_real_part.at(i) = rng->normal(0, 1);
            random_imag_part.at(i) = rng->normal(0, 1);
        }
    }
    else {
        const int shift1 = rng->uniform(0, random_real_part.size());
        // replace certain percentage of the random number
        const int step = 1. / replace;
        for (int i = shift1; i < shift1 + int(spec.size()); i += step) {
            if (i < int(spec.size())) {
                random_real_part.at(i) = rng->normal(0, 1);
                random_imag_part.at(i) = rng->normal(0, 1);
            }
            else {
                random_real_part.at(i - spec.size()) = rng->normal(0, 1);
                random_imag_part.at(i - spec.size()) = rng->normal(0, 1);
            }
        }
    }

    const int shift = rng->uniform(0, random_real_part.size());

    complex_vector_t noise_freq(spec.size(), 0);

    for (int i = shift; i < int(spec.size()); i++) {
        const double amplitude = spec.at(i - shift) * sqrt(2. / 3.1415926);  // / units::mV;
        noise_freq.at(i - shift).real(random_real_part.at(i) * amplitude);
        noise_freq.at(i - shift).imag(random_imag_part.at(i) * amplitude);  //= complex_t(real_part,imag_part);
    }
    for (int i = 0; i < shift; i++) {
        const double amplitude = spec.at(i + int(spec.size()) - shift) * sqrt(2. / 3.1415926);
        noise_freq.at(i + int(spec.size()) - shift).real(random_real_part.at(i) * amplitude);
        noise_freq.at(i + int(spec.size()) - shift).imag(random_imag_part.at(i) * amplitude);
    }

    return noise_freq;
}


void test_time(IRandom::pointer rng,
               IDFT::pointer dft,
               size_t nsamples=1024, // frequency bins / ticks
               size_t nwaves = 100000, // number of wavesforms to generate
               size_t npoints = 128,
               float Fnyquist=1.0)   // Nyquist frequency
{
    Normals::Recycling rn(rng);
    Normals::Fresh fn(rng);

    auto true_spectrum = make_true(rng, nsamples, npoints, Fnyquist);
    WaveGenerator rwgen(dft, rn);
    WaveGenerator fwgen(dft, fn);

    TimeKeeper tk("wave generator");
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        auto spec = old_generate_spectrum(true_spectrum, rng);
        auto wave = Aux::inv_c2r(dft, spec);
        wave.clear();
    }
    tk("old noise");

    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        auto junk = rwgen.wave(true_spectrum);
        junk.clear();
    }
    tk("recycling");

    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        auto junk = fwgen.wave(true_spectrum);
        junk.clear();
    }
// before closure:
// default
// TICK: 0 ms (this: 0 ms) wave generator
// TICK: 183 ms (this: 183 ms) recycling
// TICK: 749 ms (this: 565 ms) fresh
// 
// twister
// TICK: 0 ms (this: 0 ms) wave generator
// TICK: 186 ms (this: 186 ms) recycling
// TICK: 1027 ms (this: 841 ms) fresh

// after closure:
// default
// TICK: 0 ms (this: 0 ms) wave generator
// TICK: 182 ms (this: 182 ms) recycling
// TICK: 523 ms (this: 340 ms) fresh

// twister
// TICK: 0 ms (this: 0 ms) wave generator
// TICK: 182 ms (this: 182 ms) recycling
// TICK: 653 ms (this: 471 ms) fresh
    
// 1.6x speedup for default fresh.

    tk("fresh");
    std::cerr << tk.summary() << std::endl;


}



std::ostream& doit(std::ostream& os,     // dump results
                   IRandom::pointer rng,
                   IDFT::pointer dft,
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

    // We want a ring buffer size which is larger than and coprime
    // with the size we will eventually sample.  In this way each
    // sampling will be at a "random" starting point.
    const size_t not_nsamples = nearest_coprime(2*nsamples, 1.7*nsamples);
    // const size_t not_nsamples = nsamples;
    const double replacement_fraction = 0.02;
    const size_t nexample = 100;
    {
        Normals::Recycling rn(rng, not_nsamples, replacement_fraction);
        WaveGenerator wavegen(dft, rn);
        for (size_t iwave=0; iwave<nexample; ++iwave) {
            auto wave = wavegen.wave(true_spectrum);
            Stream::write(os, format("recycled_wave%03d.npy", iwave), wave);
        }

        WaveCollector wavecol(dft);
        for (size_t iwave=0; iwave<nwaves; ++iwave) {
            wavecol.add(wavegen.wave(true_spectrum));
        }
        Stream::write(os, "recycled_spectrum.npy", wavecol.mean());
    }
    {
        Normals::Fresh fn(rng);
        WaveGenerator wavegen(dft, fn);
        for (size_t iwave=0; iwave<nexample; ++iwave) {
            auto wave = wavegen.wave(true_spectrum);
            Stream::write(os, format("fresh_wave%03d.npy", iwave), wave);
        }

        WaveCollector wavecol(dft);
        for (size_t iwave=0; iwave<nwaves; ++iwave) {
            wavecol.add(wavegen.wave(true_spectrum));
        }
        Stream::write(os, "fresh_spectrum.npy", wavecol.mean());
    }
    { // old noise
        for (size_t iwave=0; iwave<nexample; ++iwave) {
            auto spec = old_generate_spectrum(true_spectrum, rng);
            auto wave = Aux::inv_c2r(dft, spec);
            Stream::write(os, format("oldnoise_wave%03d.npy", iwave), wave);
        }
        WaveCollector wavecol(dft);
        for (size_t iwave=0; iwave<nwaves; ++iwave) {
            auto spec = old_generate_spectrum(true_spectrum, rng);
            auto wave = Aux::inv_c2r(dft, spec);
            wavecol.add(wave);
        }
        Stream::write(os, "oldnoise_spectrum.npy", wavecol.mean());
    }

    return os;
}

IRandom::pointer getran(std::string name)
{
    auto rngcfg = Factory::lookup<IConfigurable>("Random", name);
    auto cfg = rngcfg->default_configuration();
    cfg["generator"] = name;    // default or twister
    rngcfg->configure(cfg);
    return Factory::lookup<IRandom>("Random", name);
}

int main(int argc, char* argv[])
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellGen");
    auto rng = getran("default");
    auto dft = Factory::lookup<IDFT>("FftwDFT");

    std::string outfile = argv[0];
    outfile += ".tar";          // fixme: need PR #163 merged to save as .npz
    if (argc > 1) {
        outfile = argv[1];
    }

    Stream::filtering_ostream out;
    Stream::output_filters(out, outfile);
    
    const size_t nsamples = 1024;
    const size_t npoints = 128;
    const size_t nwaves = 2000;
    const float Fnyquist = 1.0;

    doit(out, rng, dft, nsamples, npoints, nwaves, Fnyquist);

    // Note, I temporarily defined all those that the C++ standard
    // gives us and default, which seems to be twister 64 is
    // fastelst., "mt19937_64", "minstd0", "minstd", "ranlux24",
    // "ranlux24_base", "ranlux48", "ranlux48_base", "knuth"
    std::vector<std::string> rnames = {
        "default", "twister"
    };
    const size_t nwaves_speed = 10000;
    for (const auto& rname: rnames) {
        auto rng2 = getran(rname);
        std::cerr << rname << std::endl;
        test_time(rng2, dft, nsamples, nwaves_speed, npoints, Fnyquist);
    }

    out.pop();
    std::cerr << "wirecell-test noise " << outfile << " test_noise.pdf" << std::endl;
    return 0;
}
