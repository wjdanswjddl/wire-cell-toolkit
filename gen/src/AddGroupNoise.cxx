#include "WireCellGen/AddGroupNoise.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/FFTBestLength.h"

#include "Noise.h"

#include <iostream>

WIRECELL_FACTORY(AddGroupNoise, WireCell::Gen::AddGroupNoise, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::AddGroupNoise::AddGroupNoise(const std::string& spectra_file, const std::string& map_file, const std::string& rng)
  : m_spectra_file(spectra_file),
    m_map_file(map_file)
  , m_rng_tn(rng)
  , m_nsamples(4096)
  , m_fluctuation(0.1)  // Fluctuation randomization
  , m_shift(0.9)
  , m_period(0.4)
  , log(Log::logger("sim"))
{
}

Gen::AddGroupNoise::~AddGroupNoise() {}

void Gen::AddGroupNoise::gen_elec_resp_default()
{
    // calculate the frequencies ...
    m_elec_resp_freq.resize(m_fft_length, 0);
    for (unsigned int i = 0; i != m_elec_resp_freq.size(); i++) {
        if (i <= m_elec_resp_freq.size() / 2.) {
            m_elec_resp_freq.at(i) =
                i / (m_elec_resp_freq.size() * 1.0) * 1. / m_period;  // the second half is useless ...
        }
        else {
            m_elec_resp_freq.at(i) = (m_elec_resp_freq.size() - i) / (m_elec_resp_freq.size() * 1.0) * 1. /
                                     m_period;  // the second half is useless ...
        }
    }
}

WireCell::Configuration Gen::AddGroupNoise::default_configuration() const
{
    Configuration cfg;

    cfg["spectra_file"] = m_spectra_file;
    cfg["map_file"] = m_map_file;
    cfg["rng"] = m_rng_tn;
    cfg["nsamples"] = m_nsamples;
    cfg["random_fluctuation_amplitude"] = m_fluctuation;
    cfg["random_shift_amplitude"] = m_shift;
    cfg["period"] = m_period;
    cfg["normalization"] = m_normalization;

    return cfg;
}

void Gen::AddGroupNoise::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    m_spectra_file = get(cfg, "spectra_file", m_spectra_file);
    m_map_file = get(cfg, "map_file", m_map_file);
    m_nsamples = get<int>(cfg, "nsamples", m_nsamples);
    m_period = get<int>(cfg, "period", m_period);
    m_fluctuation = get<double>(cfg, "random_fluctuation_amplitude", m_fluctuation);
    m_shift = get<double>(cfg,"random_shift_amplitude",m_shift);
    m_normalization = get<int>(cfg, "normalization", m_normalization);

    m_fft_length = fft_best_length(m_nsamples);
    gen_elec_resp_default();
    
    m_ch2grp.clear();
    auto mapdata = Persist::load(m_map_file);
    for(unsigned int i=0; i<mapdata.size();++i){
	auto jdata = mapdata[i];
	int groupID = jdata["groupID"].asInt();
	auto chnls_json = jdata["channels"];
	for(unsigned int j=0;j<chnls_json.size();++j){
		m_ch2grp.insert({chnls_json[j].asInt(),groupID});
	} 
    }
    
    

    m_grp2spec.clear();
    m_grp2rnd_amp.clear();
    m_grp2rnd_phs.clear();
    auto specdata = Persist::load(m_spectra_file);
    
    for(unsigned int i=0; i<mapdata.size();++i){
	auto jdata = specdata[i];
	int groupID = jdata["groupID"].asInt();
	auto spec_freq = jdata["freqs"];
	auto spec_amps =  jdata["amps"];
	std::vector<float> spec(spec_freq.size());
        std::vector<float> rnd_phs(spec_freq.size());
        std::vector<float> rnd_amp(spec_freq.size());
	for(unsigned int j=0;j<spec.size();++j){
		spec[j]=spec_amps[j].asFloat();
		rnd_amp[j] = m_shift + 2 * m_fluctuation * m_rng->uniform(0, 1);
                rnd_phs[j] = m_rng->uniform(0, 2 * M_PI);
	} 
	m_grp2spec.insert({groupID,spec});   
    	m_grp2rnd_amp.insert({groupID,rnd_amp});
        m_grp2rnd_phs.insert({groupID,rnd_phs}); 
   }
}

bool Gen::AddGroupNoise::operator()(const input_pointer& inframe, output_pointer& outframe)
{
    if (!inframe) {
        outframe = nullptr;
        return true;
    }

    ITrace::vector outtraces;

    for (const auto& intrace : *inframe->traces()) {
        int chid = intrace->channel();
	int groupID = m_ch2grp[chid];
	auto spec = m_grp2spec[groupID];
        auto rnd_amp = m_grp2rnd_amp[groupID];
        auto rnd_phs = m_grp2rnd_phs[groupID];
        
	WireCell::Waveform::compseq_t noise_freq(spec.size(), 0);

        for (std::size_t i = 0; i < spec.size(); i++) {
            const float amplitude = spec[i];

            float theta = rnd_phs[i];
            float rad = amplitude * rnd_amp[i];

            complex<float> tc(rad * cos(theta), rad * sin(theta));
            noise_freq[i] = tc;
        }

        Waveform::realseq_t wave = WireCell::Waveform::idft(noise_freq);

        Waveform::increase(wave, intrace->charge());
        auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), wave);
        outtraces.push_back(trace);

    }  // end channels

    outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(), outtraces, inframe->tick());
    return true;
}
