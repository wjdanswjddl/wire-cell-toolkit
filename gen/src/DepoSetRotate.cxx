#include "WireCellGen/DepoSetRotate.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellAux/SimpleDepoSet.h"
#include "WireCellAux/SimpleDepo.h"

WIRECELL_FACTORY(DepoSetRotate, WireCell::Gen::DepoSetRotate,
                 WireCell::INamed,
                 WireCell::IDepoSetFilter, WireCell::IConfigurable)


using namespace WireCell;
using namespace WireCell::Gen;

DepoSetRotate::DepoSetRotate()
    : Aux::Logger("DepoSetRotate", "gen")
{
}
DepoSetRotate::~DepoSetRotate()
{
}

WireCell::Configuration DepoSetRotate::default_configuration() const
{
    Configuration cfg;

    // set the transpose and scale with a three element array
    cfg["rotate"] = false;
    cfg["transpose"] = Json::arrayValue;
    cfg["scale"] = Json::arrayValue;
    return cfg;
}

void DepoSetRotate::configure(const WireCell::Configuration& cfg)
{
    m_rotate = get(cfg, "rotate", false);

    if (! cfg["transpose"].isNull()) {
        m_transpose = std::tuple<int, int, int>(
            cfg["transpose"][0].asInt(), cfg["transpose"][1].asInt(), cfg["transpose"][2].asInt());
    }
    if (! cfg["scale"].isNull()) {
        m_scale = std::tuple<double, double, double>(
            cfg["scale"][0].asDouble(), cfg["scale"][1].asDouble(), cfg["scale"][2].asDouble());
    }
}

bool DepoSetRotate::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {                  // EOS
        log->debug("EOS at call={}", m_count);
        return true;
    }

    IDepo::vector all_depos;
    for (auto depo: *(in->depos())) {
      if (m_rotate) {
        auto pos = depo->pos();
        WireCell::Point newpos;
        newpos.x(pos[std::get<0>(m_transpose)] * std::get<0>(m_scale)); // apply transpose and scale
        newpos.y(pos[std::get<1>(m_transpose)] * std::get<1>(m_scale));
        newpos.z(pos[std::get<2>(m_transpose)] * std::get<2>(m_scale));
        auto newdepo = std::make_shared<Aux::SimpleDepo>(depo->time(), newpos, depo->charge(), depo, depo->extent_long(), depo->extent_tran());
        all_depos.push_back(newdepo);
      }
    }
 
    log->debug("call={} rotated ndepos={}", m_count, all_depos.size());
    out = std::make_shared<Aux::SimpleDepoSet>(m_count, all_depos);
    ++m_count;
    return true;
}

