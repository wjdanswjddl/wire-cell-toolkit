#include "WireCellImg/MaskSlice.h"
#include "WireCellImg/ImgData.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"

#include "WireCellAux/FrameTools.h"

WIRECELL_FACTORY(MaskSlicer, WireCell::Img::MaskSlicer,
                 WireCell::INamed,
                 WireCell::IFrameSlicer, WireCell::IConfigurable)

WIRECELL_FACTORY(MaskSlices, WireCell::Img::MaskSlices,
                 WireCell::INamed,
                 WireCell::IFrameSlices, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Img::MaskSliceBase::MaskSliceBase()
    : Aux::Logger("MaskSlice", "img")
    , m_tick_span(4)
    , m_tag("")
{
}
Img::MaskSliceBase::~MaskSliceBase() {}
Img::MaskSlicer::~MaskSlicer() {}
Img::MaskSlices::~MaskSlices() {}

WireCell::Configuration Img::MaskSliceBase::default_configuration() const
{
    Configuration cfg;

    // Name of an IAnodePlane from which we can resolve channel ident to IChannel
    cfg["anode"] = "";

    // Number of ticks over which each output slice should sum
    cfg["tick_span"] = m_tick_span;

    // If given, use tagged traces.  Otherwise use all traces.
    cfg["tag"] = m_tag;

    //
    cfg["active_planes"][0] = WireCell::kUlayer;
    cfg["active_planes"][1] = WireCell::kVlayer;
    cfg["active_planes"][2] = WireCell::kWlayer;


    //
    cfg["masked_plane_charge"] = Json::arrayValue;

    //
    cfg["tmax"] = -1;

    return cfg;
}

void Img::MaskSliceBase::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());  // throws
    m_tick_span = get(cfg, "tick_span", m_tick_span);
    m_tag = get<std::string>(cfg, "tag", m_tag);
    m_tmax = get<int>(cfg, "tmax", m_tmax);
    for (auto id : cfg["active_planes"]) {
        m_active_planes.push_back(static_cast<WireCell::WirePlaneLayer_t>(id.asInt()));
    }
    for (auto pc : cfg["masked_plane_charge"]) {
        m_masked_plane_charge[static_cast<WireCell::WirePlaneLayer_t>(pc[0].asInt())] = pc[1].asInt();
    }

}

void Img::MaskSliceBase::slice(const IFrame::pointer& in, slice_map_t& svcmap)
{
    const double tick = in->tick();
    const double span = tick * m_tick_span;

    // active slices
    for (auto trace : Aux::tagged_traces(in, m_tag)) {
        const int tbin = trace->tbin();
        const int chid = trace->channel();
        IChannel::pointer ich = m_anode->channel(chid);
        auto planeid = ich->planeid();
        if (std::find(m_active_planes.begin(),m_active_planes.end(),planeid.layer())==m_active_planes.end()) {
            continue;
        }
        const auto& charge = trace->charge();
        const size_t nq = charge.size();
        for (size_t qind = 0; qind != nq; ++qind) {
            const auto q = charge[qind];
            if (q == 0.0) {
                continue;
            }
            size_t slicebin = (tbin + qind) / m_tick_span;
            auto s = svcmap[slicebin];
            if (!s) {
                const double start = slicebin * span;  // thus relative to slice frame's time.
                s = new Img::Data::Slice(in, slicebin, start, span);
                svcmap[slicebin] = s;
            }
            s->sum(ich, q);

            if(m_tmax > 0 && (tbin + (int)qind) > m_tmax) break;
        }
    }

    // masked slices
    auto cmm = in->masks()["bad"];
    for (auto ch_tbins : cmm) {
        const int chid = ch_tbins.first;
        auto tbins = ch_tbins.second;
        IChannel::pointer ich = m_anode->channel(chid);
        auto planeid = ich->planeid();
        if (m_masked_plane_charge.find(planeid.layer())==m_masked_plane_charge.end()) continue;
        auto q = m_masked_plane_charge[planeid.layer()];
        for (auto tbin : ch_tbins.second) {
            // log->debug("t: {} {}", tbin.first, tbin.second);
            for (auto t = tbin.first; t!= tbin.second; ++t) {
                size_t slicebin = t / m_tick_span;
                auto s = svcmap[slicebin];
                if (!s) {
                    const double start = slicebin * span;  // thus relative to slice frame's time.
                    s = new Img::Data::Slice(in, slicebin, start, span);
                    svcmap[slicebin] = s;
                }
                s->assign(ich, q);

                if(m_tmax > 0 && t > m_tmax) break;
            }
        }
    }

    log->debug("Img::MaskSliceBase::slice done.");
}

bool Img::MaskSlicer::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        return true;  // eos
    }

    // Slices will be sparse in general.  Index by a "slice bin" number
    slice_map_t svcmap;
    slice(in, svcmap);

    // intern
    ISlice::vector islices;
    for (auto sit : svcmap) {
        auto s = sit.second;
        islices.push_back(ISlice::pointer(s));
    }
    out = make_shared<Img::Data::SliceFrame>(islices, in->ident(), in->time());

    return true;
}

bool Img::MaskSlices::operator()(const input_pointer& in, output_queue& slices)
{
    if (!in) {
        log->debug("EOS");
        slices.push_back(nullptr);
        return true;  // eos
    }

    // Slices will be sparse in general.  Index by a "slice bin" number
    slice_map_t svcmap;
    slice(in, svcmap);

    // intern
    for (auto sit : svcmap) {
        auto s = sit.second;

        /// debug
        // double qtot = 0;
        // for (const auto& a : s->activity()) {
        //     qtot += a.second;
        // }

        slices.push_back(ISlice::pointer(s));
    }

    log->debug("frame={}, make {} slices in [{},{}] from {}",
               in->ident(), slices.size(),
               slices.front()->ident(), slices.back()->ident(),
               svcmap.size());
    return true;
}
