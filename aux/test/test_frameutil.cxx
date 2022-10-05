#include "WireCellAux/FrameTools.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Units.h"
#include "WireCellAux/SimpleFrame.h"
#include "WireCellAux/SimpleTrace.h"

#include <iostream>

using namespace std;
using namespace WireCell;
using namespace WireCell::Aux;
using WireCell::Aux::SimpleTrace;
using WireCell::Aux::SimpleFrame;

void test_tensor(IFrame::pointer frame, FrameTensorMode mode, bool truncate,
                 size_t ntens, size_t nchans, std::string frame_tag)
{
    std::cerr << "test frame/tensor round trip with"
              << " mode=" << (int)mode
              << " truncate=" << truncate
              << " ntens=" << ntens
              << " nchans " << nchans
              << " frame_tag=" << frame_tag
              << "\n";

    auto itens = to_itensorset(frame, mode, {}, truncate);
    Assert(itens);

    auto top = itens->metadata();
    Assert(top["tags"][0].asString() == frame_tag);

    size_t ntensgot = itens->tensors()->size();
    std::cerr << ntensgot << " tensors\n";
    Assert(ntensgot == ntens);

    size_t nchansgot = 0;
    for (auto iten : *(itens->tensors().get())) {
        auto md = iten->metadata();
        auto type = md["type"].asString();
        if (type == "trace") {
            auto jchid = md["chid"];
            if (jchid.isInt()) {
                nchansgot += 1;
            }
            else {
                nchansgot += jchid.size();
            }
        }
        if (type == "index") {
            auto name = md["name"].asString();
            Assert(name == "evench" or name == "oddch");
            auto shape = iten->shape();
            Assert(shape.size() == 1);
            Assert(shape[0] == 2);
        }
    }
    Assert(nchansgot == nchans);

    // round trip back to frame
    auto frame2 = to_iframe(itens);
    Assert(frame2->ident() == frame->ident());
    Assert(frame2->time() == frame->time());
    Assert(frame2->tick() == frame->tick());
    {
        // check traces
        auto ts1 = frame->traces();
        auto ts2 = frame2->traces();
        Assert(ts1->size() == ts2->size());
        std::set<int> chids1, chids2;
        for (size_t ind=0; ind<ts1->size(); ++ind) {
            auto t1 = ts1->at(ind);
            auto t2 = ts2->at(ind);
            chids1.insert(t1->channel());
            chids2.insert(t2->channel());
            std::cerr << "trace: " << ind << "/" << ts1->size()
                      << " chan: " << t1->channel() << " =?= " << t2->channel()
                      << ", tbin: " << t1->tbin() << " =?= " << t2->tbin()
                      << "\n";
            if (mode == FrameTensorMode::sparse) {
                // Only get exact representation round-trip in sparse mode
                Assert(t1->channel() == t2->channel());
                Assert(t1->tbin() == t2->tbin());

                const auto& qs1 = t1->charge();
                const auto& qs2 = t2->charge();
                std::cerr << "trace: " << ind << "/" << ts1->size()
                          << " sizes: " << qs1.size() << " =?= " << qs2.size() << "\n";
                Assert(qs1.size() == qs2.size());

                for (size_t iq=0; iq < qs1.size(); ++iq) {
                    float q1 = qs1[iq];
                    float q2 = qs2[iq];
                    if (truncate) {
                        // I/O truncated to int16_t, so must do likewise in this check
                        q1 = (int16_t)q1;
                    }

                    if (q1 != q2) {
                        std::cerr << "charge mismatch: " <<  q1 << " != " <<  q2 << "\n";
                    }
                    Assert(q1 == q2);
                }
            }
        }
        Assert(chids1 == chids2);
    }
    {
        // check channel mask map
        auto m1 = frame->masks();
        auto m2 = frame2->masks();
        Assert(m1.size() == m2.size());
        for (const auto& [tag, cms] : m1) {
            auto it2 = m2.find(tag);
            Assert(it2 != m2.end());
            Assert(it2->second.size() == cms.size());
        }
        for (const auto& [tag, cms] : m2) {
            auto it1 = m1.find(tag);
            Assert(it1 != m1.end());
            Assert(it1->second.size() == cms.size());
        }
    }
    // only get exact representation round trip in sparse mode
    if (mode == FrameTensorMode::sparse) {
        // check tagged traces and summaries
        const auto tts1 = frame->trace_tags();
        const auto tts2 = frame2->trace_tags();
        Assert(tts1.size() == tts2.size());
        std::set ttss1(tts1.begin(), tts1.end());
        std::set ttss2(tts2.begin(), tts2.end());
        Assert(ttss1 == ttss2);

        for (const auto& tag : frame->trace_tags()) {
            const auto& tt1 = frame->tagged_traces(tag);
            const auto& tt2 = frame2->tagged_traces(tag);
            std::cerr << "tagged traces: " << tt1.size() << " =?= " << tt2.size() << "\n";
            Assert(tt1.size() == tt2.size());
            for (size_t ind=0; ind<tt1.size(); ++ind) {
                Assert(tt1[ind] == tt2[ind]);
            }

            const auto& ts1 = frame->trace_summary(tag);
            const auto& ts2 = frame2->trace_summary(tag);
            Assert(ts1.size() == ts2.size());
            for (size_t ind=0; ind<ts1.size(); ++ind) {
                Assert(ts1[ind] == ts2[ind]);
            }
        }
    }

    // And one more back to tensor
    auto itens2 = to_itensorset(frame2, mode, {}, truncate);
    Assert(itens2);
    Assert(itens->ident() == itens2->ident());
    Assert(itens->metadata() == itens2->metadata());
    if (mode == FrameTensorMode::sparse) {
        auto tens1 = itens->tensors();
        auto tens2 = itens2->tensors();
        Assert(tens1->size() == tens2->size());
        for (size_t ind=0; ind<tens1->size(); ++ind) {
            auto ten1 = tens1->at(ind);
            auto ten2 = tens2->at(ind);
            Assert(ten1->metadata() == ten2->metadata());
            std::cerr << "tensor sizes: " << ten1->size() << " =?= " << ten2->size() << "\n";
            Assert(ten1->size() == ten2->size());            
        }
    }
}

void test_tensors(IFrame::pointer signal, IFrame::pointer noise)
{
    const size_t nchan = 4;
    const size_t nindices = 2;
    const size_t nunified = nindices + 1;
    const size_t nsparse = nindices + nchan;
    const size_t ntagged = nindices + 2;

    // 3x2x2

    const std::vector<bool> tf = {true, false};

    for (auto truncate : tf) {
        // signal has 4 channels, 2 unique
        test_tensor(signal, FrameTensorMode::unified, truncate, nunified, nchan, "signal");
        test_tensor(signal, FrameTensorMode::tagged,  truncate, ntagged,  nchan, "signal");
        test_tensor(signal, FrameTensorMode::sparse,  truncate, nsparse,  nchan, "signal");

        // noise has 4 unique channels
        test_tensor(noise, FrameTensorMode::unified, truncate, nunified, nchan, "noise");
        test_tensor(noise, FrameTensorMode::tagged,  truncate, ntagged,  nchan, "noise");
        test_tensor(noise, FrameTensorMode::sparse,  truncate, nsparse,  nchan, "noise");
    }
}

int main()
{
    const double tick = 0.5 * units::us;
    const double readout = 5 * units::ms;
    const int nsamples = round(readout / tick);

    // "Signal"
    const int signal_frame_ident = 100;
    const double signal_start_time = 6 * units::ms;
    vector<float> signal{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 0.0};

    ITrace::vector signal_traces{make_shared<SimpleTrace>(1, 10, signal), make_shared<SimpleTrace>(1, 20, signal),
                                 make_shared<SimpleTrace>(2, 30, signal), make_shared<SimpleTrace>(2, 33, signal)};
    {
        auto sbr = tbin_range(signal_traces);
        std::cerr << "signal tbin range=[" << sbr.first << "," << sbr.second << "]\n";
        Assert(sbr.first == 10 and sbr.second == 44);
    }

    auto f1 = make_shared<SimpleFrame>(signal_frame_ident, signal_start_time, signal_traces, tick);
    f1->tag_frame("signal");
    f1->tag_traces("oddch", {0,2});
    f1->tag_traces("evench", {1,3});

    // "Noise"
    const int noise_frame_ident = 0;
    const double noise_start_time = 5 * units::ms;
    vector<float> noise(nsamples, -0.5);
    ITrace::vector noise_traces{make_shared<SimpleTrace>(0, 0, noise), make_shared<SimpleTrace>(1, 0, noise),
                                make_shared<SimpleTrace>(2, 0, noise), make_shared<SimpleTrace>(3, 0, noise)};
    {
        auto sbr = tbin_range(noise_traces);
        std::cerr << "noise tbin range=[" << sbr.first << "," << sbr.second << "]\n";
        Assert(sbr.first == 0 and sbr.second == nsamples);
    }
    auto f2 = make_shared<SimpleFrame>(noise_frame_ident, noise_start_time, noise_traces, tick);
    f2->tag_frame("noise");
    f2->tag_traces("oddch", {0,2});
    f2->tag_traces("evench", {1,3});

    const int summed_ident = 1;
    auto f3 = Aux::sum({f1, f2}, summed_ident);
    Assert(f3);
    Assert(f3->ident() == summed_ident);
    Assert(f3->time() == noise_start_time);
    Assert(f3->tick() == tick);
    auto traces = f3->traces();
    Assert(traces->size() == 4);
    // cerr << "Frame #" << f3->ident() << " time=" << f3->time() / units::ms << "ms\n";
    for (auto trace : *traces) {
        const auto& charge = trace->charge();
        // cerr << "ch=" << trace->channel() << " tbin=" << trace->tbin() << " nsamples=" << charge.size() << endl;
        for (size_t ind = 0; ind < charge.size(); ++ind) {
            const float q = charge[ind];
            if (q < 0) {
                continue;
            }
            // cerr << "\tq=" << q << " tbin=" << ind << " t=" << (ind * tick) / units::ms << "ms\n";
        }
        // cerr << endl;
    }

    test_tensors(f1, f2);
    return 0;
}
