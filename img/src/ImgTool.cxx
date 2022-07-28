#include "WireCellImg/ImgTool.h"
#include "WireCellUtil/Exceptions.h"

using namespace WireCell;
using namespace WireCell::Img::Tool;


template <typename It> boost::iterator_range<It> mir(std::pair<It, It> const& p) {
    return boost::make_iterator_range(p.first, p.second);
}
template <typename It> boost::iterator_range<It> mir(It b, It e) {
    return boost::make_iterator_range(b, e);
}

std::vector<vdesc_t> WireCell::Img::Tool::neighbors(const WireCell::cluster_graph_t& cg, const vdesc_t& vd)
{
    std::vector<vdesc_t> ret;
    for (auto edge : boost::make_iterator_range(boost::out_edges(vd, cg))) {
        vdesc_t neigh = boost::target(edge, cg);
        ret.push_back(neigh);
    }
    return ret;
}

template <typename Type>
std::vector<vdesc_t> WireCell::Img::Tool::neighbors_oftype(const WireCell::cluster_graph_t& cg, const vdesc_t& vd)
{
    std::vector<vdesc_t> ret;
    for (const auto& vp : neighbors(cg, vd)) {
        if (std::holds_alternative<Type>(cg[vp].ptr)) {
            ret.push_back(vp);
        }
    }
    return ret;
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

    // debug
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

Projection2D WireCell::Img::Tool::get_2D_projection(
    const WireCell::cluster_graph_t& cg, std::vector<vdesc_t> group)
{
    using triplet_t = Eigen::Triplet<double>;
    using triplet_vec_t = std::vector<triplet_t>;
    std::unordered_map<WirePlaneLayer_t, triplet_vec_t> lcoeff;
    int chan_min = std::numeric_limits<int>::max();
    int chan_max = std::numeric_limits<int>::min();
    int tick_min = std::numeric_limits<int>::max();
    int tick_max = std::numeric_limits<int>::min();

    // assumes one blob linked to one slice
    // use b-w-c to find all channels linked to the blob
    for (const auto& blob_desc : group) {
        const auto& node = cg[blob_desc];
        if (node.code() == 'b') {
            const auto slice_descs = neighbors_oftype<slice_t>(cg, blob_desc);
            if (slice_descs.size() != 1) {
                THROW(ValueError() << errmsg{"slice_descs.size()!=1"});
            }
            auto slice = std::get<slice_t>(cg[slice_descs.front()].ptr);
            int start = int(slice->start() / (500 * units::nanosecond));
            int span = int(slice->span() / (500 * units::nanosecond));
            auto activity = slice->activity();
            const auto wire_descs = neighbors_oftype<wire_t>(cg, blob_desc);
            // std::cout << " #wire " << wire_descs.size();
            for (const auto wire_desc : wire_descs) {
                const auto chan_descs = neighbors_oftype<channel_t>(cg, wire_desc);
                for (const auto chan_desc : chan_descs) {
                    auto chan = std::get<channel_t>(cg[chan_desc].ptr);
                    WirePlaneLayer_t layer = chan->planeid().layer();
                    int index = chan->index();
                    auto charge = activity[chan].value();
                    // FIXME how to fill this?
                    lcoeff[layer].push_back({index, start, charge});
                    if (index < chan_min) chan_min = index;
                    if (index > chan_max) chan_max = index;
                    if (start < tick_min) tick_min = start;
                    if (start > tick_max) tick_max = start;
                }
            }
            // std::cout << std::endl;
        }
    }

    Projection2D proj2d;
    proj2d.m_bound = {chan_min, chan_max, tick_min, tick_max};
    auto& lproj = proj2d.m_lproj;
    for (auto lc : lcoeff) {
        auto l = lc.first;
        auto c = lc.second;
        lproj[l] = sparse_dmat_t(8256,9592);
        lproj[l].setFromTriplets(c.begin(), c.end());
    }
    return proj2d;
}

std::string WireCell::Img::Tool::dump(const Projection2D& proj2d, bool verbose) {
    std::stringstream ss;
    ss << "Projection2D ";
    ss << "bounds: "
    << " " << std::get<0>(proj2d.m_bound)
    << " " << std::get<1>(proj2d.m_bound)
    << " " << std::get<2>(proj2d.m_bound)
    << " " << std::get<3>(proj2d.m_bound);

    for (const auto lm : proj2d.m_lproj) {
        ss << " layer: " << lm.first;
        auto ctq = lm.second;
        size_t counter = 0;
        for (int k=0; k<ctq.outerSize(); ++k) {
            for (sparse_dmat_t::InnerIterator it(ctq,k); it; ++it)
            {
                if (verbose) {
                    ss << " {" << it.row() << ", " << it.col() << "}->" << it.value();
                }
                ++counter;
            }
        }
        ss << " #: " << counter;
    }
    return ss.str();
}