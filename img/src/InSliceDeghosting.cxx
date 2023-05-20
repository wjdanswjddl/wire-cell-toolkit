
#include "WireCellImg/InSliceDeghosting.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/GeomClusteringUtil.h"
#include "WireCellAux/SimpleCluster.h"
#include "WireCellUtil/GraphTools.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Exceptions.h"

#include <iterator>
#include <chrono>
#include <fstream>

WIRECELL_FACTORY(InSliceDeghosting, WireCell::Img::InSliceDeghosting, WireCell::INamed, WireCell::IClusterFilter,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Aux;

Img::InSliceDeghosting::InSliceDeghosting()
  : Aux::Logger("InSliceDeghosting", "img")
{
}

Img::InSliceDeghosting::~InSliceDeghosting() {}

WireCell::Configuration Img::InSliceDeghosting::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["dryrun"] = m_dryrun;
    cfg["good_blob_charge_th"] = m_good_blob_charge_th;
    cfg["clustering_policy"] = m_clustering_policy;
    cfg["config_round"] = m_config_round;
    return cfg;
}

void Img::InSliceDeghosting::configure(const WireCell::Configuration& cfg)
{
    m_dryrun = get<bool>(cfg, "dryrun", m_dryrun);
    m_good_blob_charge_th = get<double>(cfg, "good_blob_charge_th", m_good_blob_charge_th);
    m_clustering_policy = get<std::string>(cfg, "clustering_policy", m_clustering_policy);
    m_config_round = get<int>(cfg, "config_round", m_config_round);
    
    Json::FastWriter jwriter;
    log->debug("{}", jwriter.write(cfg));
}

namespace {
    void dump_cg(const cluster_graph_t& cg, Log::logptr_t& log)
    {
        size_t mcount{0}, bcount{0};
        CS::value_t bval;
        for (const auto& vtx : GraphTools::mir(boost::vertices(cg))) {
            const auto& node = cg[vtx];
            if (node.code() == 'b') {
                const auto iblob = get<cluster_node_t::blob_t>(node.ptr);
                bval += CS::value_t(iblob->value(), iblob->uncertainty());
                ++bcount;
                continue;
            }
            if (node.code() == 'm') {
                const auto imeas = get<cluster_node_t::meas_t>(node.ptr);
                ++mcount;
            }
        }
        log->debug("cluster graph: vertices={} edges={} #blob={} bval={} #meas={}", boost::num_vertices(cg),
                   boost::num_edges(cg), bcount, bval, mcount);
    }

    template <class Map, class Key, class Val>
    bool exist(const Map& m, const Key& k, const Val& t)
    {
        auto er = m.equal_range(k);
        for (auto it = er.first; it != er.second; ++it) {
            if (it->second == t) {
                return true;
            }
        }
        return false;
    }

    /// b -> connected channel idents
    std::unordered_set<int> connected_channels(const cluster_graph_t& cg, const cluster_vertex_t& bdesc)
    {
        std::unordered_set<int> cidents;
        const auto wdescs = neighbors_oftype<cluster_node_t::wire_t>(cg, bdesc);
        for (auto wdesc : wdescs) {
            const auto cdescs = neighbors_oftype<cluster_node_t::channel_t>(cg, wdesc);
            /// 'w' can only be connect to 1 'c'
            if (cdescs.size() != 1) {
                THROW(ValueError() << errmsg{String::format("'w' connnected to %d 'c's!", cdescs.size())});
            }
            const auto ichannel = get<cluster_node_t::channel_t>(cg[cdescs[0]].ptr);
            cidents.insert(ichannel->ident());
        }
        return cidents;
    }
}  // namespace

void InSliceDeghosting::blob_quality_ident(const cluster_graph_t& cg, vertex_tags_t& blob_tags)
{
  // loop over blob 
    for (const auto& vtx : GraphTools::mir(boost::vertices(cg))) {
        const auto& node = cg[vtx];
        if (node.code() != 'b') {
            continue;
        }
        const auto iblob = get<cluster_node_t::blob_t>(node.ptr);
	//        double current_t = iblob->slice()->start();
        double current_q = iblob->value(); // charge ...
        if (current_q > m_good_blob_charge_th) {
	  blob_tags.insert({vtx, GOOD});
	  blob_tags.insert({vtx, POTENTIAL_GOOD});
        }
	//        else {
        //    blob_tags.insert({vtx, BAD});
        //}

	//loop over the b-b connections ...
        /// XIN: add between slice b-b connection info.
        /// follow b-b edges -> time, charge of connected blobs
        //std::vector<double> fronts, backs;
        for (auto v : neighbors_oftype<cluster_node_t::blob_t>(cg, vtx)) {
            const auto ib = get<cluster_node_t::blob_t>(cg[v].ptr);
	    if (ib->value() > m_good_blob_charge_th){
	      blob_tags.insert({vtx, POTENTIAL_GOOD});
	    }
	    //  if (ib->slice()->start() > current_t) {
            //    backs.push_back(ib->value());
	    // }
            //if (ib->slice()->start() < current_t) {
            //    fronts.push_back(ib->value());
            //}
        }
	
        /// DEBUGONLY:
        //log->debug("{} fronts {} backs {}", vtx, fronts.size(), backs.size());
    }
}
void InSliceDeghosting::local_deghosting1(const cluster_graph_t& cg, vertex_tags_t& blob_tags)
{
  
}

void InSliceDeghosting::local_deghosting(const cluster_graph_t& cg, vertex_tags_t& blob_tags)
{
    int count_keeper = 0;
    for (const auto& svtx : GraphTools::mir(boost::vertices(cg))) {
        if (cg[svtx].code() != 's') {
            continue;
        }
        // Loop over the neighbor blobs of current slice. Keep the largest one.
        std::unordered_map<size_t, std::unordered_set<cluster_vertex_t> > view_groups;
        std::unordered_map<RayGrid::grid_index_t, size_t> grid_occupancy;
        for (auto bedge : GraphTools::mir(boost::out_edges(svtx, cg))) {
            /// XIN: bvtx -> RayGrid::Strip::grid_range_t, {2views}, {3views}
            /// CHECK: per slice map {RayGrid::grid_index_t -> float}
            auto bvtx = boost::target(bedge, cg);
            if (cg[bvtx].code() != 'b') {
                THROW(ValueError() << errmsg{
                          String::format("'s' node connnected to '%s' not implemented!", cg[bvtx].code())});
            }

            /// group blobs into #live view group
            /// ASSUMPTION: connected meas node == # live views
            auto meas = neighbors_oftype<cluster_node_t::meas_t>(cg, bvtx);
            view_groups[meas.size()].insert(bvtx);
	    // how do we know which plane meas_t is ???

            /// EXAMPLE: add up #blobs using each RayGrid::grid_index_t
	    // RayGrid ... layer_index_t ...
            const IBlob::pointer iblob = get<cluster_node_t::blob_t>(cg[bvtx].ptr);
            const auto& strips = iblob->shape().strips();
            for (const RayGrid::Strip& strip : strips) {
                for (RayGrid::grid_index_t gridx = strip.bounds.first; gridx < strip.bounds.second; ++gridx) {
                    if (grid_occupancy.find(gridx) != grid_occupancy.end()) {
                        grid_occupancy[gridx]++;
                    }
                    else {
                        grid_occupancy[gridx] = 1;
                    }
                }
            }

            /// EXAMPLE: b -> connected channel idents
	    // how do we know which plane is channel is?
            auto cidents = connected_channels(cg, bvtx);
            log->debug("cidents {}", cidents.size());

            /// EXAMPLEONLY: temp tag all as TO_BE_REMOVED
            blob_tags.insert({bvtx, TO_BE_REMOVED});
        }
	
        /// EXAMPLEONLY: rm 3views from TO_BE_REMOVED
        /// this is to prune all 3views from blob_tags, for single key deletion, use equal_range.
        for (auto i = blob_tags.begin(), last = blob_tags.end(); i != last;) {
            auto const& [key, value] = *i;
            if (view_groups[3].find(key) != view_groups[3].end() && value == TO_BE_REMOVED) {
                i = blob_tags.erase(i);
                ++count_keeper;
            }
            else {
                ++i;
            }
        }
    }
    log->debug("count_keeper {}", count_keeper);
}
bool InSliceDeghosting::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);

    // blob desc -> quality tag
    std::unordered_multimap<cluster_vertex_t, BLOB_QUALITY> blob_tags;

    WireCell::cluster_graph_t cg_new_bb;
    
    if (m_config_round==1){
      /// 1, Ident good/bad blob groups. in: ICluster out: blob_quality_tags {blob_desc -> quality_tag} TODO: map or
      /// multi-map?
      /// FIXME: place holder. Using simple thresholing.
      blob_quality_ident(in_graph, blob_tags);

      /// 2, Local (in-slice) de-ghosting. in: ICluster + blob_quality_tags out: updated blob_quality_tags (remove or not)
      // /// FIXME: place holder. Only keep the largest blob in slice
      local_deghosting(in_graph, blob_tags);
      
      /// 3, delete some blobs. in: ICluster + blob_quality_tags out: filtered ICluster
      /// FIXME: need checks.
      using VFiltered =
        typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
      VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) { return !exist(blob_tags, vtx, TO_BE_REMOVED); });

      
    

      
    /// 4, in-group clustering. in: ICluster + blob_quality_tags out: filtered ICluster
    /// FIXME: place holder.
    /// TODO: do we need to call copy_graph?
      WireCell::cluster_graph_t cg_old_bb;
      boost::copy_graph(fg_rm_bad_blobs, cg_old_bb);
      log->debug("cg_old_bb:");
      dump_cg(cg_old_bb, log);
      /// rm old b-b edges
      using EFiltered =
        typename boost::filtered_graph<cluster_graph_t, std::function<bool(cluster_edge_t)>, boost::keep_all>;
      EFiltered fg_no_bb(cg_old_bb,
			 [&](auto edge) {
                           auto source = boost::source(edge, cg_old_bb);
                           auto target = boost::target(edge, cg_old_bb);
                           if (cg_old_bb[source].code() == 'b' and cg_old_bb[target].code() == 'b') {
			     return false;
                           }
                           return true;
			 },
			 {});

      boost::copy_graph(fg_no_bb, cg_new_bb);
      log->debug("cg_new_bb:");
      dump_cg(cg_new_bb, log);
      /// make new b-b edges within groups
      /// four separated groups, good&bad, good&good, bad&bad
      std::vector<gc_filter_t> filters = {
        [&](const cluster_vertex_t& vtx) { return exist(blob_tags, vtx, POTENTIAL_GOOD) && !exist(blob_tags, vtx, POTENTIAL_BAD); },
        [&](const cluster_vertex_t& vtx) { return exist(blob_tags, vtx, POTENTIAL_GOOD) && exist(blob_tags, vtx, POTENTIAL_BAD); },
	[&](const cluster_vertex_t& vtx) { return !exist(blob_tags, vtx, POTENTIAL_GOOD) && !exist(blob_tags, vtx, POTENTIAL_BAD); },
        [&](const cluster_vertex_t& vtx) { return !exist(blob_tags, vtx, POTENTIAL_GOOD) && exist(blob_tags, vtx, POTENTIAL_BAD); },
      };
      /// each filter adds new b-b edges on cg_new_bb
      for (auto filter : filters) {
        geom_clustering(cg_new_bb, m_clustering_policy, filter);
      }
      log->debug("cg_new_bb:");
      dump_cg(cg_new_bb, log);

     
      

    }else if (m_config_round==2){
      // after charge solving ...
      blob_quality_ident(in_graph, blob_tags);
      
      // second round of local deghosting ...
      local_deghosting1(in_graph, blob_tags);

      using VFiltered =
        typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
      VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) { return !exist(blob_tags, vtx, TO_BE_REMOVED); });
      // do we have to copy this every time ???
      boost::copy_graph(fg_rm_bad_blobs, cg_new_bb);
      
       
      
    }else if (m_config_round==3){
      
      // after charge solving ...
      blob_quality_ident(in_graph, blob_tags);

      // at some points remove the Non-potential good ... (last round ...)
      using VFiltered =
	typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
      VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) { return exist(blob_tags, vtx, POTENTIAL_GOOD); });
      // do we have to copy this every time ???
      boost::copy_graph(fg_rm_bad_blobs, cg_new_bb);
    }

    /// output
    auto& out_graph = cg_new_bb;
    // debug info
    log->debug("in_graph:");
    dump_cg(in_graph, log);
    log->debug("out_graph:");
    dump_cg(out_graph, log);
    
    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    if (m_dryrun) {
      out = std::make_shared<Aux::SimpleCluster>(in_graph, in->ident());
    }
    return true;
    
}
