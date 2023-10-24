#include "WireCellGen/GroupNoiseModel.h"


#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/Spectrum.h"

#include "WireCellAux/DftTools.h"

WIRECELL_FACTORY(GroupNoiseModel,
                 WireCell::Gen::GroupNoiseModel,
                 WireCell::INamed,
                 WireCell::IChannelSpectrum,
                 WireCell::IGroupSpectrum,
                 WireCell::IConfigurable)

using namespace WireCell;

Gen::GroupNoiseModel::GroupNoiseModel()
    : Aux::Logger("GroupNoiseModel", "gen") { } 
Gen::GroupNoiseModel::~GroupNoiseModel() { } 

WireCell::Configuration Gen::GroupNoiseModel::default_configuration() const
{
    Configuration cfg;

    /// Configuration: spectra.  See aux/docs/noise.org.
    cfg["spectra"] = Json::nullValue;

    /// Configuration: groups.  See aux/docs/noise.org.
    cfg["groups"] = Json::nullValue;

    /// Configuration: spec_scale.  An arbitrary factor multiplied to
    /// each spectral input amplitude.
    cfg["scale"] = 1.0;

    /// Configuration: nsamples.  This gives the size of the spectra
    /// returned to the user.  It is distinct from both the "nsamples"
    /// in the spectral_file and the size of the freqs/amps array.
    cfg["nsamples"] = 1024;

    /// Configuration: tick.  The sampling time period
    /// corresponding to 1/Fmax of the produced spectra
    cfg["tick"] = 0.5*units::us;

    return cfg;
}

void Gen::GroupNoiseModel::configure(const WireCell::Configuration& cfg)
{
    // desired for generated waveforms:
    const double spec_scale = get(cfg, "scale", 1.0);
    const size_t nsamples = get(cfg, "nsamples", 1024);
    const double tick = get(cfg, "tick", 0.5*units::us);

    size_t errors = 0;

    // for debug() output
    std::string map_file = "<config>";
    std::string spectral_file = "<config>";

    auto jgroups = cfg["groups"];
    if (jgroups.isNull()) {
        log->critical("no parameter \"group\" given");
        ++errors;
    }
    if (jgroups.isString()) {
        map_file = jgroups.asString();
        jgroups = Persist::load(map_file);
    }

    auto jspectra = cfg["spectra"];
    if (jspectra.isNull()) {
        log->critical("no parameter \"spectra\" given");
        ++errors;
    }
    if (jspectra.isString()) {
        spectral_file = jspectra.asString();
        jspectra = Persist::load(spectral_file);
    }

    // Intern channel groups data.
    std::set<int> groups;
    m_ch2grp.clear();
    for (const auto& jcg : jgroups) {
        auto jgroup = jcg["group"].isNull() ? jcg["groupID"] : jcg["group"];
        const int group = jgroup.asInt();
        groups.insert(group);
        for (const auto& jch : jcg["channels"]) {
            m_ch2grp[jch.asInt()] = group;
        }
    }
    
    // Read in, resample and intern the spectra.
    m_grp2amp.clear();
    int count=0; 
    for (const auto& jso : jspectra) {
        // Some noise files lack explicit group ID so simply count groups.
        int group = count;
        auto jgroup = jso["group"];
        if (jgroup.isNull()) {
            jgroup = jso["groupID"];
        }
        if (jgroup.isInt()) {
            group = jgroup.asInt();
        }
        ++count;

        // The original waveform size and sampling period
        const size_t norig = jso["nsamples"].asInt();
        if (!norig) {
            log->critical("group {} lacks required \"nsamples\"", group);
            ++errors;
            continue;
        }
        const double porig = jso["period"].asDouble();

        const auto jfreqs = jso["freqs"];
        const auto jamps = jso["amps"];
        const size_t npts = jfreqs.size();
        log->debug("loaded group:{} nsamples:{} period:{} nfreqs:{} namps:{}",
                   group, norig, porig, jfreqs.size(), jamps.size());

        const double Fnyquist = 1.0/(2*porig);
        const double Frayleigh = 1.0/(norig*porig);

        std::unordered_map<float, float> pts;
        for (int ind=0; ind<(int)npts; ++ind) {
            const float freq = jfreqs[ind].asFloat();
            if (freq > Fnyquist) {
                // only read in half-spectrum
                continue;
            }
            pts[freq] = spec_scale * jamps[ind].asFloat();
        }

        // Restore original, full spectrum
        irrterp<float> terp(pts.begin(), pts.end());
        std::vector<float> sorig(norig, 0);
        terp(sorig.begin(), norig, 0, Frayleigh);

        Spectrum::hermitian_mirror(sorig.begin(), sorig.end());

        std::vector<float> spec(nsamples, 0);
        Spectrum::resample(sorig.begin(), sorig.end(),
                           spec.begin(), spec.end(),
                           tick / porig);
        
        m_grp2amp[group] = spec;
    }
    
    if (errors) {
        THROW(ValueError() << errmsg{"configuration error"});
    }

    log->debug("loaded {} channels from {} in {} groups and {} spectra from {}",
               m_ch2grp.size(), map_file, groups.size(), m_grp2amp.size(), spectral_file);
}

const Gen::GroupNoiseModel::amplitude_t& Gen::GroupNoiseModel::channel_spectrum(int chid) const
{
    static amplitude_t dummy;

    // Lookup channels group
    auto git = m_ch2grp.find(chid);
    if (git == m_ch2grp.end()) {
        return dummy;
    }
    int group = git->second;

    // Lookup groups spectrum
    auto ait = m_grp2amp.find(group);
    if (ait == m_grp2amp.end()) {
        return dummy;
    }
    return ait->second;
}

int Gen::GroupNoiseModel::groupid(int chid) const
{
    // Lookup channels group
    auto git = m_ch2grp.find(chid);
    if (git == m_ch2grp.end()) {
        return -1;
    }
    return git->second;
}

const Gen::GroupNoiseModel::amplitude_t& Gen::GroupNoiseModel::group_spectrum(int group) const
{
    static amplitude_t dummy;
    if (group < 0) {
        return dummy;
    }

    // Lookup groups spectrum
    auto ait = m_grp2amp.find(group);
    if (ait == m_grp2amp.end()) {
        return dummy;
    }
    return ait->second;
}

