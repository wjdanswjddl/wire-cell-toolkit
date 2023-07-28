/***************************************************************************/
/*             Noise filter (NF) module for DUNE CRP                       */
/***************************************************************************/
// Features:                       
//   undershoot correction         
//   partial undershoot correction 
//
// 07/19/2023, created by W.Gu (wgu@bnl.gov) 


#include "WireCellSigProc/Microboone.h"
#include "WireCellSigProc/DuneCrp.h"
#include "WireCellSigProc/Derivations.h"

#include "WireCellAux/DftTools.h"

#include "WireCellUtil/NamedFactory.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <set>

WIRECELL_FACTORY(duneCrpOneChannelNoise, WireCell::SigProc::DuneCrp::OneChannelNoise, WireCell::IChannelFilter,
                 WireCell::IConfigurable)

using namespace WireCell::SigProc;
using WireCell::Aux::DftTools::fwd_r2c;
using WireCell::Aux::DftTools::inv_c2r;


/*
 * Classes
 */

DuneCrp::OneChannelNoise::OneChannelNoise(const std::string& anode, const std::string& noisedb)
  : m_anode_tn(anode)
  , m_noisedb_tn(noisedb)
  , m_check_partial()  // fixme, here too.
{
}
DuneCrp::OneChannelNoise::~OneChannelNoise() {}

void DuneCrp::OneChannelNoise::configure(const WireCell::Configuration& cfg)
{
    m_anode_tn = get(cfg, "anode", m_anode_tn);
    m_anode = Factory::find_tn<IAnodePlane>(m_anode_tn);
    m_noisedb_tn = get(cfg, "noisedb", m_noisedb_tn);
    m_noisedb = Factory::find_tn<IChannelNoiseDatabase>(m_noisedb_tn);

    std::string dft_tn = get<std::string>(cfg, "dft", "FftwDFT");
    m_dft = Factory::find_tn<IDFT>(dft_tn);
}
WireCell::Configuration DuneCrp::OneChannelNoise::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = m_anode_tn;
    cfg["noisedb"] = m_noisedb_tn;
    cfg["dft"] = "FftwDFT";     // type-name for the DFT to use
    return cfg;
}

WireCell::Waveform::ChannelMaskMap DuneCrp::OneChannelNoise::apply(int ch, signal_t& signal) const
{
    WireCell::Waveform::ChannelMaskMap ret;

    // do we need a nominal baseline correction?
    // float baseline = m_noisedb->nominal_baseline(ch);

    // correct rc undershoot
    auto spectrum = fwd_r2c(m_dft, signal);
    bool is_partial = m_check_partial(spectrum);  // Xin's "IS_RC()"

    if (!is_partial) {
        auto const& spec = m_noisedb->rcrc(ch);  // rc_layers set to 1 in channel noise db
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

    // Now do the adaptive baseline for the bad RC channels
    if (is_partial) {
        auto wpid = m_anode->resolve(ch);
        const int iplane = wpid.index();
        // add something
        WireCell::Waveform::BinRange temp_bin_range;
        temp_bin_range.first = 0;
        temp_bin_range.second = signal.size();

        if (iplane != 2) {  // not collection
            ret["lf_noisy"][ch].push_back(temp_bin_range);
            // std::cout << "Partial " << ch << std::endl;
        }
        Microboone::SignalFilter(signal);
        Microboone::RawAdapativeBaselineAlg(signal);
        Microboone::RemoveFilterFlags(signal);
    }

    const float min_rms = m_noisedb->min_rms_cut(ch);
    const float max_rms = m_noisedb->max_rms_cut(ch);
    // alternative RMS tagging
    Microboone::SignalFilter(signal);
    bool is_noisy = Microboone::NoisyFilterAlg(signal, min_rms, max_rms);
    Microboone::RemoveFilterFlags(signal);
    if (is_noisy) {
        WireCell::Waveform::BinRange temp_bin_range;
        temp_bin_range.first = 0;
        temp_bin_range.second = signal.size();
        ret["noisy"][ch].push_back(temp_bin_range);
    }

    return ret;
}

WireCell::Waveform::ChannelMaskMap DuneCrp::OneChannelNoise::apply(channel_signals_t& chansig) const
{
    return WireCell::Waveform::ChannelMaskMap();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
