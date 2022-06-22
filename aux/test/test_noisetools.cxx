// Test noise tools

#include "WireCellAux/NoiseTools.h"
#include "WireCellAux/Testing.h"
#include "WireCellAux/RandTools.h"
#include "WireCellAux/DftTools.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Spectrum.h"
#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"

#include <map>
#include <iostream>

using namespace WireCell;
using namespace WireCell::Spectrum;
using namespace WireCell::String;
using namespace WireCell::Aux::Testing;
using namespace WireCell::Aux::RandTools;
using namespace WireCell::Aux::DftTools;
using namespace WireCell::Aux::NoiseTools;

struct Params {
    IRandom::pointer rng;
    IDFT::pointer dft;

    // We need both normals and uniforms
    Fresh fn;
    Fresh fu;
    // Make both so....
    GeneratorN ngn;
    GeneratorU ngu;
    // ...we can test with one.
    Generator& ng;    

    Params() 
        : rng(get_random())
        , dft(get_dft())
        , fn(Normals::make_fresh(rng))
        , fu(Uniforms::make_fresh(rng))
        , ngn(dft, fn)
        , ngu(dft, fu)
        , ng(ngu)               // pick which to test
    {
    }
};

// This returns the amplitude of a purely fictional mean spectral
// amplitude which vaguely resembles some measured spectra.  It
// happens to use a Rayleigh distribribution but do not conflate this
// with the Rayleigh distributions which models fluctuations of |X_k|.
// The constant can be used to get the spectrum near some desired
// physical units.
float fictional_function(float freq, float fsigma, float constant=10.0)
{
    const auto fsigma2 = fsigma*fsigma;
    return constant * freq/fsigma2 *exp(-0.5*freq*freq/fsigma2);
}

// Pretend to "hand digitize" the fictional spectrum by sampling
// points chosen uniformaly randomly on the abscissa.
using points_t = std::map<float,float>;
points_t digitize_fictional(Fresh fu,
                            size_t npoints,
                            float fsigma = 0.2,
                            float Fnyquist=1.0)
{
    points_t ret;
    for (size_t ind=0; ind<npoints; ++ind) {
        const float freq = Fnyquist * fu();
        const float amp = fictional_function(freq, fsigma);
        ret[freq] = amp;
    }
    return ret;
}

// Given a "hand digitized" spectrum as set of (freq,amp) pairs,
// return a regularly sampled spectrum of size nsamples.  Only
// frequencies up to the central "Fnyquist" Nyquist frequency are
// filled, the remaining are filled by asserting Hermitian symmetry.
// Linear interpolation and constant extrapolation will be performed
// on the input spectrum.  This interpolation is faithful to the
// original function and no normalization is attempted.
real_vector_t sample_spectrum(const points_t& points,
                              size_t nsamples, float Fnyquist)
{
    irrterp<float> terp(points.begin(), points.end());
    real_vector_t spec(nsamples, 0);

    float df = 2*Fnyquist / nsamples;

    size_t nhalf = nsamples/2;
    if (nsamples % 2 == 0) {
        ++nhalf;                // need nyquist bin
    }
    terp(spec.begin(), nhalf, 0, df);

    // Note, this is wasteful, better if we were to implement a
    // Hermitian symmetry for reals, but I'm lazy.
    complex_vector_t cspec(spec.size());
    hermitian_mirror(spec.begin(), spec.end(), cspec.begin());

    for (size_t ind=0; ind<nsamples; ++ind) {
        spec[ind] = std::abs(cspec[ind]);
    }

    return spec;
}

// Return the sigma_k's from the fictional spectrum 
real_vector_t fictional_sigmas(Fresh fu,
                               size_t nsamples,
                               float fsigma=0.2, float Fnyquist=1.0,
                               size_t npoints=128)
{
    auto points = digitize_fictional(fu, npoints, fsigma, Fnyquist);
    auto ret = sample_spectrum(points, nsamples, Fnyquist);
    const float sqrt2opi = sqrt(2/3.141592);
    for (auto& val : ret) {
        val *= sqrt2opi;
    }
    return ret;
}


static void write_nc(std::ostream& os,  Collector& nc,
                     const std::string& name="default")
{
    Stream::write(os, format("amp_%s.npy", name), nc.amplitude());
    Stream::write(os, format("sig_%s.npy", name), nc.sigmas());
    Stream::write(os, format("lin_%s.npy", name), nc.linear());
    Stream::write(os, format("sqr_%s.npy", name), nc.square());
    Stream::write(os, format("rms_%s.npy", name), nc.rms());
    Stream::write(os, format("per_%s.npy", name), nc.periodogram());
    Stream::write(os, format("bac_%s.npy", name), nc.bac());
    Stream::write(os, format("sac_%s.npy", name), nc.sac());
    Stream::write(os, format("psd_%s.npy", name), nc.psd());
    // also wav_<name>_<NNN>.npy 
}

static void gauss_noise(
    std::ostream& os,    
    Fresh& fn,                  // normals
    const std::string& name, // for saving waves
    size_t nticks,           // size of waves
    size_t nwaves,           // number of waves
    Collector& ncout)
{
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        real_vector_t wave = fn(nticks);
        ncout.add(wave.begin(), wave.end());
        std::string aname = format("wav_%s_%03d.npy", name, iwave);
        Stream::write(os, aname, wave);
    }
    write_nc(os, ncout, name);
}

// Given a spectrum of Rayleigh sigma_k's, and a noise generator,
// generate into a noise collector and the output stream.
static void generate_noise(
    std::ostream& os,
    Generator& ng,
    const std::string& name, // for saving waves
    size_t nticks,           // size of waves
    size_t nwaves,           // number of waves
    const real_vector_t& sigmas, // Rayleigh sigmas
    Collector& ncout)
{
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        real_vector_t wave = ng.wave(sigmas);
        wave.resize(nticks, 0);
        ncout.add(wave.begin(), wave.end());
        std::string aname = format("wav_%s_%03d.npy", name, iwave);
        Stream::write(os, aname, wave);
        if (!iwave) {
            std::cerr << name << " nticks=" << nticks
                      << " wave size=" << wave.size()
                      << " sigmas size=" << sigmas.size() << "\n";
        }
    }
    write_nc(os, ncout, name);
}

// Python's wirecell.test.spectra relies on this naming convention
std::string make_name(std::string name, bool cycle, size_t nticks, size_t round)
{
    std::stringstream ss;
    ss << name << "_c" << (cycle ? 1 : 0) << "_n" << nticks << "_r" << round;
    return ss.str();
}

void gauss(std::ostream& out, Params& params, size_t nticks, bool cycle)
{
    const std::string name = "gauss";
    const size_t nsamples = nticks * (cycle ? 1 : 2);
    const size_t nwaves = nticks; // optimize resolution + stability

    // Make Gaussian noise
    const std::string name1 = make_name(name, cycle, nticks, 1);

    real_vector_t trusig(nsamples, sqrt(nticks/2)); // estimate Rayleigh sigma
    Stream::write(out, "trusig_" + name1 + "_.npy", trusig);

    Collector nc1(params.dft, nsamples, true);
    gauss_noise(out, params.fn, name1, nticks, nwaves, nc1);
    real_vector_t gsigmas = nc1.sigmas();
    std::cerr << name1 << " cycle:" << cycle
              << " nticks=" << nticks << " nsamples=" << nsamples
              << " sigma size=" << gsigmas.size() << "\n";

    const std::string name2 = make_name(name, cycle, nticks, 2);
    Collector nc2(params.dft, nsamples, true);
    generate_noise(out, params.ng, name2, nticks, nwaves, gsigmas, nc2);

}    

static real_vector_t
write_tru(std::ostream& out, std::string kind, std::string name, size_t nsamples, double val)
{
    real_vector_t arr(nsamples, val);
    Stream::write(out, "tru"+kind+"_" + name + "_.npy", arr);
    return arr;
}

void white(std::ostream& out, Params& params, size_t nticks, bool cycle)
{
    const std::string name = "white";
    const size_t nsamples = nticks * (cycle ? 1 : 2);
    const size_t nwaves = nticks; // optimize resolution + stability
    const double rms = 1.0;

    // Note, if we magically generated truth from many nticks long
    // waves, we'd get sqrt(nticks/2.0)*rms but when we use
    // nsamples>nticks we'd normalize to sqrt(nsamples/nticks).  Here
    // we jump to the chaise:
    const double target_sigma = sqrt(nsamples/2.0)*rms; // Rayleigh parameter
    const double target_energy = 2*target_sigma*target_sigma;
    std::cerr << "White: nticks=" << nticks << " nsamples=" << nsamples << " sigma=" << target_sigma <<  " E=" << target_energy << "\n";

    // Make shaped noise
    const std::string name1 = make_name(name, cycle, nticks, 1);

    auto trusig = write_tru(out, "sig", name1, nsamples, target_sigma);
    // The NC's amp is also normalized by nsamples but it is mean
    // |X_k| instead of mode/sigma |X_k| so we need the sqrt(pi/2).
    write_tru(out, "amp", name1, nsamples, sqrt(3.141592/2.0)*target_sigma);
    // NC does not correct linear for nsamples>nticks
    write_tru(out, "lin", name1, nsamples, sqrt(3.141592*0.5*0.5*nticks*rms*rms));
    // NC does not correct square
    write_tru(out, "sqr", name1, nsamples, nticks*rms*rms);


    Collector nc1(params.dft, nsamples, true);
    generate_noise(out, params.ng, name1, nticks, nwaves, trusig, nc1);
    real_vector_t gsigmas = nc1.sigmas();

    const std::string name2 = make_name(name, cycle, nticks, 2);
    Collector nc2(params.dft, nsamples, true);
    generate_noise(out, params.ng, name2, nticks, nwaves, gsigmas, nc2);
}    

void shape(std::ostream& out, Params& params, size_t nticks, bool cycle)
{
    const std::string name = "shape";
    const size_t nsamples = nticks * (cycle ? 1 : 2);
    const size_t nwaves = nticks; // optimize resolution + stability
    const size_t npoints = sqrt(nticks*2);
    const float fsigma=0.2;
    const float Fnyquist=1.0;

    // Make shaped noise
    const std::string name1 = make_name(name, cycle, nticks, 1);

    auto trusig = fictional_sigmas(params.fu, nticks,
                                   fsigma,  Fnyquist, npoints);
    Stream::write(out, "trusig_" + name1 + "_.npy", trusig);

    Collector nc1(params.dft, nsamples, true);
    generate_noise(out, params.ng, name1, nticks, nwaves, trusig, nc1);
    real_vector_t gsigmas = nc1.sigmas();

    const std::string name2 = make_name(name, cycle, nticks, 2);
    Collector nc2(params.dft, nsamples, true);
    generate_noise(out, params.ng, name2, nticks, nwaves, gsigmas, nc2);
}    

int main(int argc, char* argv[])
{
    Stream::filtering_ostream out;
    std::string outfile = argc > 1 ? argv[1] : std::string(argv[0]) + ".tar";
    Stream::output_filters(out, outfile);

    Params params;
    assert(params.dft);

    // const size_t nticks = 1 << 7;
    const size_t nticks = 1 << 8;

    gauss(out, params, nticks, false);
    gauss(out, params, nticks, true);

    white(out, params, nticks, false);
    white(out, params, nticks, true);

    shape(out, params, nticks, false);
    shape(out, params, nticks, true);

    out.pop();

    std::cerr
        << "\nNote, this test is connected with gen/docs/noise.org presentation\n"
        << "wirecell-test plot -n noisetools " << outfile << " gen/docs/test_noisetools.pdf" << std::endl;
    return 0;
}
