#include "WireCellImg/FrameQualityTagging.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/IWaveformMap.h"
#include "WireCellAux/FrameTools.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(FrameQualityTagging, WireCell::Img::FrameQualityTagging, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;

FrameQualityTagging::FrameQualityTagging()
    : Aux::Logger("FrameQualityTagging", "img")
{
}

FrameQualityTagging::~FrameQualityTagging() {}

WireCell::Configuration FrameQualityTagging::default_configuration() const
{
    Configuration cfg;

    //
    cfg["anode"] = "AnodePlane";
    //
    cfg["trace_tag"] = m_trace_tag;
    
    return cfg;
}

void FrameQualityTagging::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    m_trace_tag = get<std::string>(cfg, "trace_tag", m_trace_tag);
}

bool FrameQualityTagging::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    // For Xin
    // bad frame, out = nullptr for now
    auto in_traces = Aux::tagged_traces(in, m_trace_tag);
    if (in_traces.size() == 0) {
        log->debug("no trace tagged as {}", m_trace_tag);
        log->debug("bad frame, out = nullptr for now");
        return true;
    }

    // good frame, pass trhough
    out = in;
    log->debug("good frame, pass trhough");
    return true;
}
