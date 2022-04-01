#include "WireCellImg/CMMModifier.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/IWaveformMap.h"
#include "WireCellAux/FrameTools.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(CMMModifier, WireCell::Img::CMMModifier, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;

CMMModifier::CMMModifier()
    : Aux::Logger("CMMModifier", "img")
{
}

CMMModifier::~CMMModifier() {}

WireCell::Configuration CMMModifier::default_configuration() const
{
    Configuration cfg;

    //
    cfg["anode"] = "AnodePlane";
    //
    cfg["cm_tag"] = m_cm_tag;
    //
    cfg["trace_tag"] = m_trace_tag;

    return cfg;
}

void CMMModifier::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    m_cm_tag = get<std::string>(cfg, "cm_tag", m_cm_tag);
    m_trace_tag = get<std::string>(cfg, "trace_tag", m_trace_tag);
}

bool CMMModifier::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    // copy a CMM from input frame
    auto cmm = in->masks();
    if (cmm.find(m_cm_tag)==cmm.end()) {
        THROW(RuntimeError()<< errmsg{"no ChannelMask with name "+m_cm_tag});
    }
    auto& cm = cmm[m_cm_tag];
    log->debug("input: {} size: {}", m_cm_tag, cm.size());

    // begin for Xin
    // some dummy ops for now
    cm.clear();
    for (auto trace : Aux::tagged_traces(in, m_trace_tag)) {
        const int tbin = trace->tbin();
        const int chid = trace->channel();
        const auto& charge = trace->charge();
        const size_t nq = charge.size();
        cm[chid].push_back({tbin,tbin+nq});
    }
    // end for Xin

    // make a copy of all input trace pointers
    ITrace::vector out_traces(*in->traces());

    // Basic frame stays the same.
    auto sfout = new SimpleFrame(in->ident(), in->time(), out_traces, in->tick(), cmm);

    // passing through other parts of the original frame
    for (auto ftag : in->frame_tags()) {
        sfout->tag_frame(ftag);
    }

    for (auto inttag : in->trace_tags()) {
        const auto& traces = in->tagged_traces(inttag);
        const auto& summary = in->trace_summary(inttag);
        sfout->tag_traces(inttag, traces, summary);
    };

    out = IFrame::pointer(sfout);

    log->debug("output: {} size: {}", m_cm_tag, out->masks()[m_cm_tag].size());

    return true;
}
