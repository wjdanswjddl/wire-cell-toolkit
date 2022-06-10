#include "WireCellAux/Spectra.h"
#include "WireCellUtil/Waveform.h"

#include <cmath>

using namespace WireCell;
using namespace WireCell::Waveform;
using namespace WireCell::Aux::Spectra;

NoiseCollector::NoiseCollector()
{
}
NoiseCollector::~NoiseCollector()
{
}
NoiseCollector::NoiseCollector(IDFT::pointer dft, size_t nsamples, bool acs)
{
    init(dft, nsamples, acs);
}

void NoiseCollector::init(IDFT::pointer dft, size_t nsamples, bool acs)
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
void NoiseCollector::add_safe(const std::vector<float>& fwave)
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

NoiseCollector::spectrum_t NoiseCollector::amplitude() const
{
    return normit(m_sum, (double)m_nsamples/(double)(m_nticks*m_nwaves)); 
}

NoiseCollector::spectrum_t NoiseCollector::linear() const
{
    return normit(m_sum, 1.0/m_nwaves);
}

NoiseCollector::spectrum_t NoiseCollector::square() const
{
    return normit(m_sum2, 1.0/m_nwaves);
}

NoiseCollector::spectrum_t NoiseCollector::rms() const
{
    auto ret = square();
    for (size_t ind=0; ind<m_nsamples; ++ind) {
        ret[ind] = sqrt(ret[ind]);
    }
    return ret;
}

NoiseCollector::spectrum_t NoiseCollector::periodogram() const
{
    return normit(m_sum2, 1.0/(double)(m_nwaves*m_nticks));
}

NoiseCollector::sequence_t NoiseCollector::bac() const
{
    return normit(m_bac, 1.0/m_nwaves);
}

NoiseCollector::sequence_t NoiseCollector::sac() const
{
    return normit(m_sac, 1.0/m_nwaves);
}

NoiseCollector::spectrum_t NoiseCollector::psd() const
{
    return normit(m_psd, 1.0/m_nwaves);
}



NoiseGenerator::~NoiseGenerator()
{
}

//
// NoiseGeneratorN
//

NoiseGeneratorN::NoiseGeneratorN(IDFT::pointer dft, normal_f normal)
    :dft(dft), normal(normal)
{
}
NoiseGeneratorN::~NoiseGeneratorN()
{
}

complex_vector_t
NoiseGeneratorN::spec(const real_vector_t& meanspec)
{
    const size_t nsamples = meanspec.size();
    // nsamples: even->1, odd->0
    const size_t nextra = (nsamples+1)%2;
    const size_t nhalf = nsamples / 2;
            
    complex_vector_t spec(nsamples, 0);

    auto normals = normal(nsamples);

    // The Gaussian sigma must be the Rayleigh mode which is smaller
    // than Rayleigh mean (ie, what NoiseCollector::amplitude() returns) by
    // this factor:
    const float mean_to_mode = sqrt(2/3.141592);

    // The zero-frequency bin must be real and may be negative.
    spec.at(0).real(mean_to_mode*meanspec.at(0)*normals.at(0));
    for (size_t ind=1; ind < nhalf; ++ind) {
        float mode = mean_to_mode*meanspec.at(ind);
        spec.at(ind) = std::complex(mode*normals.at(ind),
                                 mode*normals.at(ind+nhalf));
    }
    if (nextra) {       // have Nyquist bin
        // Must be real, can be negative.
        spec.at(nhalf+1).real(mean_to_mode*meanspec.at(nhalf)*normals.at(nhalf));
    }
    hermitian_symmetry_inplace(spec);
    return spec;
}

real_vector_t
NoiseGeneratorN::wave(const real_vector_t& meanspec)
{
    // Internally, this normalizes by 1/N
    return inv_c2r(dft, spec(meanspec));
}


//
// NoiseGeneratorU
//

NoiseGeneratorU::NoiseGeneratorU(IDFT::pointer dft, uniform_f uniform)
    :dft(dft), uniform(uniform)
{
}
NoiseGeneratorU::~NoiseGeneratorU()
{
}

complex_vector_t
NoiseGeneratorU::spec(const real_vector_t& meanspec)
{
    const size_t nsamples = meanspec.size();
    // nsamples: even->1, odd->0
    const size_t nextra = (nsamples+1)%2;
    const size_t nhalf = nsamples / 2;
            
    complex_vector_t spec(nsamples, 0);

    auto uniforms = uniform(nsamples);

    constexpr double π {3.141592653589793};

    // The Gaussian sigma must be the Rayleigh mode which is smaller
    // than Rayleigh mean (ie, what NoiseCollector::amplitude() returns) by
    // this factor:
    const float mean_to_mode = sqrt(2/π);

    // The zero-frequency bin must be real and may be negative.
    spec.at(0).real(mean_to_mode*meanspec.at(0)*sqrt(-2*log(uniforms.at(0))));

    for (size_t ind=1; ind < nhalf; ++ind) {
        float mode = mean_to_mode*meanspec.at(ind);
        float rad = mode*sqrt(-2*log(uniforms.at(ind)));
        float ang = 2 * π * uniforms.at(ind+nhalf);
        spec.at(ind) = std::polar(rad, ang);
    }
    if (nextra) {       // have Nyquist bin
        // Must be real, can be negative.
        spec.at(nhalf+1).real(mean_to_mode*meanspec.at(nhalf+1)*sqrt(-2*log(uniforms.at(nhalf))));
    }
    hermitian_symmetry_inplace(spec);
    return spec;
}

real_vector_t
NoiseGeneratorU::wave(const real_vector_t& meanspec)
{
    // Internally, this normalizes by 1/N
    return inv_c2r(dft, spec(meanspec));
}
