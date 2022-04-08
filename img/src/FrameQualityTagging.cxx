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
    cfg["gauss_trace_tag"] = m_gauss_trace_tag;
    //
    cfg["wiener_trace_tag"] = m_wiener_trace_tag;
    //
    cfg["tick0"] = m_tick0;
    cfg["nticks"] = m_nticks;

    
    return cfg;
}

void FrameQualityTagging::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    m_gauss_trace_tag = get<std::string>(cfg, "gauss_trace_tag", m_gauss_trace_tag);
    m_wiener_trace_tag = get<std::string>(cfg, "wiener_trace_tag", m_wiener_trace_tag);
    m_cm_tag = get<std::string>(cfg, "cm_tag", m_cm_tag);
    m_tick0 = get<int>(cfg, "tick0", m_tick0);
    m_nticks = get<int>(cfg, "nticks", m_nticks);
}

bool FrameQualityTagging::operator()(const input_pointer& in, output_pointer& out)
{
    // frame quality
    // no mechanism to consume it for now
    int frame_quality = 0;

    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    // Prepare a cmm for output
    auto cmm = in->masks();
    if (cmm.find(m_cm_tag)==cmm.end()) {
        THROW(RuntimeError()<< errmsg{"no ChannelMask with name "+m_cm_tag});
    }
    auto& cm = cmm[m_cm_tag];
    log->debug("input: {} size: {}", m_cm_tag, cm.size());

    // Sort out channel list for each plane
    std::unordered_map<int, Aux::channel_list> chbyplane;
    m_anode->channels();
    for (auto channel : m_anode->channels()) {
        auto iplane = (m_anode->resolve(channel)).index();
        if (iplane < 0 || iplane >= 3) {
            THROW(RuntimeError() << errmsg{"Illegal wpid"});
        }
        // TODO: asumming this sorted?
        chbyplane[iplane].push_back(channel);
    }

    // Prepare Eigen matrices for gauss, wiener
    std::vector<size_t> nchannels = {chbyplane[0].size(), chbyplane[1].size(), chbyplane[2].size()};
    log->debug("nchannels: {} {} {}", nchannels[0], nchannels[1], nchannels[2]);
    std::vector<Array::array_xxf> arr_gauss;
    std::vector<Array::array_xxf> arr_wiener;
    auto trace_gauss = Aux::tagged_traces(in, m_gauss_trace_tag);
    auto trace_wiener = Aux::tagged_traces(in, m_wiener_trace_tag);
    for (int iplane=0; iplane< 3; ++iplane) {
        arr_gauss.push_back(Array::array_xxf::Zero(nchannels[iplane], m_nticks));
        Aux::fill(arr_gauss.back(), trace_gauss, chbyplane[iplane].begin(), chbyplane[iplane].end(), m_tick0);
        arr_wiener.push_back(Array::array_xxf::Zero(nchannels[iplane], m_nticks));
        Aux::fill(arr_wiener.back(), trace_wiener, chbyplane[iplane].begin(), chbyplane[iplane].end(), m_tick0);
    }
    std::cout << arr_gauss[0] << std::endl;

    // Prepare threshold for wiener
    auto threshold = in->trace_summary(m_wiener_trace_tag);
    log->debug("threshold.size() {}", threshold.size());

    // begin for Xin
    // end for Xin

    // good frame, pass trhough
    if (frame_quality == 0 || frame_quality == 1) {
        log->debug("good frame, pass trhough");
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

    // bad frame, out = nullptr for now
    return true;
}
