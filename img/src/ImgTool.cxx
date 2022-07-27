#include "WireCellImg/ImgTool.h"

using namespace WireCell;
using namespace WireCell::Img::Tool;


template <typename It> boost::iterator_range<It> mir(std::pair<It, It> const& p) {
    return boost::make_iterator_range(p.first, p.second);
}
template <typename It> boost::iterator_range<It> mir(It b, It e) {
    return boost::make_iterator_range(b, e);
}

std::unordered_map<int, std::vector<vdesc_t> > WireCell::Img::Tool::get_geom_clusters(const WireCell::cluster_graph_t& cg)
{
    std::unordered_map<int, std::vector<vdesc_t> > groups;
    cluster_graph_t cg_blob;
    
    size_t nblobs = 0;
    std::unordered_map<cluster_vertex_t, cluster_vertex_t> old2new;
    std::unordered_map<cluster_vertex_t, cluster_vertex_t> new2old;
    for (const auto& vtx : mir(boost::vertices(cg))) {
        const auto& node = cg[vtx];
        if (node.code() == 'b') {
            ++nblobs;
            auto newvtx = boost::add_vertex(node, cg_blob);
            old2new[vtx] = newvtx;
            new2old[newvtx] = vtx;
        }
    }
    
    if (!nblobs) {
        return groups;
    }

    for (auto edge : mir(boost::edges(cg))) {
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
        const auto& hnode = cg_blob[old_hit->second];
        const auto& tnode = cg_blob[old_tit->second];
        if (hnode.code() == 'b' && tnode.code() == 'b') {
            boost::add_edge(old_tit->second, old_hit->second, cg_blob);
        }
    }

    std::unordered_map<vdesc_t, int> desc2id;
    boost::connected_components(cg_blob, boost::make_assoc_property_map(desc2id));
    for (auto& [desc,id] : desc2id) {  // invert
        groups[id].push_back(new2old[desc]);
    }
    for (auto id2desc : groups) {
        auto & descvec = id2desc.second;
        if (descvec.size()>10) {
            std::cout << id2desc.first << ": ";
            for (auto &desc : descvec) {
                std::cout << desc << " ";
            }
            std::cout << std::endl;
        }
    }

    return groups;
}

sparse_dmat_t WireCell::Img::Tool::get_2D_projection(const WireCell::cluster_graph_t& cg, std::vector<vdesc_t> group)
{
    // 
    // for (const auto& vtx : group) {
    //     const auto& node = cg[vtx];
    //     if (node.code() == 'b') {
    //         ++nblobs;
    //         auto newvtx = boost::add_vertex(node, cg_blob);
    //         old2new[vtx] = newvtx;
    //         new2old[newvtx] = vtx;
    //     }
    // }

    using triplet_t = Eigen::Triplet<double>;
    // channel-tick-charge matrix
    sparse_dmat_t ctq(2,3);
    std::vector<triplet_t> coeff;
    coeff.push_back({1,2,42});
    ctq.setFromTriplets(coeff.begin(), coeff.end());
    return ctq;
}