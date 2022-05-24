#include "WireCellAux/Spectra.h"
#include "WireCellUtil/Waveform.h"

using namespace WireCell;
using namespace WireCell::Waveform;


Aux::WaveCollector::WaveCollector(IDFT::pointer dft) : dft(dft)
{
}

void Aux::WaveCollector::add(const real_vector_t& wave)
{
    if (sum.empty()) {
        sum.resize(wave.size(), 0);
    }
    auto spec = fwd_r2c(dft, wave);
    auto amp = Waveform::magnitude(spec);
    increase(sum, amp);
    ++count;
}

Aux::WaveCollector::real_vector_t Aux::WaveCollector::mean()
{
    real_vector_t avg(sum.begin(), sum.end());
    scale(avg, 1.0/count);
    return avg;
}

Aux::WaveGenerator::WaveGenerator(IDFT::pointer dft, normal_f normal,
                                  const real_vector_t& meanspec)
    :dft(dft), meanspec(meanspec), normal(normal)
{
}

Aux::WaveGenerator::complex_vector_t Aux::WaveGenerator::spec()
{
    const size_t nsamples = meanspec.size();
    // nsamples: even->1, odd->0
    const size_t nextra = (nsamples+1)%2;
    const size_t nhalf = nsamples / 2;
            
    complex_vector_t spec(nsamples, 0);

    auto normals = normal(nsamples);

    spec[0].real(meanspec[0]*normals[0]);
    for (size_t ind=1; ind < nhalf; ++ind) {
        float mean = meanspec[ind];
        spec[ind] = std::complex(mean*normals[ind],
                                 mean*normals[ind+nhalf]);
    }
    if (nextra) {       // have Nyquist bin
        spec[nhalf+1].real(meanspec[nhalf]*normals.back());
    }
    hermitian_symmetry_inplace(spec);
    return spec;
}

Aux::WaveGenerator::real_vector_t Aux::WaveGenerator::wave()
{
    auto wave = inv_c2r(dft, spec());

    size_t nsamples = meanspec.size();
    const float mean_to_mode = sqrt(2/3.141592);
    for (size_t ind=0; ind<nsamples; ++ind) {
        wave[ind] *= mean_to_mode;
    }
    return wave;
}
