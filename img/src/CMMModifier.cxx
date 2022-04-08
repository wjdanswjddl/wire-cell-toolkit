#include "WireCellImg/CMMModifier.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/IWaveformMap.h"
#include "WireCellAux/FrameTools.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(CMMModifier, WireCell::Img::CMMModifier, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;

CMMModifier::CMMModifier()
    : Aux::Logger("CMMModifier", "img")
{
}

CMMModifier::~CMMModifier() {}

WireCell::Configuration CMMModifier::default_configuration() const
{
    Configuration cfg;

    //
    cfg["anode"] = "AnodePlane";
    //
    cfg["cm_tag"] = m_cm_tag;
    //
    cfg["trace_tag"] = m_trace_tag;

    cfg["m_start"] = m_start;
    cfg["m_end"] = m_end;
    
    // bad channels ...
    cfg["ncount_cont_ch"] = m_ncount_cont_ch;
    for(int i=0;i!=m_ncount_cont_ch;i++){
      cfg["cont_ch_llimit"][i] = m_cont_ch_llimit[i];
      cfg["cont_ch_hlimit"][i] = m_cont_ch_hlimit[i];
    }

    cfg["ncount_veto_ch"] = m_ncount_veto_ch;
    for(int i=0;i!=m_ncount_veto_ch;i++){
      cfg["veto_ch_llimit"][i] = m_veto_ch_llimit[i];
      cfg["veto_ch_hlimit"][i] = m_veto_ch_hlimit[i];
    }

    cfg["dead_ch_ncount"] = m_dead_ch_ncount;
    cfg["dead_ch_charge"] = m_dead_ch_charge;
    cfg["ncount_dead_ch"] = m_ncount_dead_ch;
    for(int i=0;i!=m_ncount_dead_ch;i++){
      cfg["dead_ch_llimit"][i] = m_dead_ch_llimit[i];
      cfg["dead_ch_hlimit"][i] = m_dead_ch_hlimit[i];
    }
    
    return cfg;
}

void CMMModifier::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    m_cm_tag = get<std::string>(cfg, "cm_tag", m_cm_tag);
    m_trace_tag = get<std::string>(cfg, "trace_tag", m_trace_tag);


    m_start = get<int>(cfg,"start",m_start);
    m_end = get<int>(cfg,"end",m_end);
    
    m_ncount_cont_ch = get<int>(cfg,"ncount_cont_ch",m_ncount_cont_ch);
    if (cfg.isMember("cont_ch_llimit")){
      m_cont_ch_llimit.clear();
      for (auto value: cfg["cont_ch_llimit"]){
	m_cont_ch_llimit.push_back(value.asInt());
      }
    }
    if (cfg.isMember("cont_ch_hlimit")){
      m_cont_ch_hlimit.clear();
      for (auto value: cfg["cont_ch_hlimit"]){
	m_cont_ch_hlimit.push_back(value.asInt());
      }
    }

    m_ncount_veto_ch = get<int>(cfg,"ncount_veto_ch",m_ncount_veto_ch);
    if (cfg.isMember("veto_ch_llimit")){
      m_veto_ch_llimit.clear();
      for (auto value: cfg["veto_ch_llimit"]){
	m_veto_ch_llimit.push_back(value.asInt());
      }
    }
    if (cfg.isMember("veto_ch_hlimit")){
      m_veto_ch_hlimit.clear();
      for (auto value: cfg["veto_ch_hlimit"]){
	m_veto_ch_hlimit.push_back(value.asInt());
      }
    }

    m_dead_ch_ncount = get<int>(cfg,"dead_ch_ncount",m_dead_ch_ncount);
    m_dead_ch_charge = get<float>(cfg,"dead_ch_charge",m_dead_ch_charge);
    m_ncount_dead_ch = get<int>(cfg,"ncount_dead_ch",m_ncount_dead_ch);
    if (cfg.isMember("dead_ch_llimit")){
      m_dead_ch_llimit.clear();
      for (auto value: cfg["dead_ch_llimit"]){
	m_dead_ch_llimit.push_back(value.asInt());
      }
    }
    if (cfg.isMember("dead_ch_hlimit")){
      m_dead_ch_hlimit.clear();
      for (auto value: cfg["dead_ch_hlimit"]){
	m_dead_ch_hlimit.push_back(value.asInt());
      }
    }

    // log->debug("Xin: {} , {} ", m_dead_ch_ncount, m_dead_ch_charge);
}

bool CMMModifier::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS");
        return true;  // eos
    }

    // copy a CMM from input frame
    auto cmm = in->masks();
    if (cmm.find(m_cm_tag)==cmm.end()) {
        THROW(RuntimeError()<< errmsg{"no ChannelMask with name "+m_cm_tag});
    }
    auto& cm = cmm[m_cm_tag];
    log->debug("input: {} size: {}", m_cm_tag, cm.size());

    // begin for Xin
    // some dummy ops for now
    // cm.clear();
    //    int nq=0;
    // int tbin=0;
    //for (auto trace : Aux::tagged_traces(in, m_trace_tag)) {
    //  tbin = trace->tbin();
    //     const int chid = trace->channel();
    //  const auto& charge = trace->charge();
    //  nq = charge.size();
    //  break;
    //     cm[chid].push_back({tbin,tbin+nq});
    // }
    // for (auto it = cm.begin(); it!=cm.end(); it++){
    //   log->debug("{}, {}, {}", it->first, it->second.at(0).first, it->second.at(0).second);
    // }

    // shorted wires ...
    for (int i = 0;i != m_ncount_cont_ch; i++){
      for (int ch = m_cont_ch_llimit[i]; ch != m_cont_ch_hlimit[i]; ch++){
	//	log->debug("{}", ch);
	if (cm.find(ch) == cm.end()){
	  if ((cm.find(ch-1) != cm.end() && cm.find(ch+2) != cm.end())){
	    log->debug("(Shorted) {} added to the bad channel list", ch);
	    cm[ch] = cm[ch-1];
	  } else if ((cm.find(ch-2) != cm.end() && cm.find(ch+1) != cm.end())){
	    log->debug("(Shorted) {} added to the bad channel list", ch);
	    cm[ch] = cm[ch+1];
	  }
	}
      }
    }

    // veto these wires anyway
    for (int i=0;i!=m_ncount_veto_ch;i++){
      for (int ch = m_veto_ch_llimit[i]; ch != m_veto_ch_hlimit[i]; ch++){
	if (cm.find(ch) == cm.end()){
	  cm[ch].push_back({m_start,m_end});
	  log->debug("(Veto) {}, added to the bad channel list with range ({}, {}) ",ch, m_start,m_end);
	}
      }
    } 

    // dead channels ...
    std::vector<int> temp_counts;
    if (m_ncount_dead_ch>0){
      temp_counts.resize(m_ncount_dead_ch,0);
      
      for (auto trace : Aux::tagged_traces(in, m_trace_tag)) {
	//	const int tbin = trace->tbin();
	const int chid = trace->channel();
	const auto& charge = trace->charge();
	const int nq = charge.size();

	int saved_group = -1;
	for (int i=0;i!=m_ncount_dead_ch;i++){
	  if (chid >= m_dead_ch_llimit[i] && chid < m_dead_ch_hlimit[i]){
	    saved_group = i;
	    break;
	  }
	}

	if (saved_group >=0){
	  for (int i=0;i!=nq;i++){
	    if (charge.at(i) > m_dead_ch_charge)
	      temp_counts.at(saved_group) ++;
	  }
	}
      }

      //      log->debug("(Dead) group: {},{}", temp_counts.at(0), temp_counts.at(1));

      for(int i=0;i!=m_ncount_dead_ch;i++){
	if (temp_counts.at(i) < m_dead_ch_ncount){
	  for (int ch = m_dead_ch_llimit[i]; ch != m_dead_ch_hlimit[i]; ch++){
	    if (cm.find(ch) == cm.end()){
	      cm[ch].push_back({m_start, m_end});
	      log->debug("(Dead) {}, added to the bad channel list with range ({}, {}) ",ch, m_start, m_end);
	    }
	  }
	}
      }
    }

    

    
    
    // end for Xin

    // make a copy of all input trace pointers
    ITrace::vector out_traces(*in->traces());

    // Basic frame stays the same.
    auto sfout = new SimpleFrame(in->ident(), in->time(), out_traces, in->tick(), cmm);

    // passing through other parts of the original frame
    for (auto ftag : in->frame_tags()) {
        sfout->tag_frame(ftag);
    }

    for (auto inttag : in->trace_tags()) {
        const auto& traces = in->tagged_traces(inttag);
        const auto& summary = in->trace_summary(inttag);
        sfout->tag_traces(inttag, traces, summary);
    };

    out = IFrame::pointer(sfout);

    log->debug("output: {} size: {}", m_cm_tag, out->masks()[m_cm_tag].size());

    return true;
}
