
#include "WireCellImg/InSliceDeghosting.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/GeomClusteringUtil.h"
#include "WireCellAux/ClusterShadow.h"
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
    return cfg;
}

void Img::InSliceDeghosting::configure(const WireCell::Configuration& cfg)
{
    m_dryrun = get<bool>(cfg, "dryrun", m_dryrun);
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
    bool exist(const Map& m, const Key& k, const Val& t) {
        auto er = m.equal_range(k);
        for (auto it = er.first; it != er.second; ++it) {
            if (it->second == t) {
                return true;
            }
        }
        return false;
    }

    // struct pair_hash
    // {
    //     template <class T1, class T2>
    //     std::size_t operator () (std::pair<T1, T2> const &pair) const
    //     {
    //         std::size_t h1 = std::hash<T1>()(pair.first);
    //         std::size_t h2 = std::hash<T2>()(pair.second);
    //         return h1 ^ h2;
    //     }
    // };

}  // namespace

bool Img::InSliceDeghosting::operator()(const input_pointer& in, output_pointer& out)
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

    /// 1, Ident good/bad blob groups. in: ICluster out: blob_quality_tags {blob_desc -> quality_tag} TODO: map or
    /// multi-map?
    /// FIXME: place holder. Using simple thresholing.
    for (const auto& vtx : GraphTools::mir(boost::vertices(in_graph))) {
        const auto& node = in_graph[vtx];
        if (node.code() != 'b') {
            continue;
        }
        const auto iblob = get<cluster_node_t::blob_t>(node.ptr);
        if (iblob->value() > m_good_blob_charge_th) {
            blob_tags.insert({vtx, GOOD});
        }
        else {
            blob_tags.insert({vtx, BAD});
        }
    }

    /// 2, Local (in-slice) de-ghosting. in: ICluster + blob_quality_tags out: updated blob_quality_tags (remove or not)
    /// FIXME: place holder. Only keep the largest blob in slice
    int count_keeper = 0;
    for (const auto& svtx : GraphTools::mir(boost::vertices(in_graph))) {
        if (in_graph[svtx].code() != 's') {
            continue;
        }
        // Loop over the neighbor blobs of current slice. Keep the largest one.
        cluster_vertex_t max_blob = -1;
        double max_charge = -1e12;
        for (auto bedge : GraphTools::mir(boost::out_edges(svtx, in_graph))) {
            auto bvtx = boost::target(bedge, in_graph);
            if (in_graph[bvtx].code() != 'b') {
                THROW(ValueError() << errmsg{
                          String::format("'s' node connnected to '%s' not implemented!", in_graph[bvtx].code())});
            }
            // temp tag all TO_BE_REMOVED
            blob_tags.insert({bvtx, TO_BE_REMOVED});
            const auto iblob = get<cluster_node_t::blob_t>(in_graph[bvtx].ptr);
            auto charge = iblob->value();
            if (charge > max_charge) {
                max_charge = charge;
                max_blob = bvtx;
            }
        }
        // rm the keeper from TO_BE_REMOVED
        // log->debug("rm {} from TO_BE_REMOVED", max_blob);
        // erase_if not available until c++20
        // const auto count = std::erase_if(blob_tags, [](const auto& item) {
        //     auto const& [key, value] = item;
        //     return key == max_blob && value == TO_BE_REMOVED;
        // });
        for (auto i = blob_tags.begin(), last = blob_tags.end(); i != last;) {
            auto const& [key, value] = *i;
            if (key == max_blob && value == TO_BE_REMOVED) {
                i = blob_tags.erase(i);
                ++count_keeper;
            }
            else {
                ++i;
            }
        }
    }
    log->debug("count_keeper {}", count_keeper);

    /// 3, delete some blobs. in: ICluster + blob_quality_tags out: filtered ICluster
    /// FIXME: need checks.
    using VFiltered =
        typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
    // VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) {
    //     auto er = blob_tags.equal_range(vtx);
    //     for (auto it = er.first; it != er.second; ++it) {
    //         if (it->second == TO_BE_REMOVED) {
    //             return false;
    //         }
    //     }
    //     return true;
    // });
    VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) {
        return !exist(blob_tags, vtx, TO_BE_REMOVED);
    });

    /// 4, in-group clustering. in: ICluster + blob_quality_tags out: filtered ICluster
    /// FIXME: place holder.
    /// TODO: do we need to call copy_graph?
    WireCell::cluster_graph_t cg_old_bb;
    boost::copy_graph(fg_rm_bad_blobs, cg_old_bb);
    log->debug("cg_old_bb:");
    dump_cg(cg_old_bb, log);
    /// rm old b-b edges
    using EFiltered =
        typename boost::filtered_graph<cluster_graph_t, std::function<bool(cluster_edge_t)>, boost::keep_all >;
    EFiltered fg_no_bb(cg_old_bb, [&](auto edge) {
        auto source = boost::source(edge, cg_old_bb);
        auto target = boost::target(edge, cg_old_bb);
        if (cg_old_bb[source].code()=='b' and cg_old_bb[target].code()=='b') {
            return false;
        }
        return true;
    }, {});
    WireCell::cluster_graph_t cg_new_bb;
    boost::copy_graph(fg_no_bb, cg_new_bb);
    log->debug("cg_new_bb:");
    dump_cg(cg_new_bb, log);
    /// make new b-b edges within groups
    std::vector<gc_filter_t> filters = {
        [&](const cluster_vertex_t& vtx) { return exist(blob_tags, vtx, GOOD) && !exist(blob_tags, vtx, BAD); },
        [&](const cluster_vertex_t& vtx) { return !exist(blob_tags, vtx, GOOD) && exist(blob_tags, vtx, BAD); }
    };
    /// each filter adds new b-b edges on cg_new_bb
    for (auto filter : filters) {
        geom_clustering(cg_new_bb, m_clustering_policy, filter);
    }
    log->debug("cg_new_bb:");
    dump_cg(cg_new_bb, log);

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
