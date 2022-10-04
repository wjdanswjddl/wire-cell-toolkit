
#include "WireCellImg/TestProjection2D.h"
#include "WireCellImg/CSGraph.h"
#include "WireCellImg/Projection2D.h"

#include "WireCellAux/SimpleCluster.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Rectangles.h"

#include <iterator>
#include <chrono>

WIRECELL_FACTORY(TestProjection2D, WireCell::Img::TestProjection2D,
                 WireCell::INamed,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Img;
using namespace WireCell::Img::CS;
using namespace WireCell::Img::Projection2D;

Img::TestProjection2D::TestProjection2D()
    : Aux::Logger("TestProjection2D", "img")
{
}

Img::TestProjection2D::~TestProjection2D() {}

WireCell::Configuration Img::TestProjection2D::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["blob_value_threshold"] = m_blob_thresh.value();
    cfg["blob_error_threshold"] = m_blob_thresh.uncertainty();

    return cfg;
}

void Img::TestProjection2D::configure(const WireCell::Configuration& cfg)
{
    float bt_val = get(cfg,"blob_value_threshold", m_blob_thresh.value());
    float bt_err = get(cfg,"blob_error_threshold", m_blob_thresh.uncertainty());
    m_blob_thresh = value_t(bt_val, bt_err);
}

static void dump_cg(const cluster_graph_t& cg, Log::logptr_t& log)
{
    size_t mcount{0}, bcount{0};
    value_t bval;
    for (const auto& vtx : mir(boost::vertices(cg))) {
        const auto& node = cg[vtx];
        if (node.code() == 'b') {
            const auto iblob = get<blob_t>(node.ptr);
            bval += value_t(iblob->value(), iblob->uncertainty());
            ++bcount;
            continue;
        }
        if (node.code() == 'm') {
            const auto imeas = get<meas_t>(node.ptr);
            ++mcount;
        }
    }
    log->debug("cluster graph: vertices={} edges={} $blob={} bval={} #meas={}",
               boost::num_vertices(cg), boost::num_edges(cg),
               bcount, bval, mcount);
}

bool Img::TestProjection2D::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("EOS");
        return true;
    }
    const auto in_graph = in->graph();
    dump_cg(in_graph, log);
    auto& out_graph = in_graph;
    auto id2cluster = get_geom_clusters(out_graph);
    log->debug("found: {} blob geom clusters", id2cluster.size());
    std::unordered_map<int, layer_projection_map_t> id2lproj;
    std::unordered_set<WirePlaneLayer_t> layers;
    using rect_t = Rectangles<int, int>;
    using layer_rects_t = std::unordered_map<WirePlaneLayer_t, rect_t>;
    layer_rects_t lrecs;
    for (auto ic : id2cluster) {
        int id = ic.first;
        auto cluster = ic.second;
        // TODO zero-copy?
        id2lproj[id] = get_2D_projection(out_graph, cluster);
        for (auto lproj : id2lproj[id]) {
            auto layer = lproj.first;
            auto proj = lproj.second;
            projection_bound_t bound = proj.m_bound;
            lrecs[layer].add(
                std::get<0>(bound),
                std::get<1>(bound)+1,
                std::get<2>(bound),
                std::get<3>(bound)+1,
                id);
            layers.insert(layer);
            // log->debug("{{{},{}}} => {}", id, layer, dump(proj));
            const std::string aname = String::format("proj2d_%d.tar.gz", (int)layer);
            // write(proj, aname);
        }
        // exit(42);
    }

    // debug
    auto time_start = std::chrono::high_resolution_clock::now();
    std::map<std::string, int> counters = {
        {"ncomp", 0},
        {"n1", 0}
    };
    // std::unordered_set<int> check_ids = {70,71};

    using checked_pairs_t = std::set< std::pair<int, int> >;
    using layer_checked_pairs_t = std::unordered_map< WirePlaneLayer_t, checked_pairs_t>;
    layer_checked_pairs_t layer_checked_pairs;
    for (auto layer : layers) {
        for (auto ref_ilp : id2lproj) {
            auto ref_id = ref_ilp.first;
            auto& ref_proj = ref_ilp.second[layer];
            // if (check_ids.find(ref_id)!=check_ids.end()) {
            //     log->debug("ref: {{{},{}}} => {}", ref_id, layer, dump(ref_proj));
            // }
            projection_bound_t ref_bound = ref_proj.m_bound;
            auto overlap_regions = lrecs[layer].intersection(
                {std::get<0>(ref_bound),std::get<1>(ref_bound)+1},
                {std::get<2>(ref_bound),std::get<3>(ref_bound)+1}
            );
            // if (check_ids.find(ref_id)!=check_ids.end()) {
            //     for(const auto& [rec, ids] : overlap_regions) {
            //         for (const auto& tar_id : ids) {
            //             log->debug("overlap_regions ids: {}", tar_id);
            //         }
            //     }
            // }
            for(const auto& [rec, ids] : overlap_regions) {
                for (const auto& tar_id : ids) {
                    if (tar_id == ref_id) continue;
                    if (layer_checked_pairs[layer].find({ref_id, tar_id}) != layer_checked_pairs[layer].end()) continue;
                    counters["ncomp"] += 1;
                    auto& tar_proj = id2lproj[tar_id][layer];
                    int coverage = compare(ref_proj, tar_proj);
                    if (coverage == 1) {
                        counters["n1"] += 1;
                        log->debug("ref: {{{},{}}} => {}", ref_id, layer, dump(ref_proj));
                        log->debug("tar: {{{},{}}} => {}", tar_id, layer, dump(tar_proj));
                    }
                    layer_checked_pairs[layer].insert({ref_id, tar_id});
                }
            }
        }
    }

    auto time_stop = std::chrono::high_resolution_clock::now();
    auto time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    log->debug("compare: {} sec", time_duration.count());
    for (auto c : counters) {
        log->debug("{} : {} ", c.first, c.second);
    }
    counters["ncomp"] = 0;
    counters["n1"] = 0;

    // layer_checked_pairs.clear();
    time_start = std::chrono::high_resolution_clock::now();
    for (auto layer : layers) {
        for (auto ref_ilp : id2lproj) {
            auto ref_id = ref_ilp.first;
            auto& ref_proj = ref_ilp.second[layer];
            for (auto tar_ilp : id2lproj) {
                auto tar_id = tar_ilp.first;
                if (tar_id == ref_id) continue;
                // if (layer_checked_pairs[layer].find({ref_id, tar_id}) == layer_checked_pairs[layer].end()) continue;
                auto& tar_proj = tar_ilp.second[layer];
                counters["ncomp"] += 1;
                int coverage = compare(ref_proj, tar_proj);
                if (coverage == 1) {
                    counters["n1"] += 1;
                    log->debug("ref: {{{},{}}} => {}", ref_id, layer, dump(ref_proj));
                    log->debug("tar: {{{},{}}} => {}", tar_id, layer, dump(tar_proj));
                }
                // layer_checked_pairs[layer].insert({ref_id, tar_id});
            }
        }
    }
    time_stop = std::chrono::high_resolution_clock::now();
    time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    log->debug("compare: {} sec", time_duration.count());
    for (auto c : counters) {
        log->debug("{} : {} ", c.first, c.second);
    }

    dump_cg(out_graph, log);
    out = std::make_shared<Aux::SimpleCluster>(out_graph, in->ident());
    return true;
}


