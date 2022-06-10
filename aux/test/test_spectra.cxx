
#include "WireCellAux/Spectra.h"
#include "WireCellAux/Testing.h"
#include "WireCellAux/RandTools.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"

#include <iostream>

using namespace WireCell;
using namespace WireCell::String;
using namespace WireCell::Aux::Testing;
using namespace WireCell::Aux::RandTools;
using namespace WireCell::Aux::Spectra;

struct Params {
    IRandom::pointer rng;
    IDFT::pointer dft;
    Fresh fn;
    Fresh fu;

    Params() 
        : rng(get_random())
        , dft(get_dft())
        , fn(Normals::make_fresh(rng))
        , fu(Uniforms::make_fresh(rng))
    {
    }
};

static void write_nc(std::ostream& os,  NoiseCollector& nc,
                     const std::string& name="default")
{
    Stream::write(os, format("amp_%s.npy", name), nc.amplitude());
    Stream::write(os, format("lin_%s.npy", name), nc.linear());
    Stream::write(os, format("sqr_%s.npy", name), nc.square());
    Stream::write(os, format("rms_%s.npy", name), nc.rms());
    Stream::write(os, format("per_%s.npy", name), nc.periodogram());
    Stream::write(os, format("bac_%s.npy", name), nc.bac());
    Stream::write(os, format("sac_%s.npy", name), nc.sac());
    Stream::write(os, format("psd_%s.npy", name), nc.psd());
    // also wav_<name>_<NNN>.npy 
}

static void white_noise(
    std::ostream& os,    
    Fresh& fn,                  // normals
    const std::string& name, // for saving waves
    size_t nticks,           // size of waves
    size_t nwaves,           // number of waves
    NoiseCollector& ncout)
{
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        real_vector_t wave = fn(nticks);
        ncout.add(wave.begin(), wave.end());
        std::string aname = format("wav_%s_%03d.npy", name, iwave);
        Stream::write(os, aname, wave);
    }
    write_nc(os, ncout, name);
}

static void generate_noise(
    std::ostream& os,
    NoiseGenerator& ng,
    const std::string& name, // for saving waves
    size_t nticks,           // size of waves
    size_t nwaves,           // number of waves
    const real_vector_t& spec, // noise spectrum
    NoiseCollector& ncout)
{
    for (size_t iwave=0; iwave<nwaves; ++iwave) {
        real_vector_t wave = ng.wave(spec);
        wave.resize(nticks, 0);
        ncout.add(wave.begin(), wave.end());
        std::string aname = format("wav_%s_%03d.npy", name, iwave);
        Stream::write(os, aname, wave);
    }
    write_nc(os, ncout, name);
}

void doit(std::ostream& out, Params& params, size_t nticks, bool cycle)
{
    std::string cyclic = cycle ? "cyclic" : "acyclic";
    std::string name = format("%s%d", cyclic, nticks);
    const size_t nsamples = nticks * (cycle ? 1 : 2);
    const size_t nwaves = nticks; // optimize resolution + stability

    // Make white noise from independent Gaussians with sigma=1
    const std::string name1 = name + "w";
    NoiseCollector nc1(params.dft, nsamples, true);
    white_noise(out, params.fn, name1, nticks, nwaves, nc1);
    real_vector_t dspec = nc1.amplitude();

    // nc's spectrum() is now equivalent to a WCT "noise file"
    // spectrum (though, noise files may have interpolated spectra and
    // should carry an "nticks" value so the spectra may be properly
    // normalized).  In any case, now we form yet another NC to check
    // round trip.
    const std::string name2 = name + "r";
    NoiseCollector nc2(params.dft, nsamples, true);
    NoiseGeneratorU ng(params.dft, params.fu);
    generate_noise(out, ng, name2, nticks, nwaves, dspec, nc2);

}    

int main(int argc, char* argv[])
{
    Stream::filtering_ostream out;
    std::string outfile = argc > 1 ? argv[1] : std::string(argv[0]) + ".tar";
    Stream::output_filters(out, outfile);

    Params params;
    assert(params.dft);

    const size_t nticks = 1 << 7;

    doit(out, params, nticks, false);

    doit(out, params, nticks, true);

    out.pop();
    std::cerr << "wirecell-test plot -n spectra " << outfile << " test_spectra.pdf" << std::endl;
    return 0;
}
