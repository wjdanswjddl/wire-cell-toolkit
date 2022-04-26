#include "WireCellRoot/CelltreeSource.h"
#include "WireCellAux/SimpleTrace.h"
#include "WireCellAux/SimpleFrame.h"

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

    // input branch base name
    // raw, calibGaussian, calibWiener
    cfg["in_branch_base_names"][0] = "raw";

    // output frame tags, should be corresponding to each input
    // branch name, raw: orig, calibGaussian: gauss, calibWiener, wiener
    cfg["out_trace_tags"][0] = "orig";

    cfg["time_scale"] = 4;

    return cfg;
}

bool Root::CelltreeSource::read_traces(
    ITrace::vector& all_traces, std::unordered_map<IFrame::tag_t, IFrame::trace_list_t>& tagged_traces, std::string& fname,
    const std::string& br_name, const std::string& frametag, const unsigned int entry, const int time_scale) const
{
    TFile* tfile = TFile::Open(fname.c_str());
    TTree* tree = (TTree*) tfile->Get("/Event/Sim");
    if (!tree) {
        THROW(IOError() << errmsg{"No tree: /Event/Sim in input file"});
    }

    std::vector<int>* channelid = new std::vector<int>;
    TClonesArray* esignal = new TClonesArray;

    auto br_name_channelId = String::format("%s_channelId",br_name);
    auto br_name_wf = String::format("%s_wf",br_name);

    // tree->SetBranchStatus(br_name_channelId.c_str(), 1);
    tree->SetBranchAddress(br_name_channelId.c_str(), &channelid);
    // tree->SetBranchStatus(br_name_wf.c_str(), 1);
    tree->SetBranchAddress(br_name_wf.c_str(), &esignal);

    int size = tree->GetEntry(entry);
    if (!(size>0)) {
        THROW(RuntimeError() << errmsg{"Failed loading entry!"});
    }

    int nchannels = channelid->size();

    int channel_number = 0;
    std::cout << "CelltreeSource: loading " << frametag << " " << nchannels << " channels \n";
    std::cout << "CelltreeSource: esignal->GetSize(): " << esignal->GetSize() << "\n";

    // fill waveform ...
    TH1* signal;

    for (int ind = 0; ind < nchannels; ++ind) {
        signal = dynamic_cast<TH1*>(esignal->At(ind));
        if (!signal) continue;
        channel_number = channelid->at(ind);

        if(ind==0) {
            std::cout << "yuhw: " << br_name << " channel_number: " << channel_number << " signal->Integral(): " << signal->Integral() << std::endl;
        }

        ITrace::ChargeSequence charges;
        int nticks = signal->GetNbinsX();
        for (int itickbin = 0; itickbin != nticks; itickbin++) {
            for (int i=0; i<time_scale; ++i) {
                charges.push_back(signal->GetBinContent(itickbin + 1));
            }
        }

        const size_t index = all_traces.size();
        tagged_traces[frametag].push_back(index);
        // std::cout<<"CelltreeSource: charges.size() "<<charges.size()<<"\n";
        all_traces.push_back(std::make_shared<Aux::SimpleTrace>(channel_number, 0, charges));
    }

    tfile->Close();
    return true;
}

bool Root::CelltreeSource::read_cmm(WireCell::Waveform::ChannelMaskMap &cmm, std::string& fname, const unsigned int entry) const
{
    TFile* tfile = TFile::Open(fname.c_str());
    TTree* tree = (TTree*) tfile->Get("/Event/Sim");
    if (!tree) {
        THROW(IOError() << errmsg{"No tree: /Event/Sim in input file"});
    }

    std::vector<int>* badChannel = new std::vector<int>;
    std::vector<int>* badBegin = new std::vector<int>;
    std::vector<int>* badEnd = new std::vector<int>;

    // tree->SetBranchStatus("badChannel", 1);
    // tree->SetBranchStatus("badBegin", 1);
    // tree->SetBranchStatus("badEnd", 1);
    tree->SetBranchAddress("badChannel", &badChannel);
    tree->SetBranchAddress("badBegin", &badBegin);
    tree->SetBranchAddress("badEnd", &badEnd);

    int size = tree->GetEntry(entry);
    if (!(size>0)) {
        THROW(RuntimeError() << errmsg{"Failed loading entry!"});
    }

    int nchannels = badChannel->size();
    if (nchannels != (int)badBegin->size() || nchannels != (int)badEnd->size()) {
        THROW(RuntimeError() << errmsg{"Length doesn't match"});
    }

    for (int ind = 0; ind < nchannels; ++ind) {
        auto chid = badChannel->at(ind);
        WireCell::Waveform::BinRange br;
        br.first = badBegin->at(ind);
        br.second = badEnd->at(ind);
        cmm["bad"][chid].push_back(br);
    }

    tfile->Close();
    return true;
}

bool Root::CelltreeSource::operator()(IFrame::pointer& out)
{
    out = nullptr;
    ++m_calls;
    std::cout << "CelltreeSource: called " << m_calls << " times\n";
    if (m_calls > 2) {
        std::cout << "CelltreeSource: past EOS\n";
        return false;
    }
    if (m_calls > 1) {
        std::cout << "CelltreeSource: EOS\n";
        return true;  // this is to send out EOS
    }

    std::string url = m_cfg["filename"].asString();
    TFile* tfile = TFile::Open(url.c_str());
    TTree* tree = (TTree*) tfile->Get("/Event/Sim");
    if (!tree) {
        THROW(IOError() << errmsg{"No tree: /Event/Sim in input file"});
    }

    // tree->SetBranchStatus("*", 0);

    int run_no, subrun_no, event_no;
    // tree->SetBranchStatus("eventNo", 1);
    tree->SetBranchAddress("eventNo", &event_no);
    // tree->SetBranchStatus("runNo", 1);
    tree->SetBranchAddress("runNo", &run_no);
    // tree->SetBranchStatus("subRunNo", 1);
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
    std::cout << "CelltreeSource: frame_number " << frame_number << ", frame_ident " << frame_ident << ", siz " << siz << "\n";
    std::cout << "CelltreeSource: runNo " << run_no << ", subrunNo " << subrun_no << ", eventNo " << event_no << "\n";
    std::cout << "CelltreeSource: time_scale " << time_scale << "\n";
    std::cout << "CelltreeSource: ent " << ent << "\n";
    tfile->Close();

    ITrace::vector all_traces;
    std::unordered_map<IFrame::tag_t, IFrame::trace_list_t> tagged_traces;

    if (m_cfg["in_branch_base_names"].size() != m_cfg["out_trace_tags"].size()) {
        THROW(ValueError() << errmsg{"#in_branch_base_names != #out_trace_tags"});
    }

    for (Json::ArrayIndex i = 0; i < m_cfg["in_branch_base_names"].size(); ++i) {
        auto branch_base_name = m_cfg["in_branch_base_names"][i].asString();
        auto frame_tag = m_cfg["out_trace_tags"][i].asString();
        read_traces(all_traces, tagged_traces, url, branch_base_name, frame_tag, ent, time_scale);
    }
    
    WireCell::Waveform::ChannelMaskMap cmm;
    read_cmm(cmm, url, ent);
    std::cout << "CelltreeSource: cmm[\"bad\"].size() " << cmm["bad"].size() << "\n";

    auto sframe = new Aux::SimpleFrame(frame_ident, frame_time, all_traces, 0.5 * units::microsecond, cmm);
    for (auto const& it : tagged_traces) {
        sframe->tag_traces(it.first, it.second);
        std::cout << "CelltreeSource: tag " << it.second.size() << " traces as: \"" << it.first << "\"\n";
    }

    out = IFrame::pointer(sframe);

    return true;
}
