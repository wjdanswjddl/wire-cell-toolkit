
#include "WireCellImg/ProjectionDeghosting.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/Projection2D.h"
#include "WireCellAux/ClusterShadow.h"
#include "WireCellAux/SimpleCluster.h"
#include "WireCellUtil/GraphTools.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Rectangles.h"

#include <iterator>
#include <chrono>
#include <fstream>

WIRECELL_FACTORY(ProjectionDeghosting, WireCell::Img::ProjectionDeghosting,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Aux;

Img::ProjectionDeghosting::ProjectionDeghosting()
    : Aux::Logger("ProjectionDeghosting", "img")
{
}

Img::ProjectionDeghosting::~ProjectionDeghosting() {}

WireCell::Configuration Img::ProjectionDeghosting::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["verbose"] = m_verbose;

    return cfg;
}

void Img::ProjectionDeghosting::configure(const WireCell::Configuration& cfg)
{
    m_verbose = get<bool>(cfg,"verbose", m_verbose);
    log->debug("m_verbose: {}", m_verbose);
}

static void dump_cg(const cluster_graph_t& cg, Log::logptr_t& log)
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
    log->debug("cluster graph: vertices={} edges={} #blob={} bval={} #meas={}",
               boost::num_vertices(cg), boost::num_edges(cg),
               bcount, bval, mcount);
}

namespace {

    // run in keep mode if removal_mode is false
    cluster_graph_t remove_blobs(const cluster_graph_t& cg, std::unordered_set<cluster_vertex_t> blobs, bool removal_mode = true)
    {
        cluster_graph_t cg_out;
        
        size_t nblobs = 0;
        std::unordered_map<cluster_vertex_t, cluster_vertex_t> old2new;
        for (const auto& vtx : GraphTools::mir(boost::vertices(cg))) {
            const auto& node = cg[vtx];
            if (node.code() == 'b') {
                bool removal = blobs.find(vtx) != blobs.end();
                if (!removal_mode) removal =  !removal;
                if (!removal) {
                    ++nblobs;
                    old2new[vtx] = boost::add_vertex(node, cg_out);
                }
            } else {
                old2new[vtx] = boost::add_vertex(node, cg_out);
            }
        }
        
        std::cout << "nblobs: " << nblobs << std::endl;
        std::cout << "boost::num_vertices(cg_out): " << boost::num_vertices(cg_out) << std::endl;

        for (auto edge : GraphTools::mir(boost::edges(cg))) {
            auto old_tail = boost::source(edge, cg);
            auto old_head = boost::target(edge, cg);

            auto old_tit = old2new.find(old_tail);
            if (old_tit == old2new.end()) {
                continue;
            }
            auto old_hit = old2new.find(old_head);
            if (old_hit == old2new.end()) {
                continue;
            }
            boost::add_edge(old_tit->second, old_hit->second, cg_out);
        }
        std::cout << "boost::num_vertices(cg_out): " << boost::num_vertices(cg_out) << std::endl;
        return cg_out;
    }
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

using cluster_proj_t = std::unordered_map<ClusterShadow::vdesc_t, Projection2D::LayerProjection2DMap>;
static Projection2D::LayerProjection2DMap& get_projection(cluster_proj_t& id2lproj, const ClusterShadow::vdesc_t& id,
                                                          const WireCell::cluster_graph_t& cg,
                                                          const std::set<cluster_vertex_t>& group, const size_t nchan,
                                                          const size_t nslice)
{
    if (id2lproj.find(id) != id2lproj.end()) {
        auto& lproj = id2lproj.at(id);
        return lproj;
    }
    else {
        auto lproj = Projection2D::get_projection(cg, group, nchan, nslice);
        id2lproj[id] = lproj;
        return id2lproj[id];
    }
}

bool Img::ProjectionDeghosting::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);
    auto& out_graph = in_graph;
    BlobShadow::graph_t bsgraph = BlobShadow::shadow(out_graph, 'w'); // or 'c'
    ClusterShadow::blob_cluster_map_t clusters;
    auto cs_graph = ClusterShadow::shadow(out_graph, bsgraph, clusters);
    log->debug("clusters.size(): {} ", clusters.size());

    // make a cluster -> blob map
    std::unordered_map<ClusterShadow::vdesc_t, std::set<cluster_vertex_t> > c2b;
    for(auto p : clusters) {
        c2b[p.second].insert(p.first);
    }

    // Count edges
    std::map<std::string, int> counters = {
        {"cs_edges", 0},
        {"cs_vertices", 0},
        {"nblobs", 0},
        {"nblobs_filter", 0},
        {"nblobs_clusters_tmp", 0},
        {"nblobs_clusters", 0}
    };

    std::ofstream fout("ProjectionDeghosting.log");
    // using pairset_t = std::unordered_set<std::pair<ClusterShadow::vdesc_t, ClusterShadow::vdesc_t>, pair_hash>;
    // using layer2pairset_t = std::unordered_map<WireCell::WirePlaneLayer_t, pairset_t>;
    // layer2pairset_t layer2compared;

    // cluster desc -> LayerProjection2DMap
    cluster_proj_t id2lproj;

    for (const auto& cs_edge : GraphTools::mir(boost::edges(cs_graph))) {
        counters["cs_edges"] += 1;
        WireCell::WirePlaneLayer_t layer = cs_graph[cs_edge].wpid.layer();
        ClusterShadow::vdesc_t tail = boost::source(cs_edge, cs_graph);
        ClusterShadow::vdesc_t head = boost::target(cs_edge, cs_graph);
        // // only compare once
        // auto &compared = layer2compared[layer];
        // // can head-tail switch?
        // if (compared.find({tail, head}) != compared.end() || compared.find({head, tail}) != compared.end()) {
        //     continue;
        // }
        // compared.insert({tail, head});
        // log->debug("layer: {}, compared.size(): {}, tail: {}, head: {}", layer, compared.size(), tail, head);
        auto b_tail = c2b[tail];
        auto b_head = c2b[head];
        // std::stringstream ss;
        // ss << tail << " {";
        // for (auto& b : b_tail) {
        //     ss << b << " ";
        // }
        // ss << "} " << layer << "-> ";
        // ss << head << " {";
        // for (auto& b : b_head) {
        //     ss << b << " ";
        // }
        // ss << "} \n";
        // fout << ss.str();
        // std::cout << ss.str();

        log->debug("layer: {}, tail: {}, head: {}", layer, tail, head);
        Projection2D::LayerProjection2DMap& proj_tail =
            get_projection(id2lproj, tail, in_graph, b_tail, m_nchan, m_nslice);
        Projection2D::LayerProjection2DMap& proj_head =
            get_projection(id2lproj, head, in_graph, b_head, m_nchan, m_nslice);
        log->debug("proj_tail.m_layer_proj.size(): {}", proj_tail.m_layer_proj.size());
        log->debug("proj_head.m_layer_proj.size(): {}", proj_head.m_layer_proj.size());
        Projection2D::Projection2D& ref_proj = proj_tail.m_layer_proj[layer];
        Projection2D::Projection2D& tar_proj = proj_head.m_layer_proj[layer];
        int coverage = Projection2D::judge_coverage(ref_proj, tar_proj);
        log->debug("coverage: {}", coverage);

    }
    fout.close();
    for (auto c : counters) {
        log->debug("{} : {} ", c.first, c.second);
    }

    dump_cg(out_graph, log);
    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    return true;








    // auto time_start = std::chrono::high_resolution_clock::now();
    // auto time_stop = std::chrono::high_resolution_clock::now();
    // auto time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);

    // // Geom. clustering (connected blobs)
    // time_start = std::chrono::high_resolution_clock::now();
    // auto id2cluster = get_geom_clusters(in_graph);
    // time_stop = std::chrono::high_resolution_clock::now();
    // time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    // log->debug("Geom. clustering: {} sec", time_duration.count());
    // log->debug("found: {} blob geom clusters", id2cluster.size());

    // // Make Projection2D and build Rectangles
    // time_start = std::chrono::high_resolution_clock::now();
    // std::unordered_map<int, layer_projection_map_t> id2lproj;
    // std::unordered_set<WirePlaneLayer_t> layers;
    // using rect_t = Rectangles<int, int>;
    // using layer_rects_t = std::unordered_map<WirePlaneLayer_t, rect_t>;
    // layer_rects_t lrecs;
    // for (auto ic : id2cluster) {
    //     int id = ic.first;
    //     auto cluster = ic.second;
    //     // TODO zero-copy?
    //     id2lproj[id] = get_2D_projection(in_graph, cluster);
    //     for (auto lproj : id2lproj[id]) {
    //         auto layer = lproj.first;
    //         auto proj = lproj.second;
    //         projection_bound_t bound = proj.m_bound;
    //         lrecs[layer].add(
    //             std::get<0>(bound),
    //             std::get<1>(bound)+1,
    //             std::get<2>(bound),
    //             std::get<3>(bound)+1,
    //             id);
    //         layers.insert(layer);
    //         // log->debug("{{{},{}}} => {}", id, layer, dump(proj));
    //         const std::string aname = String::format("proj2d_%d.tar.gz", (int)layer);
    //         // write(proj, aname);
    //     }
    //     // exit(42);
    // }
    // time_stop = std::chrono::high_resolution_clock::now();
    // time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    // log->debug("Make Projection2D and build Rectangles: {} sec", time_duration.count());

    // std::map<std::string, int> counters = {
    //     {"ncomp", 0},
    //     {"noverlap", 0}
    // };


    // // for debugging
    // // std::unordered_set<cluster_vertex_t> bs_keep{758, 634};
    // std::unordered_set<cluster_vertex_t> bs_keep;
    // std::ofstream fout("ProjectionDeghosting.log");

    // if (m_compare_rectangle) {
    //     time_start = std::chrono::high_resolution_clock::now();
    //     // std::unordered_set<int> check_ids = {70,71};

    //     using checked_pairs_t = std::set< std::pair<int, int> >;
    //     using layer_checked_pairs_t = std::unordered_map< WirePlaneLayer_t, checked_pairs_t>;
    //     layer_checked_pairs_t layer_checked_pairs;
    //     [&] {
    //     for (WirePlaneLayer_t layer : layers) {
    //         for (auto ref_ilp : id2lproj) {
    //             auto ref_id = ref_ilp.first;
    //             auto& ref_proj = ref_ilp.second[layer];
    //             // if (check_ids.find(ref_id)!=check_ids.end()) {
    //             //     log->debug("ref: {{{},{}}} => {}", ref_id, layer, dump(ref_proj));
    //             // }
    //             projection_bound_t ref_bound = ref_proj.m_bound;
    //             auto overlap_regions = lrecs[layer].intersection(
    //                 {std::get<0>(ref_bound),std::get<1>(ref_bound)+1},
    //                 {std::get<2>(ref_bound),std::get<3>(ref_bound)+1}
    //             );
    //             // if (check_ids.find(ref_id)!=check_ids.end()) {
    //             //     for(const auto& [rec, ids] : overlap_regions) {
    //             //         for (const auto& tar_id : ids) {
    //             //             log->debug("overlap_regions ids: {}", tar_id);
    //             //         }
    //             //     }
    //             // }
    //             for(const auto& [rec, ids] : overlap_regions) {
    //                 for (const auto& tar_id : ids) {
    //                     if (tar_id == ref_id) continue;
    //                     if (layer_checked_pairs[layer].find({ref_id, tar_id}) != layer_checked_pairs[layer].end()) continue;
    //                     counters["ncomp"] += 1;
    //                     auto& tar_proj = id2lproj[tar_id][layer];
    //                     int coverage = judge_coverage(ref_proj, tar_proj);
    //                     // log->debug("coverage: {}", coverage);
    //                     if (coverage != 0) {
    //                         counters["noverlap"] += 1;
    //                         if (m_verbose) {
    //                             if (id2cluster[ref_id].size() > 1) {
    //                                 log->debug("ref: {{{},{}}} => {}", ref_id, layer, dump(ref_proj,true));
    //                                 log->debug("tar: {{{},{}}} => {}", tar_id, layer, dump(tar_proj,true));
    //                             }
    //                         }
    //                         // dump specific clusters
    //                         if (true) {
    //                             if (ref_id == 1 && tar_id == 3 && layer == kVlayer) {
    //                                 write(ref_proj, String::format("ref_%d.tar.gz",ref_id));
    //                                 write(tar_proj, String::format("tar_%d.tar.gz",tar_id));
    //                             }
    //                         }
    //                         // dump specific clusters
    //                         if (true) {
    //                             std::stringstream ss;
    //                             ss << ref_id << " {";
    //                             for (auto& b : id2cluster[ref_id]) {
    //                                 ss << b << " ";
    //                                 bs_keep.insert(b);
    //                             }
    //                             ss << "} " << layer << "-> ";
    //                             ss << tar_id << " {";
    //                             for (auto& b : id2cluster[tar_id]) {
    //                                 ss << b << " ";
    //                                 // bs_keep.insert(b); // ref and tar are switched, one would be enough
    //                             }
    //                             ss << "} cov: " << coverage << std::endl;
    //                             fout << ss.str();
    //                             // return;
    //                         }
    //                     }
    //                     layer_checked_pairs[layer].insert({ref_id, tar_id});
    //                 }
    //             }
    //         }
    //     }
    //     }();
    //     fout.close();
    //     time_stop = std::chrono::high_resolution_clock::now();
    //     time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    //     log->debug("judge_coverage rectangle: {} sec", time_duration.count());
    //     for (auto c : counters) {
    //         log->debug("{} : {} ", c.first, c.second);
    //     }
    // }

    // // only keep some blobs
    // log->debug("in_graph:");
    // dump_cg(in_graph, log);
    // auto out_graph = remove_blobs(in_graph,bs_keep,false);
    // // auto& out_graph = in_graph;
    // log->debug("out_graph:");
    // dump_cg(out_graph, log);
    // out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    // return true;
}


