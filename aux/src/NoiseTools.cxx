#include "WireCellAux/NoiseTools.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Spectrum.h"
#include "WireCellUtil/Interpolate.h"

#include <cmath>

using namespace WireCell;
using namespace WireCell::Waveform;
using namespace WireCell::Spectrum;
using namespace WireCell::Aux::DftTools;
using namespace WireCell::Aux::NoiseTools;


Collector::Collector()
{
}
Collector::~Collector()
{
}
Collector::Collector(IDFT::pointer dft, size_t nsamples, bool acs)
{
    init(dft, nsamples, acs);
}

void Collector::init(IDFT::pointer dft, size_t nsamples, bool acs)
{
    m_dft = dft;

    m_nwaves = 0;               // we count these
    m_nsamples = nsamples;

    m_sum.clear();
    m_sum.resize(m_nsamples, 0);
    m_sum2.clear();
    m_sum2.resize(m_nsamples, 0);

    m_bac.clear();
    m_sac.clear();
    m_psd.clear();

    if (acs) {
        m_bac.resize(m_nsamples, 0);
        m_sac.resize(m_nsamples, 0);
        m_psd.resize(m_nsamples, 0);
    }
}

#include <iostream>
void Collector::add_safe(const std::vector<float>& fwave)
{
    ++m_nwaves;

    const complex_vector_t fspec = fwd_r2c(m_dft, fwave);

    complex_vector_t sqr(m_nsamples, 0);

    // Always collect sum abs and sum of square
    for (size_t ind=0; ind < m_nsamples; ++ind) {
        const double m2 = std::norm(fspec.at(ind));
        sqr.at(ind).real(m2);
        m_sum.at(ind) += sqrt(m2);
        m_sum2.at(ind) += m2;
    }

    if (0 == m_bac.size()) {
        return;
    }

    const real_vector_t fbac = inv_c2r(m_dft, sqr);
    for (size_t ind=0; ind<m_nsamples; ++ind) {
        m_bac.at(ind) += fbac[ind];
    }

    real_vector_t fsac(m_nsamples, 0);
    for (size_t lag = 0; lag < m_nticks; ++lag) {
        double s = fbac[lag]/(m_nticks-lag);
        fsac[lag] = s;
        m_sac[lag] += s;
    }

    const complex_vector_t cpsd = fwd_r2c(m_dft, fsac);
    for (size_t ind=0; ind<m_nsamples; ++ind) {
        m_psd[ind] += std::abs(cpsd[ind]);
    }
}


static std::vector<float> normit(const std::vector<double>& vals, double norm)
{
    std::vector<float> ret(vals.size(), 0);
    for (size_t ind=0; ind<vals.size(); ++ind) {
        ret[ind] = norm*vals[ind];
    }
    return ret;
}

Collector::spectrum_t Collector::amplitude() const
{
    const double interp_norm = sqrt((double)m_nsamples/(double)(m_nticks));
    return normit(m_sum, interp_norm/m_nwaves); 
}

Collector::spectrum_t Collector::sigmas() const
{
    const double norm = sqrt((2.0*m_nsamples)/(3.14159265*m_nticks));
    return normit(m_sum, norm/m_nwaves); 
    


    // const double norm = (double)m_nsamples / (2.0*m_nticks*m_nwaves);
    // spectrum_t ret(m_nsamples, 0);
    // for (size_t ind=0; ind<m_nsamples; ++ind) {
    //     ret[ind] = sqrt(norm * m_sum2[ind]);
    // }
    // return ret;
}



Collector::spectrum_t Collector::linear() const
{
    return normit(m_sum, 1.0/m_nwaves);
}

Collector::spectrum_t Collector::square() const
{
    return normit(m_sum2, 1.0/m_nwaves);
}

Collector::spectrum_t Collector::rms() const
{
    auto ret = square();
    for (size_t ind=0; ind<m_nsamples; ++ind) {
        ret[ind] = sqrt(ret[ind]);
    }
    return ret;
}

Collector::spectrum_t Collector::periodogram() const
{
    return normit(m_sum2, 1.0/(double)(m_nwaves*m_nticks));
}

Collector::sequence_t Collector::bac() const
{
    return normit(m_bac, 1.0/m_nwaves);
}

Collector::sequence_t Collector::sac() const
{
    return normit(m_sac, 1.0/m_nwaves);
}

Collector::spectrum_t Collector::psd() const
{
    return normit(m_psd, 1.0/m_nwaves);
}



Generator::~Generator()
{
}

//
// GeneratorN
//

GeneratorN::GeneratorN(IDFT::pointer dft, normal_f normal)
    :dft(dft), normal(normal)
{
}
GeneratorN::~GeneratorN()
{
}

complex_vector_t
GeneratorN::spec(const real_vector_t& sigmas)
{
    const size_t nsamples = sigmas.size();
    // nsamples: even->1, odd->0
    const size_t nextra = (nsamples+1)%2;
    const size_t nhalf = nsamples / 2;
            
    complex_vector_t spec(nsamples, 0);

    auto normals = normal(2*(nhalf+nextra)+1);

    // The zero-frequency bin must be real and may be negative.
    for (size_t ind=0; ind <= nhalf+nextra; ++ind) {
        float mode = sigmas.at(ind);
        spec.at(ind) = std::complex(mode*normals.at(ind),
                                    mode*normals.at(ind+nhalf));
    }
    // DC bin must be real
    spec[0] = std::abs(spec[0]);
    if (nextra) {               // and same for Nyquist if we have one
        spec[nhalf] = std::abs(spec[nhalf]);
    }        

    hermitian_mirror(spec.begin(), spec.end());
    return spec;
}

real_vector_t
GeneratorN::wave(const real_vector_t& sigmas)
{
    return inv_c2r(dft, spec(sigmas));
}


//
// GeneratorU
//

GeneratorU::GeneratorU(IDFT::pointer dft, uniform_f uniform)
    :dft(dft), uniform(uniform)
{
}
GeneratorU::~GeneratorU()
{
}

complex_vector_t
GeneratorU::spec(const real_vector_t& sigmas)
{
    const size_t nsamples = sigmas.size();
    // nsamples: even->1, odd->0
    const size_t nextra = (nsamples+1)%2;
    const size_t nhalf = nsamples / 2;
            
    complex_vector_t spec(nsamples, 0);

    auto uniforms = uniform(nsamples);

    // The zero-frequency bin must be real and may be negative.
    spec.at(0).real(sigmas.at(0)*sqrt(-2*log(uniforms.at(0))));

    for (size_t ind=1; ind < nhalf; ++ind) {
        float mode = sigmas.at(ind);
        float rad = mode*sqrt(-2*log(uniforms.at(ind)));
        float ang = 2 * 3.141592653589793 * uniforms.at(ind+nhalf);
        spec.at(ind) = std::polar(rad, ang);
    }
    if (nextra) {       // have Nyquist bin
        // Must be real, can be negative.
        spec.at(nhalf).real(sigmas.at(nhalf)*sqrt(-2*log(uniforms.at(nhalf))));
    }
    hermitian_mirror(spec.begin(), spec.end());
    return spec;
}

real_vector_t
GeneratorU::wave(const real_vector_t& sigmas)
{
    return inv_c2r(dft, spec(sigmas));
}
