#include "WireCellImg/ChargeErrorFrameEstimator.h"
#include "WireCellIface/SimpleFrame.h"
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

bool ChargeErrorFrameEstimator::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    std::stringstream info;
    info << "input " << Aux::taginfo(in) << " ";

    // Basic frame stays the same.
    auto sfout = new SimpleFrame(in->ident(), in->time(), *in->traces(), in->tick(), in->masks());

    for (auto ftag : in->frame_tags()) {
        sfout->tag_frame(ftag);
    }

    for (auto inttag : in->trace_tags()) {
        const auto& traces = in->tagged_traces(inttag);
        const auto& summary = in->trace_summary(inttag);
        sfout->tag_traces(inttag, traces, summary);
    };

    out = IFrame::pointer(sfout);
    info << "output " << Aux::taginfo(out) << " ";

    log->debug(info.str());

    return true;
}
