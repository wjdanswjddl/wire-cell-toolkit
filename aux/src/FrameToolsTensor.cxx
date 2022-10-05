// break out this long code to keep FrameTools.cxx more brief.

#include "WireCellAux/FrameTools.h"
#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellAux/SimpleTensorSet.h"

#include "WireCellUtil/Dtype.h"
#include "WireCellUtil/Waveform.h"

using namespace WireCell;
using namespace  WireCell::Waveform;
using WireCell::Aux::SimpleTrace;
using WireCell::Aux::SimpleFrame;
using WireCell::Aux::SimpleTensor;
using WireCell::Aux::SimpleTensorSet;


template<typename T1, typename T2=float>
std::vector<T2> to_vector(const std::byte* data, size_t nbytes)
{
    const T1* tdata = (const T1*)data;
    const size_t nele = nbytes / sizeof(T1);
    std::vector<T2> ele(tdata, tdata+nele);
    return ele;
}


IFrame::pointer Aux::to_iframe(ITensorSet::pointer itens)
{
    ITrace::vector all_traces;

    std::map<std::string, std::vector<size_t>> indices;
    std::map<std::string, std::vector<double>> summaries;

    for (auto iten : *(itens->tensors().get())) {

        auto md = iten->metadata();

        std::string type = md["type"].asString();

        if (type == "trace") {
            int tbin = md["tbin"].asInt();
            const auto shape = iten->shape();
            const size_t first_index = all_traces.size();
            if (shape.size() == 1) { // single sparse trace
                int chid = md["chid"].asInt();
                std::vector<float> samples;
                if (dtype(iten->element_type()) == dtype<int16_t>()) {
                    samples = to_vector<int16_t>(iten->data(), iten->size());
                }
                else {
                    samples = to_vector<float>(iten->data(), iten->size());
                }
                all_traces.push_back(std::make_shared<SimpleTrace>(chid, tbin, samples));
            }
            else if (shape.size() == 2) {
                const size_t nrows = shape[0];
                const size_t ncols = shape[1];
                const size_t vsize = ncols * iten->element_size();
                const std::byte* bytes = iten->data();
                for (size_t row = 0; row < nrows; ++row) {
                    const int chid = md["chid"][(int)row].asInt();
                    std::vector<float> samples;
                    if (dtype(iten->element_type()) == dtype<int16_t>()) {
                        samples = to_vector<int16_t>(bytes, vsize);
                    }
                    else {
                        samples = to_vector<float>(bytes, vsize);
                    }
                    all_traces.push_back(std::make_shared<SimpleTrace>(chid, tbin, samples));
                }
            }
            else {
                return nullptr;
            }

            // check for implicit tag
            auto jtag = md["tag"];
            if (jtag.isNull()) {
                continue;
            }
            std::string tag = jtag.asString();
            const size_t nind = all_traces.size() - first_index;
            std::vector<size_t> inds(nind);
            std::iota(inds.begin(), inds.end(), first_index);
            indices[tag] = inds;
            continue;
        }   // trace

        if (type == "index") {
            std::string tag = md["tag"].asString();
            std::vector<size_t> inds;
            if (dtype(iten->element_type()) == dtype<int>()) {
                inds = to_vector<int, size_t>(iten->data(), iten->size());
            }
            if (dtype(iten->element_type()) == dtype<size_t>()) {
                inds = to_vector<size_t,size_t>(iten->data(), iten->size());
            }
            indices[tag] = inds;
            continue;
        }
    
        if (type == "summary") {
            std::string tag = md["tag"].asString();
            std::vector<double> sums;
            if (dtype(iten->element_type()) == dtype<int>()) {
                sums = to_vector<int,double>(iten->data(), iten->size());
            }
            if (dtype(iten->element_type()) == dtype<float>()) {
                sums = to_vector<float,double>(iten->data(), iten->size());
            }
            if (dtype(iten->element_type()) == dtype<double>()) {
                sums = to_vector<double,double>(iten->data(), iten->size());
            }
            summaries[tag] = sums;
            continue;
        }

    } // over tensors

    auto top = itens->metadata();

    ChannelMaskMap cmm;
    auto jcmm = top["masks"];
    if (! jcmm.isNull()) {

        for (auto jcmm_it = jcmm.begin(); jcmm_it != jcmm.end(); ++jcmm_it) {

            const auto& jtag = jcmm_it.key();
            const auto& jcms = *jcmm_it;
            auto& cms = cmm[jtag.asString()];

            for (auto jcms_it = jcms.begin(); jcms_it != jcms.end(); ++jcms_it) {

                const auto& jchid = jcms_it.key();
                const auto& jbrl = *jcms_it;
                auto& brl = cms[jchid.asInt()];

                for (const auto& jpair : jbrl) {
                    brl.push_back(std::make_pair(jpair[0].asInt(),
                                                 jpair[1].asInt()));
                }
            }
        }
    }

    auto sf = std::make_shared<SimpleFrame>(itens->ident(),
                                            top["time"].asDouble(),
                                            all_traces,
                                            top["tick"].asDouble(),
                                            cmm);
    for (auto jtag : top["tags"]) {
        sf->tag_frame(jtag.asString());
    }
    for (const auto& [tag, inds] : indices) {
        IFrame::trace_summary_t sums;
        auto sit = summaries.find(tag);
        if (sit != summaries.end()) {
            sums = sit->second;
        }
        sf->tag_traces(tag, inds, sums);
    }
    return sf;
}

// fixme: these should probably go somewhere public
static
Configuration toconfig(const Waveform::ChannelMasks& cms)
{
    Configuration ret = Json::objectValue;
    for (const auto& [chid, br] : cms) {
        Configuration each = Json::arrayValue;
        for (const auto& [first,second] : br) {
            Configuration pair = Json::arrayValue;
            pair.append(first);
            pair.append(second);
            each.append(pair);
        }
        ret[chid] = each;
    }
    return ret;
}
static
Configuration toconfig(const Waveform::ChannelMaskMap& cmm)
{
    Configuration ret = Json::objectValue;
    for (const auto& [tag, cms] : cmm) {
        ret[tag] = toconfig(cms);
    }
    return ret;
}


static
ITensor::pointer to_itensor_trace2d(const ITrace::vector& traces,
                                    std::string name, std::string tag,
                                    bool truncate,
                                    std::function<float(float)> transform)
{
    auto chids = Aux::channels(traces);
    auto tmm = Aux::tbin_range(traces);
    const size_t nticks = tmm.second-tmm.first;
    const ITensor::shape_t shape = {chids.size(), nticks};
    Array::array_xxf array(shape[0], shape[1]);
    Aux::fill(array, traces, chids.begin(), chids.end(), tmm.first);
    Array::array_xxf trans = array.unaryExpr(transform);
    Configuration tmd;
    tmd["type"] = "trace";
    tmd["name"] = name;
    tmd["tbin"] = tmm.first;
    assign(tmd["chid"], chids);
    if (tag.size() > 0) {
        tmd["tag"] = tag;
    }
    if (truncate) {
        using ROWI = Eigen::Array<int16_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        ROWI itrans = trans.cast<int16_t>();
        return std::make_shared<SimpleTensor<int16_t>>(shape, itrans.data(), tmd);
    }
    using ROWF = Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    ROWF ftrans = trans;
    return std::make_shared<SimpleTensor<float>>(shape, ftrans.data(), tmd);
}

static
ITensor::pointer to_itensor_trace1d(ITrace::pointer trace,
                                    std::string name, 
                                    bool truncate,
                                    std::function<float(float)> transform)                              
{
    Configuration tmd;
    tmd["type"] = "trace";
    tmd["name"] = name;
    tmd["tbin"] = trace->tbin();
    tmd["chid"] = trace->channel();
    auto q = trace->charge();
    std::transform(q.cbegin(), q.cend(), q.begin(), transform);
    const ITensor::shape_t shape = {q.size()};
    if (truncate) {
        std::vector<int16_t> iq(q.size());
        std::transform(q.cbegin(), q.cend(), iq.begin(), [](float x) -> int16_t { return (int16_t)x; });
        return std::make_shared<SimpleTensor<int16_t>>(shape, iq.data(), tmd);
    }
    return std::make_shared<SimpleTensor<float>>(shape, q.data(), tmd);
}

static
ITensor::pointer to_itensor_index(const IFrame::trace_list_t& indices,
                                  std::string name, std::string tag)
{
    Configuration tmd;
    tmd["type"] = "index";
    tmd["name"] = name;
    if (tag.size() > 0) {
        tmd["tag"] = tag;
    }
    const ITensor::shape_t shape = {indices.size()};
    return std::make_shared<SimpleTensor<size_t>>(shape, indices.data(), tmd);
}

static
ITensor::pointer to_itensor_summary(const IFrame::trace_summary_t& ts, 
                                    std::string name, std::string tag)
{
    Configuration tmd;
    tmd["type"] = "summary";
    tmd["name"] = name;
    if (tag.size() > 0) {
        tmd["tag"] = tag;
    }
    const ITensor::shape_t shape = {ts.size()};
    return std::make_shared<SimpleTensor<double>>(shape, ts.data(), tmd);
}


ITensorSet::pointer
Aux::to_itensorset(IFrame::pointer frame,
                   FrameTensorMode mode,
                   Configuration md,
                   bool truncate,
                   std::function<float(float)> transform)
{
    md["time"] = frame->time();
    md["tick"] = frame->tick();
    assign(md["tags"], frame->frame_tags());
    md["masks"] = toconfig(frame->masks());

    auto tenv = std::make_shared<ITensor::vector>();

    if (mode == FrameTensorMode::tagged) {
        for (const auto& tag : frame->trace_tags()) {
            auto name = tag;
            auto tts = tagged_traces(frame, tag);
            auto iten = to_itensor_trace2d(tts, name, tag, truncate, transform);
            tenv->push_back(iten);
        }
    }
    else if (mode == FrameTensorMode::unified) {
        auto traces = frame->traces();
        auto iten = to_itensor_trace2d(*(traces.get()), "", "", truncate, transform);
        tenv->push_back(iten);
    }
    else if (mode == FrameTensorMode::sparse) {
        int count=0;
        auto traces = frame->traces();
        for (const auto& trace : *(traces.get())) {
            auto name = std::to_string(count++);
            auto iten = to_itensor_trace1d(trace, name, truncate, transform);
            tenv->push_back(iten);
        }
    }
    else {
        return nullptr;
    }

    for (const auto& tag : frame->trace_tags()) {
        IFrame::trace_list_t indices = frame->tagged_traces(tag);
        if (indices.empty()) { continue; }
        auto iten = to_itensor_index(indices, tag, tag);
        tenv->push_back(iten);

        auto ts = frame->trace_summary(tag);
        if (ts.empty()) { continue; }
        iten = to_itensor_summary(ts, tag, tag);
        tenv->push_back(iten);
    }
    return std::make_shared<SimpleTensorSet>(frame->ident(), md, tenv);
}
