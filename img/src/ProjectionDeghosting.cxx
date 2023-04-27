
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
    cfg["nchan"] = m_nchan;
    cfg["nslice"] = m_nslice;
    cfg["dryrun"] = m_dryrun;

    return cfg;
}

void Img::ProjectionDeghosting::configure(const WireCell::Configuration& cfg)
{
    m_verbose = get<bool>(cfg,"verbose", m_verbose);
    m_nchan = get<size_t>(cfg,"nchan", m_nchan);
    m_nslice = get<size_t>(cfg,"nslice", m_nslice);
    m_dryrun = get<bool>(cfg,"dryrun", m_dryrun);
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
        log->debug("cluster graph: vertices={} edges={} #blob={} bval={} #meas={}",
                   boost::num_vertices(cg), boost::num_edges(cg),
                   bcount, bval, mcount);
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
    Projection2D::LayerProjection2DMap& get_projection(cluster_proj_t& id2lproj, const ClusterShadow::vdesc_t& id,
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

bool Img::ProjectionDeghosting::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);
    BlobShadow::graph_t bsgraph = BlobShadow::shadow(in_graph, 'w'); // or 'c'
    ClusterShadow::blob_cluster_map_t clusters;
    auto cs_graph = ClusterShadow::shadow(in_graph, bsgraph, clusters);
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

    // TODO: figure out how to do this right
    std::unordered_set<cluster_vertex_t> tagged_bs;

    for (const auto& cs_edge : GraphTools::mir(boost::edges(cs_graph))) {
        counters["cs_edges"] += 1;
        WireCell::WirePlaneLayer_t layer = cs_graph[cs_edge].wpid.layer();
        ClusterShadow::vdesc_t tail = boost::source(cs_edge, cs_graph);
        ClusterShadow::vdesc_t head = boost::target(cs_edge, cs_graph);
        if (tail == head) {
            continue;
        }
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
        int coverage_alt = Projection2D::judge_coverage_alt(ref_proj, tar_proj);
        log->debug("coverage: {} alt: {}", coverage, coverage_alt);

        // DEBUGONLY
        coverage = coverage_alt;

        // DEBUGONLY
        // std::stringstream ss;
        // ss << tail << " {";
        // for (auto& b : b_tail) {
        //     auto bptr = std::get<cluster_node_t::blob_t>(in_graph[b].ptr);
        //     ss << bptr->ident() << " ";
        // }
        // ss << "} " << layer << "-> ";
        // ss << head << " {";
        // for (auto& b : b_head) {
        //     auto bptr = std::get<cluster_node_t::blob_t>(in_graph[b].ptr);
        //     ss << bptr->ident() << " ";
        // }
        // ss << "} cov: " << coverage << std::endl;
        // fout << ss.str();
        // std::cout << ss.str();

        // DEBUGONLY
        // if (head == 7 || head == 9) {
        //     for (auto& b : b_head) {
        //         tagged_bs.insert(b);
        //     }
        //     for (auto& b : b_tail) {
        //         tagged_bs.insert(b);
        //     }
        //     break;
        // } else {
        //     continue;
        // }
        
        // tail contains head, remove blobs in head
        if (coverage == 1) {
            for (auto& b : b_head) {
                tagged_bs.insert(b);
            }
        }
        // vice versa
        if (coverage == -1) {
            for (auto& b : b_tail) {
                tagged_bs.insert(b);
            }
        }
    }
    fout.close();
    for (auto c : counters) {
        log->debug("{} : {} ", c.first, c.second);
    }

    // true: remove tagged_bs; false: only keep tagged_bs
    auto out_graph = remove_blobs(in_graph,tagged_bs,true);

    // debug info
    log->debug("tagged_bs.size(): {}",tagged_bs.size());
    log->debug("in_graph:");
    dump_cg(in_graph, log);
    log->debug("out_graph:");
    dump_cg(out_graph, log);

    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    if(m_dryrun) {
        out = std::make_shared<Aux::SimpleCluster>(in_graph, in->ident());
    }
    return true;
}


