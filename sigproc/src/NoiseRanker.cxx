#include "WireCellSigProc/NoiseRanker.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/NamedFactory.h"
#include <vector>

WIRECELL_FACTORY(NoiseRanker, WireCell::SigProc::NoiseRanker,
                 WireCell::ITraceRanker, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Waveform;
using namespace WireCell::SigProc;

NoiseRanker::NoiseRanker() {}
NoiseRanker::~NoiseRanker() {}

WireCell::Configuration NoiseRanker::default_configuration() const
{
    Configuration cfg;

    /// Maximum deviation from median and still considered noise
    // rank is 1-sum(abs(V_n-median))>maxdev)/N
    cfg["maxdev"] = 0.1*units::volt;

    return cfg;
}

void NoiseRanker::configure(const WireCell::Configuration& cfg)
{
    m_maxdev = get(cfg, "maxdev", 0.1*units::volt);
}

double NoiseRanker::rank(const WireCell::ITrace::pointer& trace)
{
    const auto& charge = trace->charge();
    const auto med = median(charge);
    const size_t siz = charge.size();
    size_t outliers = 0;
    for (size_t ind=0; ind<siz; ++ind) {
        if (std::abs(charge[ind]-med) > m_maxdev) {
            ++outliers;
        }
    }
    return 1.0 - (double)outliers/(double)siz;    
}
