#include "WireCellImg/ChargeErrorFrameEstimator.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellAux/FrameTools.h"

#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(ChargeErrorFrameEstimator, WireCell::Img::ChargeErrorFrameEstimator, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;

ChargeErrorFrameEstimator::ChargeErrorFrameEstimator()
    : Aux::Logger("ChargeErrorFrameEstimator", "img")
{
}

ChargeErrorFrameEstimator::~ChargeErrorFrameEstimator() {}

WireCell::Configuration ChargeErrorFrameEstimator::default_configuration() const
{
    Configuration cfg;

    /// traces that will be used for error estimation
    cfg["input_tag"] = "gauss";

    /// the tag for the output error traces
    cfg["output_tag"] = "error";

    return cfg;
}

void ChargeErrorFrameEstimator::configure(const WireCell::Configuration& cfg)
{
    m_input_tag = get<std::string>(cfg, "input_tag", m_input_tag);
    m_output_tag = get<std::string>(cfg, "output_tag", m_output_tag);
}

ITrace::vector ChargeErrorFrameEstimator::error_traces(const ITrace::vector& intraces) const {
    ITrace::vector ret;
    for (ITrace::pointer intrace : intraces) {
        auto error = intrace->charge();
        // ---------- begin of the stub for Xin ---------- 
        for (auto& e : error) {
            e /= 10.;
        }
        // ---------- end of the stub for Xin ---------- 
        SimpleTrace* outtrace = new SimpleTrace(intrace->channel(), intrace->tbin(), error);
        ret.push_back(ITrace::pointer(outtrace));
    }
    return ret;
}

bool ChargeErrorFrameEstimator::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    log->debug("input: {}", Aux::taginfo(in));

    // calculate error traces
    ITrace::vector err_traces = error_traces(Aux::tagged_traces(in, m_input_tag));

    // make a copy of all input trace pointers
    ITrace::vector out_traces(*in->traces());

    // list used for the new error trace tagging
    IFrame::trace_list_t out_trace_indices(err_traces.size());
    std::generate(out_trace_indices.begin(), out_trace_indices.end(), [n = out_traces.size()] () mutable {return n++;});

    // merge error traces to the output copy
    out_traces.insert(out_traces.end(), err_traces.begin(), err_traces.end());

    // Basic frame stays the same.
    auto sfout = new SimpleFrame(in->ident(), in->time(), out_traces, in->tick(), in->masks());

    // tag err traces
    sfout->tag_traces(m_output_tag, out_trace_indices);

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
    log->debug("out: {}", Aux::taginfo(out));

    return true;
}
