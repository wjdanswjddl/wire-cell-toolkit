#include "WireCellImg/MaskSlice.h"
#include "WireCellImg/ImgData.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellAux/FrameTools.h"
#include "WireCellAux/PlaneTools.h"

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

    // use these traces for error
    cfg["error_tag"] = m_error_tag;

    // 0, 1, 2 for U, V, W
    // WirePlaneId::index()
    cfg["active_planes"][0] = 0;
    cfg["active_planes"][1] = 1;
    cfg["active_planes"][2] = 2;

    //
    cfg["dummy_planes"] = Json::arrayValue;

    //
    cfg["masked_planes"] = Json::arrayValue;

    //
    cfg["dummy_charge"] = m_dummy_charge;
    cfg["dummy_error"] = m_dummy_error;
    cfg["masked_charge"] = m_masked_charge;
    cfg["masked_error"] = m_masked_error;
    // if both are zero, (default) determine tbin range from input
    // frame.
    cfg["min_tbin"] = m_min_tbin;
    cfg["max_tbin"] = m_max_tbin;

    return cfg;
}

void Img::MaskSliceBase::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());  // throws
    m_tick_span = get(cfg, "tick_span", m_tick_span);
    m_tag = get<std::string>(cfg, "tag", m_tag);
    m_error_tag = get<std::string>(cfg, "error_tag", m_error_tag);
    m_dummy_charge = get<float>(cfg, "dummy_charge", m_dummy_charge);
    m_dummy_error = get<float>(cfg, "dummy_error", m_dummy_error);
    m_masked_charge = get<float>(cfg, "masked_charge", m_masked_charge);
    m_masked_error = get<float>(cfg, "masked_error", m_masked_error);
    m_min_tbin = get<int>(cfg, "min_tbin", m_min_tbin);
    m_max_tbin = get<int>(cfg, "max_tbin", m_max_tbin);
    if (cfg.isMember("active_planes")) {
        m_active_planes.clear();
        for (auto id : cfg["active_planes"]) {
            m_active_planes.push_back(id.asInt());
        }
    }
    if (cfg.isMember("dummy_planes")) {
        m_dummy_planes.clear();
        for (auto id : cfg["dummy_planes"]) {
            m_dummy_planes.push_back(id.asInt());
        }
    }
    if (cfg.isMember("masked_planes")) {
        m_masked_planes.clear();
        for (auto id : cfg["masked_planes"]) {
            m_masked_planes.push_back(id.asInt());
        }
    }
    for (auto id : m_active_planes) {
        log->debug("active planes: {}", id);
    }
    for (auto id : m_dummy_planes) {
        log->debug("dummy planes: {}", id);
    }
    for (auto id : m_masked_planes) {
        log->debug("masked planes: {}", id);
    }
}

void Img::MaskSliceBase::slice(const IFrame::pointer& in, slice_map_t& svcmap)
{
    const double tick = in->tick();
    const double span = tick * m_tick_span;

    // active slices
    auto charge_traces = Aux::tagged_traces(in, m_tag);
    const size_t ntraces = charge_traces.size();

    if (!ntraces) {
        log->debug("no traces {} from frame {}", m_tag, in->ident());
        return;
    }

    int min_tbin = m_min_tbin;
    int max_tbin = m_max_tbin;
    if (min_tbin == 0 and max_tbin == 0) {
        std::vector<int> tbins(ntraces, 0), tends(ntraces, 0);
        for (size_t tind = 0; tind<ntraces; ++tind) {
            const auto& tr = charge_traces[tind];
            tends[tind] = tbins[tind] = tr->tbin();
            tends[tind] += tr->charge().size();
        }
        min_tbin = *std::min_element(tbins.begin(), tbins.end());
        max_tbin = *std::max_element(tends.begin(), tends.end());
    }

    auto charge_err_traces = Aux::tagged_traces(in, m_error_tag);
    if(charge_traces.size()!=charge_err_traces.size()) {
        log->error("trace size mismatch: {}: {} and {}: {}",
                   m_tag, charge_traces.size(), m_error_tag, charge_err_traces.size());
        THROW(RuntimeError() << errmsg{"charge_traces.size()!=charge_err_traces.size()"});
    }
    for (size_t idx=0; idx < charge_traces.size(); ++idx) {
        auto trace = charge_traces[idx];
        auto err_trace = charge_err_traces[idx];
        const int tbin = trace->tbin();
        const int chid = trace->channel();
        if (tbin != err_trace->tbin() || chid != err_trace->channel()) {
            THROW(RuntimeError() << errmsg{"tbin != err_trace->tbin() || chid != err_trace->channel()"});
        }
        IChannel::pointer ich = m_anode->channel(chid);
        auto planeid = ich->planeid();
        if (std::find(m_active_planes.begin(),m_active_planes.end(),planeid.index())==m_active_planes.end()) {
            continue;
        }
        const auto& charge = trace->charge();
        const auto& error = err_trace->charge();
        const size_t nq = charge.size();
        for (size_t qind = 0; qind != nq; ++qind) {
            if ((tbin + (int) qind) < min_tbin || (tbin + (int) qind) >= max_tbin) {
                log->warn("trace {} {} exceeds given range [{},{}), breaking.", trace->channel(), trace->tbin(),
                          min_tbin, max_tbin);
                break;
            }
            const auto q = charge[qind];
            const auto e = error[qind];
            // TODO: do we want this?
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
            // TODO: how to handle error?
            s->sum(ich, {q, e});
            if (chid == 0) {
                log->trace("chid: {} slicebin: {} charge: {} error {}", chid, slicebin, s->activity()[ich].value(),
                           s->activity()[ich].uncertainty());
            }
        }
    }

    // dummy: to form slices active through all ticks/channels with special charge/error
    for (int plane_index : m_dummy_planes) {
        auto ichans = Aux::plane_channels(m_anode, plane_index);
        log->debug("dummy plane: {} size {}", plane_index, ichans.size());
        for (auto ich : ichans) {
            for (auto itick = min_tbin; itick < max_tbin; ++itick) {
                size_t slicebin = (min_tbin + itick) / m_tick_span;
                auto s = svcmap[slicebin];
                if (!s) {
                    const double start = slicebin * span;  // thus relative to slice frame's time.
                    s = new Img::Data::Slice(in, slicebin, start, span);
                    svcmap[slicebin] = s;
                }
                s->assign(ich, {m_dummy_charge, m_dummy_error});
            }
        }
    }

    // masked slices
    auto cmm = in->masks()["bad"];
    for (auto ch_tbins : cmm) {
        const int chid = ch_tbins.first;
        auto tbins = ch_tbins.second;
        IChannel::pointer ich = m_anode->channel(chid);
        if (!ich) {
            continue;
        }
        auto planeid = ich->planeid();
        if (std::find(m_masked_planes.begin(),m_masked_planes.end(),planeid.index())==m_masked_planes.end()) {
            continue;
        }
        for (auto tbin : ch_tbins.second) {
            // log->debug("t: {} {}", tbin.first, tbin.second);
            for (auto t = tbin.first; t != tbin.second; ++t) {
                if (t < min_tbin || t >= max_tbin) {
                    log->warn("cmm {} {} exceeds given range [{},{}), breaking.", chid, t, min_tbin, max_tbin);
                    break;
                }
                size_t slicebin = t / m_tick_span;
                auto s = svcmap[slicebin];
                if (!s) {
                    const double start = slicebin * span;  // thus relative to slice frame's time.
                    s = new Img::Data::Slice(in, slicebin, start, span);
                    svcmap[slicebin] = s;
                }
                s->assign(ich, {m_masked_charge, m_masked_error});
            }
        }
    }

    log->debug("nslices={} from ntraces={}, tbin=[{}, {}]",
               svcmap.size(), ntraces, min_tbin, max_tbin);
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

        // log->debug("slice: id={} t={} activity={}", s->ident(), s->start(), s->activity().size());
        slices.push_back(ISlice::pointer(s));
    }

    return true;
}
