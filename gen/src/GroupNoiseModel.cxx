#include "WireCellGen/GroupNoiseModel.h"


#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Interpolate.h"

WIRECELL_FACTORY(GroupNoiseModel,
                 WireCell::Gen::GroupNoiseModel,
                 WireCell::IChannelSpectrum,
                 WireCell::IConfigurable)

using namespace WireCell;

Gen::GroupNoiseModel::~GroupNoiseModel() { } 

WireCell::Configuration Gen::GroupNoiseModel::default_configuration() const
{
    Configuration cfg;

    /// Configuration: spectra_file.  Name of file containing
    /// array of per-group data with "groupID", array "freqs" of
    /// frequency and array "amps" of amplitude.
    cfg["spectral_file"] = "";
    /// Configuration: map_file.  Name of file containing array of
    /// per group data with "groupID" and array "channels" holding
    /// IDs of channels in the group.
    cfg["map_file"] = "";
    /// Configuration: spec_scale.  An arbitrary factor multiplied to
    /// each spectrum read from file.
    cfg["spec_scale"] = 1.0;
    /// Configuration: nsamples.  The number of frequency samples
    /// in returned (no loaded) spectra.
    cfg["nsamples"] = 1024;        
    /// Configuration: tick.  The sampling time period
    /// corresponding to 1/Fmax of the produced spectra
    cfg["tick"] = 0.5*units::us;

    return cfg;
}

void Gen::GroupNoiseModel::configure(const WireCell::Configuration& cfg)
{
    double spec_scale = get(cfg, "spec_scale", 1.0);
    size_t nsamples = get(cfg, "nsamples", 1024);
    double tick = get(cfg, "tick", 0.5*units::us);

    std::string map_file = cfg["map_file"].asString();
    if (map_file.empty()) {
        THROW(ValueError() << errmsg{"no map file given to GroupNoiseModel"});
    }
    std::string spectral_file = cfg["spectral_file"].asString();
    if (spectral_file.empty()) {
        THROW(ValueError() << errmsg{"no spectral file given to GroupNoiseModel"});
    }

    m_ch2grp.clear();
    auto mapdata = Persist::load(map_file);
    for (unsigned int i = 0; i < mapdata.size(); ++i) {
        auto jdata = mapdata[i];
        const int groupID = jdata["groupID"].asInt();
        for (auto jch : jdata["channels"]) {
            m_ch2grp[jch.asInt()] = groupID;
        }
    }
    
    const double Fmax = 1/tick;
    const double dF = Fmax/nsamples;

    m_grp2amp.clear();
    auto specdata = Persist::load(spectral_file);
    for (unsigned int i = 0; i < specdata.size(); ++i) {
        auto jdata = specdata[i];
        const int groupID = jdata["groupID"].asInt();
        auto spec_freq = jdata["freqs"];
        auto spec_amps = jdata["amps"];
        const int npts = spec_freq.size();
        std::unordered_map<float, float> pts;
        for (int ind=0; ind<npts; ++ind) {
            const float freq = spec_freq[ind].asFloat();
            pts[freq] = spec_scale * spec_amps[ind].asFloat();
        }
        irrterp<float> terp(pts.begin(), pts.end());
        std::vector<float> spec(nsamples, 0);
        terp(spec.begin(), nsamples, 0, dF);
        m_grp2amp[groupID] = spec;
    }
    
}

const Gen::GroupNoiseModel::amplitude_t& Gen::GroupNoiseModel::operator()(int chid) const
{
    static amplitude_t dummy;

    // Lookup channels group
    auto git = m_ch2grp.find(chid);
    if (git == m_ch2grp.end()) {
        return dummy;
    }
    int groupID = git->second;

    // Lookup groups spectrum
    auto ait = m_grp2amp.find(groupID);
    if (ait == m_grp2amp.end()) {
        return dummy;
    }
    return ait->second;
}

