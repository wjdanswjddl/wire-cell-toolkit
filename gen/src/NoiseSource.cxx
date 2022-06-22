/*


  Warning, this is crufty old code.  It should be rewritten to use
  Noise.h bases and RandTools and Spectra in the manner that AddNoise
  clases do.


 */



#include "WireCellGen/NoiseSource.h"

#include "WireCellAux/DftTools.h"
#include "WireCellIface/IRandom.h"

#include "WireCellAux/SimpleTrace.h"
#include "WireCellAux/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"


#include <iostream>

WIRECELL_FACTORY(NoiseSource, WireCell::Gen::NoiseSource, WireCell::IFrameSource, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;
using WireCell::Aux::DftTools::inv_c2r;

using WireCell::Aux::SimpleTrace;
using WireCell::Aux::SimpleFrame;

using complex_vector_t = std::vector<std::complex<float>>;

static complex_vector_t
generate_spectrum(const std::vector<float>& spec, IRandom::pointer rng, double replace=0.02)
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


Gen::NoiseSource::NoiseSource(const std::string& model, const std::string& anode, const std::string& rng)
  : m_time(0.0 * units::ns)
  , m_stop(1.0 * units::ms)
  , m_readout(5.0 * units::ms)
  , m_tick(0.5 * units::us)
  , m_frame_count(0)
  , m_anode_tn(anode)
  , m_model_tn(model)
  , m_rng_tn(rng)
  , m_nsamples(9600)
  , m_rep_percent(0.02)  // replace 2% at a time
  , m_eos(false)
{
    // initialize the random number ...
    // auto& spec = (*m_model)(0);

    // end initialization ..
}

Gen::NoiseSource::~NoiseSource() {}

WireCell::Configuration Gen::NoiseSource::default_configuration() const
{
    Configuration cfg;
    cfg["start_time"] = m_time;
    cfg["stop_time"] = m_stop;
    cfg["readout_time"] = m_readout;
    cfg["sample_period"] = m_tick;
    cfg["first_frame_number"] = m_frame_count;

    cfg["anode"] = m_anode_tn;
    cfg["model"] = m_model_tn;
    cfg["rng"] = m_rng_tn;
    cfg["dft"] = "FftwDFT";     // type-name for the DFT to use
    cfg["nsamples"] = m_nsamples;
    cfg["replacement_percentage"] = m_rep_percent;
    return cfg;
}

void Gen::NoiseSource::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    if (!m_rng) {
        THROW(KeyError() << errmsg{"failed to get IRandom: " + m_rng_tn});
    }
    std::string dft_tn = get<std::string>(cfg, "dft", "FftwDFT");
    m_dft = Factory::find_tn<IDFT>(dft_tn);

    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    if (!m_anode) {
        THROW(KeyError() << errmsg{"failed to get IAnodePlane: " + m_anode_tn});
    }

    m_model_tn = get(cfg, "model", m_model_tn);
    m_model = Factory::find_tn<IChannelSpectrum>(m_model_tn);
    if (!m_model) {
        THROW(KeyError() << errmsg{"failed to get IChannelSpectrum: " + m_model_tn});
    }

    m_readout = get<double>(cfg, "readout_time", m_readout);
    m_time = get<double>(cfg, "start_time", m_time);
    m_stop = get<double>(cfg, "stop_time", m_stop);
    m_tick = get<double>(cfg, "sample_period", m_tick);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);
    m_nsamples = get<int>(cfg, "m_nsamples", m_nsamples);
    m_rep_percent = get<double>(cfg, "replacement_percentage", m_rep_percent);

    cerr << "Gen::NoiseSource: using IRandom: \"" << m_rng_tn << "\""
         << " IAnodePlane: \"" << m_anode_tn << "\""
         << " IChannelSpectrum: \"" << m_model_tn << "\""
         << " readout time: " << m_readout / units::us << "us\n";
}

bool Gen::NoiseSource::operator()(IFrame::pointer& frame)
{
    if (m_eos) {  // This source does not restart.
        return false;
    }

    if (m_time >= m_stop) {
        frame = nullptr;
        m_eos = true;
        return true;
    }
    ITrace::vector traces;
    const int tbin = 0;
    int nsamples = 0;
    for (auto chid : m_anode->channels()) {
        const auto& spec = m_model->channel_spectrum(chid);

        //Waveform::realseq_t noise = Gen::Noise::generate_waveform(spec, m_rng, m_rep_percent);
        auto cnoise = generate_spectrum(spec, m_rng, m_rep_percent);
        auto noise = inv_c2r(m_dft, cnoise);

        //	std::cout << noise.size() << " " << nsamples << std::endl;
        noise.resize(m_nsamples, 0);
        auto trace = make_shared<SimpleTrace>(chid, tbin, noise);
        traces.push_back(trace);
        nsamples += noise.size();
    }
    cerr << "Gen::NoiseSource: made " << traces.size() << " traces, " << nsamples << " samples\n";
    frame = make_shared<SimpleFrame>(m_frame_count, m_time, traces, m_tick);
    m_time += m_readout;
    ++m_frame_count;
    return true;
}
