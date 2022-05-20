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
    for (size_t ind=0; ind < nhalf; ++ind) {
        float mean = meanspec[ind];
        spec[ind] = std::complex(mean*normals[ind],
                                 mean*normals[ind+nhalf]);
    }
    if (nextra) {       // have Nyquist bin
        spec[nhalf+1].real(meanspec[nhalf+1]*normals.back());
    }
    hermitian_symmetry_inplace(spec);
    return spec;
}

