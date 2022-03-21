#include "WireCellGen/AddGroupNoise.h"

#include "WireCellAux/DftTools.h"

#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Persist.h"

#include "Noise.h"

#include <iostream>

WIRECELL_FACTORY(AddGroupNoise, WireCell::Gen::AddGroupNoise,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::AddGroupNoise::AddGroupNoise(const std::string &spectra_file,
                                  const std::string &map_file,
                                  const std::string &rng)
    : Aux::Logger("AddNoise", "gen"), m_spectra_file(spectra_file),
      m_map_file(map_file), m_rng_tn(rng), m_nsamples(4096),
      log(Log::logger("sim")) {}

Gen::AddGroupNoise::~AddGroupNoise() {}

WireCell::Configuration Gen::AddGroupNoise::default_configuration() const {
  Configuration cfg;

  cfg["spectra_file"] = m_spectra_file;
  cfg["map_file"] = m_map_file;
  cfg["rng"] = m_rng_tn;
  cfg["replacement_percentage"] = m_rep_percent;
  cfg["nsamples"] = m_nsamples;
  cfg["dft"] = "FftwDFT";

  return cfg;
}

void Gen::AddGroupNoise::configure(const WireCell::Configuration &cfg) {
  m_rng_tn = get(cfg, "rng", m_rng_tn);
  m_rng = Factory::find_tn<IRandom>(m_rng_tn);
  m_spectra_file = get(cfg, "spectra_file", m_spectra_file);
  m_map_file = get(cfg, "map_file", m_map_file);
  m_nsamples = get<int>(cfg, "nsamples", m_nsamples);
  m_rep_percent = get<double>(cfg, "replacement_percentage", m_rep_percent);
  std::string dft_tn = get<std::string>(cfg, "dft", "FftwDFT");
  m_dft = Factory::find_tn<IDFT>(dft_tn);

  m_ch2grp.clear();
  auto mapdata = Persist::load(m_map_file);
  for (unsigned int i = 0; i < mapdata.size(); ++i) {
    auto jdata = mapdata[i];
    int groupID = jdata["groupID"].asInt();
    auto chnls_json = jdata["channels"];
    for (unsigned int j = 0; j < chnls_json.size(); ++j) {
      m_ch2grp.insert({chnls_json[j].asInt(), groupID});
    }
  }

  m_grp2spec.clear();
  auto specdata = Persist::load(m_spectra_file);
  for (unsigned int i = 0; i < mapdata.size(); ++i) {
    auto jdata = specdata[i];
    int groupID = jdata["groupID"].asInt();
    auto spec_freq = jdata["freqs"];
    auto spec_amps = jdata["amps"];
    std::vector<float> spec(spec_freq.size());
    for (unsigned int j = 0; j < spec.size(); ++j) {
      spec[j] = spec_amps[j].asFloat();
    }
    m_grp2spec.insert({groupID, spec});
  }
}

bool Gen::AddGroupNoise::operator()(const input_pointer &inframe,
                                    output_pointer &outframe) {
  if (!inframe) {
    outframe = nullptr;
    return true;
  }
  m_grp2noise.clear();
  for (const auto &[key, value] : m_grp2spec) {
    int groupID = key;
    auto cspec = Gen::Noise::generate_spectrum(value, m_rng, m_rep_percent);
    m_grp2noise.insert({groupID, cspec});
  }

  ITrace::vector outtraces;

  for (const auto &intrace : *inframe->traces()) {

    int chid = intrace->channel();
    int groupID = m_ch2grp[chid];

    WireCell::Waveform::compseq_t noise_freq = m_grp2noise[groupID];
    auto wave = Waveform::real(Aux::inv(m_dft, noise_freq));
    wave.resize(m_nsamples, 0);

    Waveform::increase(wave, intrace->charge());

    auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), wave);

    outtraces.push_back(trace);

  } // end channels

  outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(),
                                      outtraces, inframe->tick());
  return true;
}
