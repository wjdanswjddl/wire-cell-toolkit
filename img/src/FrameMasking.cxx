#include "WireCellImg/FrameMasking.h"
#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"
#include "WireCellIface/IWaveformMap.h"
#include "WireCellAux/FrameTools.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(FrameMasking, WireCell::Img::FrameMasking, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;

FrameMasking::FrameMasking()
    : Aux::Logger("FrameMasking", "img")
{
}

FrameMasking::~FrameMasking() {}

WireCell::Configuration FrameMasking::default_configuration() const
{
    Configuration cfg;

    //
    cfg["anode"] = "AnodePlane";
    //
    cfg["cm_tag"] = m_cm_tag;
    //
    cfg["trace_tags"][0] = m_trace_tags[0];

    return cfg;
}

void FrameMasking::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    m_cm_tag = get<std::string>(cfg, "cm_tag", m_cm_tag);
    if (cfg.isMember("trace_tags")) {
        m_trace_tags.clear();
        for (auto tag : cfg["trace_tags"]) {
            m_trace_tags.push_back(tag.asString());
        }
    }
}

bool FrameMasking::operator()(const input_pointer& in, output_pointer& out)
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
    log->debug("input: {}", Aux::taginfo(in));

    // make a copy of all input trace pointers
    ITrace::vector out_traces;
    std::unordered_map<std::string, IFrame::trace_list_t> out_trace_indices;
    for (auto tag : m_trace_tags) {
        for (auto trace : Aux::tagged_traces(in, tag)) {
            const int tbin = trace->tbin();
            const int chid = trace->channel();
            auto out_charge = trace->charge();
            if(cm.find(chid)!=cm.end()) {
                const int nq = out_charge.size();
                for(auto bad_range : cm[chid]) {
		  //		  log->debug("{}, {}, {}", chid, bad_range.first, bad_range.second);
                    int mask_start = bad_range.first - tbin > 0 ? bad_range.first - tbin : 0;
                    int mask_end = bad_range.second - tbin < nq ? bad_range.second - tbin : nq;
                    if (mask_start>=mask_end) continue;
                    std::generate(out_charge.begin()+mask_start, out_charge.begin()+mask_end, [] () mutable {return 0;});
                }
            }
            out_trace_indices[tag].push_back(out_traces.size());
            out_traces.push_back(
                std::make_shared<Aux::SimpleTrace>(chid, tbin, out_charge));
        }
    }

    // Basic frame stays the same.
    auto sfout = new Aux::SimpleFrame(in->ident(), in->time(), out_traces, in->tick(), cmm);

    // tag traces
    for (auto tag : m_trace_tags) {
        log->debug("out_trace_indices: {} {}", tag, out_trace_indices[tag].size());
        // TODO: pass through summary?
        const auto& summary = in->trace_summary(tag);
        sfout->tag_traces(tag, out_trace_indices[tag], summary);
    }

    // passing through other parts of the original frame
    for (auto ftag : in->frame_tags()) {
        sfout->tag_frame(ftag);
    }

    out = IFrame::pointer(sfout);

    log->debug("out: {}", Aux::taginfo(out));

    return true;
}
