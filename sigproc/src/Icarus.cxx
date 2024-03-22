// Noise filter (NF) module for ICARUS
// Features:                       
//   RC tail correction         
//
// 11/7/2023, created by W.Gu (wgu@bnl.gov) 


#include "WireCellSigProc/Icarus.h"
#include "WireCellSigProc/Derivations.h"

#include "WireCellAux/DftTools.h"

#include "WireCellUtil/NamedFactory.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <set>

WIRECELL_FACTORY(icarusOneChannelNoise, WireCell::SigProc::Icarus::OneChannelNoise, WireCell::IChannelFilter,
                 WireCell::IConfigurable)

using namespace WireCell::SigProc;
using WireCell::Aux::DftTools::fwd_r2c;
using WireCell::Aux::DftTools::inv_c2r;

Icarus::OneChannelNoise::OneChannelNoise(const std::string& anode, const std::string& noisedb, const std::string& rcresp)
  : m_anode_tn(anode)
  , m_noisedb_tn(noisedb)
  , m_rcresp_tn(rcresp)
  , m_check_partial()
{
}
Icarus::OneChannelNoise::~OneChannelNoise() {}

void Icarus::OneChannelNoise::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    m_noisedb_tn = get(cfg, "noisedb", m_noisedb_tn);
    m_noisedb = Factory::find_tn<IChannelNoiseDatabase>(m_noisedb_tn);

    std::string dft_tn = get<std::string>(cfg, "dft", "FftwDFT");
    m_dft = Factory::find_tn<IDFT>(dft_tn);

    m_rcresp_tn = get(cfg, "rcresp", m_rcresp_tn);
    m_rcresp = Factory::find_tn<IWaveform>(m_rcresp_tn);
}
WireCell::Configuration Icarus::OneChannelNoise::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = m_anode_tn;
    cfg["noisedb"] = m_noisedb_tn;
    cfg["rcresp"] = m_rcresp_tn;
    cfg["dft"] = "FftwDFT";     // type-name for the DFT to use
    return cfg;
}

WireCell::Waveform::ChannelMaskMap Icarus::OneChannelNoise::apply(int ch, signal_t& signal) const
{
    WireCell::Waveform::ChannelMaskMap ret;

    // do we need a nominal baseline correction?
    // float baseline = m_noisedb->nominal_baseline(ch);

    // correct rc undershoot
    auto spectrum = fwd_r2c(m_dft, signal);
    bool is_partial = m_check_partial(spectrum);  // Xin's "IS_RC()"

    if (!is_partial) {
        // auto nticks = signal.size();
        // auto t0 = m_rcresp->waveform_start();
        // auto period = m_rcresp->waveform_period();
        // Binning bins(nticks , t0, t0 + nticks * period);
        // auto wave = m_rcresp->waveform_samples(bins);
        auto wave = m_rcresp->waveform_samples();

        auto spec = fwd_r2c(m_dft, wave);
        WireCell::Waveform::shrink(spectrum, spec);
    }


    // remove the DC component
    spectrum.front() = 0;
    signal = inv_c2r(m_dft, spectrum);

    // Now calculate the baseline ...
    std::pair<double, double> temp = WireCell::Waveform::mean_rms(signal);
    auto temp_signal = signal;
    for (size_t i = 0; i != temp_signal.size(); i++) {
        if (fabs(temp_signal.at(i) - temp.first) > 6 * temp.second) {
            temp_signal.at(i) = temp.first;
        }
    }
    float baseline = WireCell::Waveform::median_binned(temp_signal);
    // correct baseline
    WireCell::Waveform::increase(signal, baseline * (-1));

    return ret;
}

WireCell::Waveform::ChannelMaskMap Icarus::OneChannelNoise::apply(channel_signals_t& chansig) const
{
    return WireCell::Waveform::ChannelMaskMap();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
