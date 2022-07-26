#include "WireCellSigProc/Undigitizer.h"

#include "WireCellAux/FrameTools.h"

#include "WireCellIface/IAnodePlane.h"
#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Persist.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Interpolate.h"
#include "WireCellUtil/NamedFactory.h"

#include <vector>

WIRECELL_FACTORY(Undigitizer, WireCell::SigProc::Undigitizer,
                 WireCell::INamed,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::SigProc;

Undigitizer::Undigitizer()
    : Aux::Logger("Undigitizer", "sigproc")
{
}

Undigitizer::~Undigitizer() 
{
}

WireCell::Configuration Undigitizer::default_configuration() const
{
    Configuration cfg;

    // The type:instance name of an AnodePlane
    cfg["anode"] = Json::stringValue;

    // These all basically configured the same as Digitzer from gen,
    // (but works to invert Digitizer's transform).
    // In [bits]
    cfg["resolution"] = Json::intValue;
    // linear ratio
    cfg["gain"] = Json::realValue;
    // in [voltage]
    cfg["fullscale"] = Json::arrayValue;
    // in [voltage]
    cfg["baselines"] = Json::arrayValue;

    return cfg;
}


void Undigitizer::configure(const WireCell::Configuration& cfg)
{
    int errors = 0;

    auto anode_tn = get<std::string>(cfg, "anode", "");
    if (anode_tn.empty()) {
        log->critical("configuration parameter \"anode\" is empty");
        ++errors;
    }        
    auto anode = Factory::find_tn<IAnodePlane>(anode_tn);

    const int bits = cfg["resolution"].asInt();
    const double gain = cfg["gain"].asDouble();

    const auto fullscale = get<std::vector<double>>(cfg, "fullscale");
    if (2 != fullscale.size()) {
        log->critical("fullscale must be two numbers");
        ++errors;
    }

    const auto baselines = get<std::vector<double>>(cfg, "baselines");
    if (baselines.empty()) {
        log->critical("baselines array is empty");
        ++errors;
    }

    if (errors) {
        THROW(ValueError() << errmsg{"configuration error"});
    }

    // Digitizer: Vin/Vout refers to G/BL, not ADC.
    // Vout = G*Vin + baseline
    // ADC = ADCmax * (Vout - fs[0]) / (fs[1] - fs[0])
    const float adcmin = 0;
    const float adcmax = 1<<bits;

    // bundle it all into m_dac function.
    m_dac = [=](const ITrace::pointer& trace) -> ITrace::pointer {
        int chid = trace->channel();
        const WirePlaneId wpid = anode->resolve(chid);
        const int plane = wpid.index();
        const auto& adcs = trace->charge();
        const size_t nticks = adcs.size();
        ITrace::ChargeSequence voltage(nticks, 0);
        for (size_t ind=0; ind<nticks; ++ind) {
            float adc = adcs[ind];
            adc = std::max(adc, adcmin);
            adc = std::min(adc, adcmax);
            const float vout = adc/adcmax*(fullscale[1]-fullscale[0]) + fullscale[0];
            voltage[ind] = (vout-baselines[plane])/gain;
        }
        return std::make_shared<Aux::SimpleTrace>(trace->channel(), trace->tbin(), voltage);
    };        
        
}
bool Undigitizer::operator()(const input_pointer& adcframe, output_pointer& vframe)
{
    vframe = nullptr;
    if (!adcframe) {
        log->debug("EOS at count={}", m_count);
        ++m_count;
        return true;
    }

    auto adctraces = Aux::untagged_traces(adcframe);
    if (adctraces.empty()) {
        log->debug("no traces in input frame {} at call={}",
                   adcframe->ident(), m_count);
        ++m_count;
        return true;
    }

    ITrace::vector vtraces;
    for (const auto& trace : adctraces) {
        vtraces.push_back(m_dac(trace));
    }
    vframe = std::make_shared<Aux::SimpleFrame>(adcframe->ident(),
                                                adcframe->time(),
                                                vtraces,
                                                adcframe->tick());
    log->debug("frame {} with {} traces at call={}",
               vframe->ident(), vtraces.size(), m_count);
    ++m_count;
    return true;
}


