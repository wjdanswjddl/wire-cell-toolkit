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

    ///
    cfg["input_tag"] = "gauss";

    ///
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
        for (auto& e : error) {
            e /= 10.;
        }
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

    ITrace::vector err_traces = error_traces(Aux::tagged_traces(in, m_input_tag));
    ITrace::vector out_traces(*in->traces());
    IFrame::trace_list_t out_trace_indices;
    {
        auto index = out_traces.size();
        out_traces.insert(out_traces.end(), err_traces.begin(), err_traces.end());
        for (; index < out_traces.size(); ++index) {
            out_trace_indices.push_back(index);
        }
    }

    // Basic frame stays the same.
    auto sfout = new SimpleFrame(in->ident(), in->time(), out_traces, in->tick(), in->masks());

    // tag err traces
    sfout->tag_traces(m_output_tag, out_trace_indices);

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
