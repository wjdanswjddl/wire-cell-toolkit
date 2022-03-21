#include "WireCellAux/WaveformMap.h"

#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/Units.h"

// #include <boost/math/interpolators/cubic_b_spline.hpp>

WIRECELL_FACTORY(WaveformMap,
                 WireCell::Aux::WaveformMap,
                 WireCell::IWaveformMap, WireCell::IConfigurable)

using namespace WireCell;

struct InterpolatedWaveform : public IWaveform {

    double start{0}, period{0};
    IWaveform::sequence_type sequence{};


    using interp_f = std::function<float(double)>;
    interp_f interp;

    InterpolatedWaveform(double start, double period,
                         IWaveform::sequence_type seq)
        : start(start), period(period), sequence(std::move(seq))
    {
        interp = WireCell::linterp<float>(sequence.begin(), sequence.end(), start, period);
    }

    virtual ~InterpolatedWaveform() { }


    virtual double waveform_start() const { return start; }
    // The sampling period aka bin width
    virtual double waveform_period() const { return period; }
    // The collection of samples
    virtual const sequence_type& waveform_samples() const { return sequence; }
    // The collection of samples rebinned
    virtual sequence_type waveform_samples(const WireCell::Binning& tbins) const {
        const size_t nbins = tbins.nbins();
        sequence_type ret(nbins);
        for (size_t ind=0; ind<nbins; ++ind) {
            ret[ind] = interp(tbins.edge(ind));
        }
        return ret;
    }
};

Aux::WaveformMap::WaveformMap()
{
}
Aux::WaveformMap::~WaveformMap()
{
}

WireCell::Configuration Aux::WaveformMap::default_configuration() const
{
    Configuration cfg;
    cfg["filename"] = "";            // can be one or an array of filenames
    return cfg;
}

void Aux::WaveformMap::configure(const WireCell::Configuration& cfg)
{
    auto fnames = cfg["filename"];
    if (fnames.isString()) {
        Configuration tmp = Json::arrayValue;
        tmp.append(fnames);
        fnames = tmp;
    }
    m_wfs.clear();

    for (auto jfname : fnames) {
        std::string fname = jfname.asString();
        auto jdat = Persist::load(fname);
        for (auto jobj : jdat) {
            double start = get(jobj, "start", 0.0); // optional
            double period = get(jobj, "period", 0.5*units::ms); // required
            std::string name = jobj["name"].asString();
            if (name.empty()) {
                THROW(ValueError() << errmsg{"malformed waveform map file: " + fname});
            }
            auto jseq = jobj["samples"];
            IWaveform::sequence_type seq;
            for (auto one : jseq) {
                seq.push_back(one.asFloat());
            }
            m_wfs[name] = std::make_shared<InterpolatedWaveform>(
                start, period, seq);
        }
    }
}

std::vector<std::string> Aux::WaveformMap::waveform_names() const
{
    std::vector<std::string> ret;
    for (auto it : m_wfs) {
        ret.push_back(it.first);
    }
    return ret;
}


IWaveform::pointer Aux::WaveformMap::waveform_lookup(const std::string& name) const
{
    auto it{m_wfs.find(name)};
    if (it == m_wfs.end()) {
        return nullptr;
    }
    return it->second;
}
