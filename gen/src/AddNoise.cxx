#include "WireCellGen/AddNoise.h"

#include "WireCellAux/DftTools.h"
#include "WireCellAux/RandTools.h"
#include "WireCellAux/Spectra.h"

#include "WireCellAux/SimpleTrace.h"
#include "WireCellAux/SimpleFrame.h"

#include "WireCellUtil/NamedFactory.h"

#include <unordered_set>

WIRECELL_FACTORY(AddNoise, WireCell::Gen::AddNoise,
                 WireCell::INamed,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

using WireCell::Aux::SimpleTrace;
using WireCell::Aux::SimpleFrame;

using namespace WireCell::Aux::RandTools;
using namespace WireCell::Aux::Spectra;


Gen::AddNoise::AddNoise() : Aux::Logger("AddNoise", "gen")
{
}

Gen::AddNoise::~AddNoise() 
{
}

WireCell::Configuration Gen::AddNoise::default_configuration() const
{
    Configuration cfg;
    // The name of the noise model.  Or, if an array is given, the
    // names of multiple noise models.  The plural spelling "models"
    // is also checked for an array of model names.
    cfg["model"] = "";
    // The IRandom
    cfg["rng"] = "Random";
    // The IDFT
    cfg["dft"] = "FftwDFT";
    // The size (number of ticks) of the noise waveforms
    cfg["nsamples"] = m_nsamples;
    // Percentage of fresh randomness added in using the recyled
    // random normal-Gaussian distribution.  Note, to match prior
    // interpreation of "percent" we apply twice this amount of
    // refreshing.  0.02 remains a reasonable choice.
    cfg["replacement_percentage"] = m_rep_percent;
    return cfg;
}

void Gen::AddNoise::configure(const WireCell::Configuration& cfg)
{
    std::string rng_tn = get<std::string>(cfg, "rng", "Random");
    m_rng = Factory::find_tn<IRandom>(rng_tn);

    std::string dft_tn = get<std::string>(cfg, "dft", "FftwDFT");
    m_dft = Factory::find_tn<IDFT>(dft_tn);

    // Specify a single model name or an array of model names.  We
    // also accept an array given as a pluralized "models".
    std::unordered_set<std::string> model_names;
    auto jmodel = cfg["model"];
    if (jmodel.isString()) {
        model_names.insert(jmodel.asString());
    }
    else {
        for (const auto& jmod : jmodel) {
            model_names.insert(jmod.asString());
        }
    }
    if (cfg["models"].isArray()) {
        for (const auto& jmod : cfg["models"]) {
            model_names.insert(jmod.asString());
        }
    }
    for (const auto& mtn : model_names) {
        m_models[mtn] = Factory::find_tn<IChannelSpectrum>(mtn);
        log->debug("using IChannelSpectrum: \"{}\"", mtn);
    }

    m_nsamples = get<int>(cfg, "nsamples", m_nsamples);
    m_rep_percent = get<double>(cfg, "replacement_percentage", m_rep_percent);
}


bool Gen::AddNoise::operator()(const input_pointer& inframe, output_pointer& outframe)
{
    if (!inframe) {
        outframe = nullptr;
        log->debug("EOS at call={}", m_count);
        ++m_count;
        return true;
    }

    // We "recycle" the randoms in order to speed up generation.  Each
    // array will start from a random location in the recycled buffer
    // and a few percent will be "freshened".  This results in a small
    // amount of coherency between nearby channels.
    // Normals::Fresh rn(m_rng);
    Normals::Recycling rn(m_rng, 2*m_nsamples, 2*m_rep_percent);
    WaveGenerator rwgen(m_dft, rn);

    // Limit number of warnings below
    static bool warned = false;

    // Make waveforms of size nsample from each model, adding only
    // ncharge of their element to the trace charge.  This
    // full-nsample followed by ncharge-truncation may CPU-wasteful in
    // the sparse traces case.

    ITrace::vector outtraces;
    for (const auto& intrace : *inframe->traces()) {

        const int chid = intrace->channel();
        auto charge = intrace->charge(); // copies
        const size_t ncharge = charge.size();

        for (auto& [mtn, model] : m_models) {
            const auto& spec = (*model)(chid);

            // The model spec size may differ than expected nsamples.
            // We could interpolate to correct for that which would
            // slow things down.  Better to correct the model(s) code
            // and configuration.
            if (not warned and spec.size() != m_nsamples) {
                log->warn("model {} produced {} samples instead of expected {}, future warnings muted",
                          mtn, spec.size(), m_nsamples);
                warned = true;
            }

            auto wave = rwgen.wave(spec);
            wave.resize(ncharge);
            Waveform::increase(charge, wave);
        }

        auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), charge);
        outtraces.push_back(trace);
    }
    outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(), outtraces, inframe->tick());
    log->debug("call={} frame={} {} traces",
               m_count, inframe->ident(), outtraces.size());
    ++m_count;
    return true;
}
