#include "WireCellAux/FrameTensor.h"
#include "WireCellAux/TensorTools.h"

#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(FrameTensor, WireCell::Aux::FrameTensor,
                 WireCell::IFrameTensorSet,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Aux;

FrameTensor::FrameTensor()
{
}
FrameTensor::~FrameTensor()
{
}


bool FrameTensor::operator()(const input_pointer& frame, output_pointer& tens)
{
    tens = nullptr;
    if (! frame) {
        // eos
        return true;
    }

    tens = to_itensorset(frame, m_mode, {}, m_digitize);
    return true;
}
        


WireCell::Configuration FrameTensor::default_configuration() const
{
    std::map<FrameTensorMode, std::string> modes = {
        {FrameTensorMode::unified, "unified"},
        {FrameTensorMode::tagged, "tagged" },
        {FrameTensorMode::sparse, "sparse" }};
    Configuration cfg;
    cfg["mode"] = modes[m_mode];
    cfg["digitize"] = m_digitize;
    return cfg;
}
void FrameTensor::configure(const WireCell::Configuration& cfg)
{
    std::map<std::string, FrameTensorMode> modes = {
        {"unified",  FrameTensorMode::unified},
        {"tagged", FrameTensorMode::tagged },
        {"sparse", FrameTensorMode::sparse }};
    auto mode = get<std::string>(cfg, "mode", "tagged");
    m_mode = modes[mode];
    m_digitize = get(cfg, "digitize", m_digitize);
}

        
