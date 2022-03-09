#include "WireCellRoot/CelltreeSource.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TH1F.h"

WIRECELL_FACTORY(CelltreeSource, WireCell::Root::CelltreeSource, WireCell::IFrameSource, WireCell::IConfigurable)

using namespace WireCell;

Root::CelltreeSource::CelltreeSource()
  : m_calls(0)
{
}

Root::CelltreeSource::~CelltreeSource() {}

void Root::CelltreeSource::configure(const WireCell::Configuration& cfg)
{
    if (cfg["filename"].empty()) {
        THROW(ValueError() << errmsg{"CelltreeSource: must supply input \"filename\" configuration item."});
    }
    m_cfg = cfg;
}

WireCell::Configuration Root::CelltreeSource::default_configuration() const
{
    Configuration cfg;
    // Give a URL for the input file.
    cfg["filename"] = "";

    // which event in the celltree file to be processed
    cfg["EventNo"] = "0";

    // Give a list of frame/tree tags.

    // just raw waveform and no other choice at present
    // Tree: Sim, Wf: raw_wf
    cfg["frames"][0] = "orig";

    cfg["in_branch"] = "raw";

    cfg["time_scale"] = 4;

    return cfg;
}

bool Root::CelltreeSource::branch2traces(
    ITrace::vector& all_traces, std::unordered_map<IFrame::tag_t, IFrame::trace_list_t>& tagged_traces, TTree* tree,
    const std::string& br_name, const std::string& frametag, const unsigned int entry, const int time_scale) const
{
    std::vector<int>* channelid = new std::vector<int>;
    TClonesArray* esignal = new TClonesArray;

    auto br_name_channelId = String::format("%s_channelId",br_name);
    auto br_name_wf = String::format("%s_wf",br_name);

    tree->SetBranchStatus(br_name_channelId.c_str(), 1);
    tree->SetBranchAddress(br_name_channelId.c_str(), &channelid);
    tree->SetBranchStatus(br_name_wf.c_str(), 1);
    tree->SetBranchAddress(br_name_wf.c_str(), &esignal);

    int size = tree->GetEntry(entry);
    if (!(size>0)) {
        THROW(RuntimeError() << errmsg{"Failed loading entry!"});
    }

    int nchannels = channelid->size();

    int channel_number = 0;

    std::cerr << "CelltreeSource: loading " << frametag << " " << nchannels << " channels \n";

    // fill waveform ...
    TH1S* signal_s;
    TH1F* signal_f;

    for (int ind = 0; ind < nchannels; ++ind) {
        signal_s = dynamic_cast<TH1S*>(esignal->At(ind));
        signal_f = dynamic_cast<TH1F*>(esignal->At(ind));

        bool flag_float = 1;
        if (signal_f == 0 && signal_s != 0) flag_float = 0;

        if (!signal_f && !signal_s) continue;
        channel_number = channelid->at(ind);

        ITrace::ChargeSequence charges;
        int nticks;
        if (flag_float == 1)
            nticks = signal_f->GetNbinsX();
        else
            nticks = signal_s->GetNbinsX();
        // if (ind==0) std::cerr<<"CelltreeSource: tick "<<nticks<<"\n";
        // nticks = 9600,  this could be an issue cause just 9594 have non-ZERO value around baseline
        for (int itickbin = 0; itickbin != nticks; itickbin++) {
            if (flag_float == 1) {
                // if (signal_f->GetBinContent(itickbin + 1) != 0) {
                //     charges.push_back(signal_f->GetBinContent(itickbin + 1));
                // }
                for (int i=0; i<time_scale; ++i) {
                    charges.push_back(signal_f->GetBinContent(itickbin + 1));
                }
            }
            else {
                // if (signal_s->GetBinContent(itickbin + 1) != 0) {
                //     charges.push_back(signal_s->GetBinContent(itickbin + 1));
                // }
                for (int i=0; i<time_scale; ++i) {
                    charges.push_back(signal_s->GetBinContent(itickbin + 1));
                }
            }
        }

        const size_t index = all_traces.size();
        tagged_traces[frametag].push_back(index);
        // std::cerr<<"CelltreeSource: charges.size() "<<charges.size()<<"\n";
        all_traces.push_back(std::make_shared<SimpleTrace>(channel_number, 0, charges));
    }
    return true;
}

bool Root::CelltreeSource::operator()(IFrame::pointer& out)
{
    out = nullptr;
    ++m_calls;
    std::cerr << "CelltreeSource: called " << m_calls << " times\n";
    if (m_calls > 2) {
        std::cerr << "CelltreeSource: past EOS\n";
        return false;
    }
    if (m_calls > 1) {
        std::cerr << "CelltreeSource: EOS\n";
        return true;  // this is to send out EOS
    }

    std::string url = m_cfg["filename"].asString();

    TFile* tfile = TFile::Open(url.c_str());

    TTree* tree = (TTree*) tfile->Get("/Event/Sim");
    if (!tree) {
        THROW(IOError() << errmsg{"No tree: /Event/Sim in input file"});
    }

    tree->SetBranchStatus("*", 0);

    int run_no, subrun_no, event_no;
    tree->SetBranchStatus("eventNo", 1);
    tree->SetBranchAddress("eventNo", &event_no);
    tree->SetBranchStatus("runNo", 1);
    tree->SetBranchAddress("runNo", &run_no);
    tree->SetBranchStatus("subRunNo", 1);
    tree->SetBranchAddress("subRunNo", &subrun_no);

    int frame_number = std::stoi(m_cfg["EventNo"].asString());

    // loop entry
    int siz = 0;
    unsigned int entries = tree->GetEntries();
    unsigned int ent = 0;
    for (; ent < entries; ent++) {
        siz = tree->GetEntry(ent);
        if (event_no == frame_number) {
            break;
        }
    }

    // need run number and subrunnumber
    int frame_ident = event_no;
    double frame_time = 0;
    int time_scale = m_cfg["time_scale"].asInt();

    // some output using eventNo, runNo, subRunNO, ...
    std::cerr << "CelltreeSource: frame_number " << frame_number << ", frame_ident " << frame_ident << ", siz " << siz << "\n";
    std::cerr << "CelltreeSource: runNo " << run_no << ", subrunNo " << subrun_no << ", eventNo " << event_no << "\n";
    std::cerr << "CelltreeSource: time_scale " << time_scale << "\n";

    ITrace::vector all_traces;
    std::unordered_map<IFrame::tag_t, IFrame::trace_list_t> tagged_traces;
    // celltree input now is raw data, no information about any noisy or bad channels
    // leave cmm empty.
    WireCell::Waveform::ChannelMaskMap cmm;
    branch2traces(all_traces, tagged_traces, tree, m_cfg["in_branch"].asString(), m_cfg["frames"][0].asString(), ent, time_scale);

    auto sframe = new SimpleFrame(frame_ident, frame_time, all_traces, 0.5 * units::microsecond, cmm);
    for (auto const& it : tagged_traces) {
        sframe->tag_traces(it.first, it.second);
        std::cerr << "CelltreeSource: tag " << it.second.size() << " traces as: \"" << it.first << "\"\n";
    }

    out = IFrame::pointer(sframe);

    return true;
}
