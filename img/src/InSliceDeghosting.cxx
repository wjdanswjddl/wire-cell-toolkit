
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
    cfg["deghost_th"] = m_deghost_th;
    cfg["deghost_th1"] = m_deghost_th1;
    return cfg;
}

void Img::InSliceDeghosting::configure(const WireCell::Configuration& cfg)
{
    m_dryrun = get<bool>(cfg, "dryrun", m_dryrun);
    m_good_blob_charge_th = get<double>(cfg, "good_blob_charge_th", m_good_blob_charge_th);
    m_clustering_policy = get<std::string>(cfg, "clustering_policy", m_clustering_policy);
    m_config_round = get<int>(cfg, "config_round", m_config_round);
    m_deghost_th = get<float>(cfg, "deghost_th", m_deghost_th);
    m_deghost_th1 = get<float>(cfg, "deghost_th1", m_deghost_th1);

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
    std::unordered_map<WireCell::WirePlaneLayer_t, std::set<int> > connected_channels(const cluster_graph_t& cg,
                                                                                      const cluster_vertex_t& bdesc)
    {
        std::unordered_map<WireCell::WirePlaneLayer_t, std::set<int> > cidents;
        const auto wdescs = neighbors_oftype<cluster_node_t::wire_t>(cg, bdesc);
        for (auto wdesc : wdescs) {
            const auto cdescs = neighbors_oftype<cluster_node_t::channel_t>(cg, wdesc);
            /// 'w' can only be connect to 1 'c'
            if (cdescs.size() != 1) {
                THROW(ValueError() << errmsg{String::format("'w' connnected to %d 'c's!", cdescs.size())});
            }
            const auto ichannel = get<cluster_node_t::channel_t>(cg[cdescs[0]].ptr);
            // cidents.insert(std::make_pair(ichannel->ident(), ichannel->planeid().layer()) );
            //	    cidents[ichannel->ident()] = ichannel->planeid().layer();
            cidents[ichannel->planeid().layer()].insert(ichannel->ident());
        }
        return cidents;
    }

    bool adjacent(std::unordered_map<WireCell::WirePlaneLayer_t, std::set<int> >& cid1,
                  std::unordered_map<WireCell::WirePlaneLayer_t, std::set<int> >& cid2)
    {
        std::map<WireCell::WirePlaneLayer_t, std::pair<int, int> > map1_plane_chs, map2_plane_chs;
        std::map<WireCell::WirePlaneLayer_t, int> map_plane_score;

        for (auto it = cid1.begin(); it != cid1.end(); it++) {
            map_plane_score[it->first] = 0;
            map1_plane_chs[it->first] = std::make_pair(*it->second.begin(), *it->second.rbegin());
        }

        for (auto it = cid2.begin(); it != cid2.end(); it++) {
            map2_plane_chs[it->first] = std::make_pair(*it->second.begin(), *it->second.rbegin());
        }

        int sum_score = 0;
        for (auto it = map_plane_score.begin(); it != map_plane_score.end(); it++) {
            if (map1_plane_chs[it->first].first == map2_plane_chs[it->first].second + 1 ||
                map2_plane_chs[it->first].first == map1_plane_chs[it->first].second + 1) {
                map_plane_score[it->first] = 1;
            }
            else if (map1_plane_chs[it->first].first <= map2_plane_chs[it->first].second &&
                     map2_plane_chs[it->first].first <= map1_plane_chs[it->first].second) {
                map_plane_score[it->first] = 2;
            }

            if (map_plane_score[it->first] == 0) return false;
            sum_score += map_plane_score[it->first];
        }

        if (sum_score >= 5) {
            return true;
        }
        else {
            return false;
        }
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
        double current_q = iblob->value();  // charge ...
        if (current_q > m_good_blob_charge_th) {
            blob_tags.insert({vtx, GOOD});
            blob_tags.insert({vtx, POTENTIAL_GOOD});
        }
        //        else {
        //    blob_tags.insert({vtx, BAD});
        //}

        // loop over the b-b connections ...
        /// XIN: add between slice b-b connection info.
        /// follow b-b edges -> time, charge of connected blobs
        // std::vector<double> fronts, backs;
        for (auto v : neighbors_oftype<cluster_node_t::blob_t>(cg, vtx)) {
            const auto ib = get<cluster_node_t::blob_t>(cg[v].ptr);
            if (ib->value() > m_good_blob_charge_th) {
                blob_tags.insert({vtx, POTENTIAL_GOOD});
            }
            //  if (ib->slice()->start() > current_t) {
            //    backs.push_back(ib->value());
            // }
            // if (ib->slice()->start() < current_t) {
            //    fronts.push_back(ib->value());
            //}
        }

        /// DEBUGONLY:
        // log->debug("{} fronts {} backs {}", vtx, fronts.size(), backs.size());
    }
}
void InSliceDeghosting::local_deghosting1(const cluster_graph_t& cg, vertex_tags_t& blob_tags)
{
    // loop over time slices
    for (const auto& svtx : GraphTools::mir(boost::vertices(cg))) {
        if (cg[svtx].code() != 's') {
            continue;
        }

        std::unordered_map<int, float> wire_score_map;
        // Loop over the neighbor blobs of current slice. Keep the largest one.
        std::unordered_map<size_t, std::unordered_set<cluster_vertex_t> > view_groups;
        // std::unordered_map<RayGrid::grid_index_t, size_t> grid_occupancy;
        std::unordered_map<cluster_vertex_t, std::unordered_set<WireCell::WirePlaneLayer_t> > blob_planes;

        for (auto bedge : GraphTools::mir(boost::out_edges(svtx, cg))) {
            auto bvtx = boost::target(bedge, cg);
            if (cg[bvtx].code() != 'b') {
                THROW(ValueError() << errmsg{
                          String::format("'s' node connnected to '%s' not implemented!", cg[bvtx].code())});
            }

            /// group blobs into #live view group
            /// ASSUMPTION: connected meas node == # live views
            auto meas = neighbors_oftype<cluster_node_t::meas_t>(cg, bvtx);
            //	    std::unordered_set<WireCell::WirePlaneLayer_t> live_planes;
            view_groups[meas.size()].insert(bvtx);
            // how do we know which plane meas_t is bad ... ???
            for (auto mea : meas) {
                const auto imeasure = get<cluster_node_t::meas_t>(cg[mea].ptr);
                // log->debug("planeid {}",imeasure->planeid());
                blob_planes[bvtx].insert(imeasure->planeid().layer());
                // live_planes.insert(imeasure->planeid().layer());
            }

            if (meas.size() == 3) {  // all three plane live ...
                auto cidents = connected_channels(cg, bvtx);
                for (auto it = cidents.begin(); it != cidents.end(); it++) {
                    for (auto it1 = it->second.begin(); it1 != it->second.end(); it1++) {
                        if (wire_score_map.find(*it1) == wire_score_map.end()) {
                            wire_score_map[*it1] = 1;
                        }
                        else {
                            wire_score_map[*it1]++;
                        }
                    }
                }
            }
        }  // loop over blob ...

        // loop over two-wire blobs ...
        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            auto& live_planes = blob_planes[*it];
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
                if (flag_plane[it1->first]) {
                    for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                        if (wire_score_map.find(*it2) == wire_score_map.end()) {
                            wire_score_map[*it2] = 1;
                        }
                        else {
                            wire_score_map[*it2]++;
                        }
                    }
                }
            }
        }

        std::unordered_map<cluster_vertex_t, float> blob_high_score_map;

        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            auto& live_planes = blob_planes[*it];

            std::map<WireCell::WirePlaneLayer_t, float> score_plane;
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
                score_plane[it1->first] = -1;
                if (flag_plane[it1->first]) {
                    float sum1 = 0;
                    float sum2 = 0;
                    for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                        sum2++;
                        sum1 += 1. / wire_score_map[*it2];
                    }
                    score_plane[it1->first] = sum1 / sum2;
                }
            }

            blob_high_score_map[*it] =
                std::max(std::max(score_plane[kUlayer], score_plane[kVlayer]), score_plane[kWlayer]);
        }

        for (auto it = view_groups[3].begin(); it != view_groups[3].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            auto& live_planes = blob_planes[*it];

            std::map<WireCell::WirePlaneLayer_t, float> score_plane;
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
                score_plane[it1->first] = -1;
                if (flag_plane[it1->first]) {
                    float sum1 = 0;
                    float sum2 = 0;
                    for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                        sum2++;
                        sum1 += 1. / wire_score_map[*it2];
                    }
                    score_plane[it1->first] = sum1 / sum2;
                }
            }

            blob_high_score_map[*it] =
                std::max(std::max(score_plane[kUlayer], score_plane[kVlayer]), score_plane[kWlayer]);
            if (exist(blob_tags, *it, GOOD)) blob_high_score_map[*it] = 1;
        }

        std::unordered_set<cluster_vertex_t> cannot_remove;
        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            int count = 0;
            for (auto it1 = view_groups[3].begin(); it1 != view_groups[3].end(); it1++) {
                auto three_view_chs = connected_channels(cg, *it1);
                if (exist(blob_tags, *it1, GOOD)) {
                    if (adjacent(two_view_chs, three_view_chs)) count++;
                    if (count == 2) {
                        cannot_remove.insert(*it);
                    }
                }
            }
        }

        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            const auto iblob1 = get<cluster_node_t::blob_t>(cg[*it].ptr);
            double current_q1 = iblob1->value();  // charge ...

            auto& live_planes = blob_planes[*it];
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
            }

            int count = 0;
            // find the merged wire associated with this blob ...
            auto meas = neighbors_oftype<cluster_node_t::meas_t>(cg, *it);
            for (auto mea : meas) {
                const auto imeasure = get<cluster_node_t::meas_t>(cg[mea].ptr);
                // blob_planes[bvtx].insert(imeasure->planeid().layer());
                if (flag_plane[imeasure->planeid().layer()]) {
                    int mcell_lwire = *two_view_chs[imeasure->planeid().layer()].begin();
                    int mcell_hwire = *two_view_chs[imeasure->planeid().layer()].rbegin();

                    // find the merged blob associated with the merged wire ...
                    auto blobs = neighbors_oftype<cluster_node_t::blob_t>(cg, mea);
                    for (auto blob : blobs) {
                        if (blob == *it) continue;
                        auto blob_chs = connected_channels(cg, blob);
                        const auto iblob2 = get<cluster_node_t::blob_t>(cg[blob].ptr);
                        double current_q2 = iblob2->value();  // charge ...

                        if (blob_high_score_map[blob] > m_deghost_th) {
                            int mcell1_lwire = *blob_chs[imeasure->planeid().layer()].begin();
                            int mcell1_hwire = *blob_chs[imeasure->planeid().layer()].rbegin();
                            int min_wire = std::max(mcell_lwire, mcell1_lwire);
                            int max_wire = std::min(mcell_hwire, mcell1_hwire);

                            if ((max_wire - min_wire + 1.0) / (mcell_hwire - mcell_lwire + 1.0) >= m_deghost_th &&
                                current_q2 > current_q1 * m_deghost_th) {
                                count++;
                                break;
                            }
                        }
                    }
                }
            }

            if (count == 2 && cannot_remove.find(*it) == cannot_remove.end()) blob_tags.insert({*it, TO_BE_REMOVED});
        }

    }  // loop over slice
}

void InSliceDeghosting::local_deghosting(const cluster_graph_t& cg, vertex_tags_t& blob_tags)
{
    //    int count_keeper = 0;
    // loop over slices ...
    for (const auto& svtx : GraphTools::mir(boost::vertices(cg))) {
        if (cg[svtx].code() != 's') {
            continue;
        }

        // saved the used channels ...
        std::unordered_map<WireCell::WirePlaneLayer_t, std::set<int> > used_plane_channels;
        // Loop over the neighbor blobs of current slice. Keep the largest one.
        std::unordered_map<size_t, std::unordered_set<cluster_vertex_t> > view_groups;
        // std::unordered_map<RayGrid::grid_index_t, size_t> grid_occupancy;
        std::unordered_map<cluster_vertex_t, std::unordered_set<WireCell::WirePlaneLayer_t> > blob_planes;

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
            //	    std::unordered_set<WireCell::WirePlaneLayer_t> live_planes;
            view_groups[meas.size()].insert(bvtx);
            // how do we know which plane meas_t is bad ... ???
            for (auto mea : meas) {
                const auto imeasure = get<cluster_node_t::meas_t>(cg[mea].ptr);
                // log->debug("planeid {}",imeasure->planeid());
                blob_planes[bvtx].insert(imeasure->planeid().layer());
                // live_planes.insert(imeasure->planeid().layer());
            }

            /// EXAMPLE: add up #blobs using each RayGrid::grid_index_t
            // RayGrid ... layer_index_t ...
            // const IBlob::pointer iblob = get<cluster_node_t::blob_t>(cg[bvtx].ptr);
            // const auto& strips = iblob->shape().strips();
            // for (const RayGrid::Strip& strip : strips) {
            //     for (RayGrid::grid_index_t gridx = strip.bounds.first; gridx < strip.bounds.second; ++gridx) {
            //         if (grid_occupancy.find(gridx) != grid_occupancy.end()) {
            //             grid_occupancy[gridx]++;
            //         }
            //         else {
            //             grid_occupancy[gridx] = 1;
            //         }
            //     }
            // }

            /// EXAMPLE: b -> connected channel idents
            // how do we know which plane is channel is?
            auto cidents = connected_channels(cg, bvtx);
            if (meas.size() == 3) {  // all three plane live ...
                for (auto it = cidents.begin(); it != cidents.end(); it++) {
                    // int ich = it->first;
                    //		used_plane_channels[it->second].insert(it->first);
                    used_plane_channels[it->first].insert(it->second.begin(), it->second.end());
                }
            }
            //            log->debug("cidents {}", cidents.size());

            /// EXAMPLEONLY: temp tag all as TO_BE_REMOVED
            // blob_tags.insert({bvtx, TO_BE_REMOVED});
        }

        std::unordered_set<cluster_vertex_t> cannot_remove;
        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            auto two_view_chs = connected_channels(cg, *it);
            int count = 0;
            for (auto it1 = view_groups[3].begin(); it1 != view_groups[3].end(); it1++) {
                auto three_view_chs = connected_channels(cg, *it1);
                if (exist(blob_tags, *it1, GOOD)) {
                    if (adjacent(two_view_chs, three_view_chs)) count++;
                    if (count == 2) {
                        cannot_remove.insert(*it);
                    }
                }
            }
        }

        // loop over two-good wire cells
        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            if (!exist(blob_tags, *it, POTENTIAL_GOOD)) {
                auto two_view_chs = connected_channels(cg, *it);
                std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
                auto& live_planes = blob_planes[*it];
                bool save_flag = false;

                for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                    if (live_planes.find(it1->first) != live_planes.end()) {
                        flag_plane[it1->first] = true;
                    }
                    else {
                        flag_plane[it1->first] = false;
                    }
                    if (flag_plane[it1->first] && !save_flag) {
                        int nwire = 0;
                        int nwire_common = 0;
                        auto& used_chs = used_plane_channels[it1->first];
                        for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                            nwire++;
                            if (used_chs.find(*it2) != used_chs.end()) nwire_common++;
                        }
                        if (nwire_common != nwire) save_flag = true;
                    }
                }

                if ((!save_flag) && (cannot_remove.find(*it) == cannot_remove.end()))
                    blob_tags.insert({*it, TO_BE_REMOVED});
            }
        }

        std::unordered_map<int, float> wire_score_map;
        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            if (exist(blob_tags, *it, TO_BE_REMOVED)) continue;
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            auto& live_planes = blob_planes[*it];
            // std::unordered_map<WireCell::WirePlaneLayer_t, float> score_plane;
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
                // score_plane[it1->first] = -1;
                if (flag_plane[it1->first]) {
                    for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                        if (wire_score_map.find(*it2) == wire_score_map.end()) {
                            wire_score_map[*it2] = 1;
                        }
                        else {
                            wire_score_map[*it2]++;
                        }
                    }
                }
            }
        }

        for (auto it = view_groups[2].begin(); it != view_groups[2].end(); it++) {
            if (exist(blob_tags, *it, TO_BE_REMOVED)) continue;
            auto two_view_chs = connected_channels(cg, *it);
            std::unordered_map<WireCell::WirePlaneLayer_t, bool> flag_plane;
            auto& live_planes = blob_planes[*it];
            std::unordered_map<WireCell::WirePlaneLayer_t, float> score_plane;
            for (auto it1 = two_view_chs.begin(); it1 != two_view_chs.end(); it1++) {
                if (live_planes.find(it1->first) != live_planes.end()) {
                    flag_plane[it1->first] = true;
                }
                else {
                    flag_plane[it1->first] = false;
                }
                score_plane[it1->first] = -1;
                if (flag_plane[it1->first]) {
                    float sum1 = 0;
                    float sum2 = 0;
                    auto& used_chs = used_plane_channels[it1->first];
                    for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
                        sum2++;
                        if (used_chs.find(*it2) == used_chs.end()) {
                            sum1 += 1. / wire_score_map[*it2];
                        }
                    }
                    score_plane[it1->first] = sum1 / sum2;
                }
            }
            if (score_plane[WireCell::kUlayer] <= m_deghost_th1 && score_plane[WireCell::kVlayer] <= m_deghost_th1 &&
                score_plane[WireCell::kWlayer] <= m_deghost_th1 && (cannot_remove.find(*it) == cannot_remove.end()) &&
                (!exist(blob_tags, *it, POTENTIAL_GOOD)))
                blob_tags.insert({*it, POTENTIAL_BAD});
        }

        /// EXAMPLEONLY: rm 3views from TO_BE_REMOVED
        /// this is to prune all 3views from blob_tags, for single key deletion, use equal_range.
        //  for (auto i = blob_tags.begin(), last = blob_tags.end(); i != last;) {
        //    auto const& [key, value] = *i;
        // if (view_groups[3].find(key) != view_groups[3].end() && value == TO_BE_REMOVED) {
        //     i = blob_tags.erase(i);
        //     ++count_keeper;
        // }
        // else {
        //     ++i;
        // }
        // }
    }
    //    log->debug("count_keeper {}", count_keeper);
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

    if (m_config_round == 1) {
        /// 1, Ident good/bad blob groups. in: ICluster out: blob_quality_tags {blob_desc -> quality_tag} TODO: map or
        /// multi-map?
        /// FIXME: place holder. Using simple thresholing.
        blob_quality_ident(in_graph, blob_tags);

        /// 2, Local (in-slice) de-ghosting. in: ICluster + blob_quality_tags out: updated blob_quality_tags (remove or
        /// not)
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
        /// four separated groups, good&bad, good&good, bad&bad  (two of these might not be useful ...)
        std::vector<gc_filter_t> filters = {
            [&](const cluster_vertex_t& vtx) {
                return exist(blob_tags, vtx, POTENTIAL_GOOD) && !exist(blob_tags, vtx, POTENTIAL_BAD);
            },
            [&](const cluster_vertex_t& vtx) {
                return exist(blob_tags, vtx, POTENTIAL_GOOD) && exist(blob_tags, vtx, POTENTIAL_BAD);
            },
            [&](const cluster_vertex_t& vtx) {
                return !exist(blob_tags, vtx, POTENTIAL_GOOD) && !exist(blob_tags, vtx, POTENTIAL_BAD);
            },
            [&](const cluster_vertex_t& vtx) {
                return !exist(blob_tags, vtx, POTENTIAL_GOOD) && exist(blob_tags, vtx, POTENTIAL_BAD);
            },
        };
        /// each filter adds new b-b edges on cg_new_bb
        /// this one is too slow and stuck for 0 - 9592 full tick range
        // for (auto filter : filters) {
        //     geom_clustering(cg_new_bb, m_clustering_policy, filter);
        // }

        /// condense multi-map to map
        std::unordered_map<cluster_vertex_t, int> groups;
        for (int groupid = 0; groupid < filters.size(); ++groupid) {
            for (const auto& vtx : oftype(cg_new_bb, 'b')) {
                if (filters[groupid](vtx)) groups.insert({vtx, groupid});
            }
        }

        /// DEBUGONLY: 
        for (const auto& vtx : oftype(cg_new_bb, 'b')) {
            if (groups.find(vtx) == groups.end()) {
                log->debug("{} not found in gruops!", vtx);
            }
        }
        grouped_geom_clustering(cg_new_bb, m_clustering_policy, groups);
        log->debug("cg_new_bb:");
        dump_cg(cg_new_bb, log);
    }
    else if (m_config_round == 2) {
        // after charge solving ...
        blob_quality_ident(in_graph, blob_tags);

        // second round of local deghosting ...
        local_deghosting1(in_graph, blob_tags);

        using VFiltered =
            typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
        VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) { return !exist(blob_tags, vtx, TO_BE_REMOVED); });
        // do we have to copy this every time ???
        boost::copy_graph(fg_rm_bad_blobs, cg_new_bb);
    }
    else if (m_config_round == 3) {
        // after charge solving ...
        blob_quality_ident(in_graph, blob_tags);

        // at some points remove the Non-potential good ... (last round ...)
        using VFiltered =
            typename boost::filtered_graph<cluster_graph_t, boost::keep_all, std::function<bool(cluster_vertex_t)> >;
        VFiltered fg_rm_bad_blobs(in_graph, {}, [&](auto vtx) {
            if (in_graph[vtx].code() != 'b') return true;
            return exist(blob_tags, vtx, POTENTIAL_GOOD);
        });
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
